#include "search.hpp"
#include <random>
#include <set>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <cstdio>

// Random seed generator
std::string generateRandomSeed() {
    static const char* chars = "ABCDEFGHIJKLMNPQRSTUVWXYZ123456789"; // No O or 0
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> charDist(0, 33);
    static std::uniform_int_distribution<> lengthDist(4, 8);

    int length = lengthDist(gen);
    std::string seed;
    for (int i = 0; i < length; i++) {
        seed += chars[charDist(gen)];
    }
    return seed;
}

// Single seed analyzer
SeedMatch analyzeSeedForSearch(const std::string& seed, const SearchOptions& options) {
    SeedMatch result;
    result.seed = seed;

    Instance inst(seed);
    inst.params = InstParams(options.deck, options.stake, false, 10106);
    inst.initLocks(1, false, false);

    // Lock standard vouchers
    std::vector<std::string> lockedVouchers = {
        "Overstock Plus", "Liquidation", "Clearance Sale", "Hone",
        "Glow Up", "Reroll Glut", "Omen Globe", "Observatory", "Nacho Tong",
        "Recyclomancy", "Money Tree", "Antimatter", "Illusion",
        "Petroglyph", "Retcon", "Palette"
    };
    for (const auto& v : lockedVouchers) {
        inst.lock(v);
    }

    inst.setStake(options.stake);
    inst.setDeck(options.deck);

    int maxAnte = options.earlyExit > 0 ? std::min(options.earlyExit, options.maxAnte) : options.maxAnte;

    for (int a = 1; a <= maxAnte; a++) {
        inst.initUnlocks(a, false);

        // Boss
        std::string boss = inst.nextBoss(a);
        for (const auto& bossSearch : options.bossTerms) {
            if (containsIgnoreCase(boss, bossSearch)) {
                result.details.push_back({
                    "Boss", boss, a, 0, ""
                });
            }
        }

        // Also check general find terms for boss
        for (const auto& findSearch : options.findTerms) {
            if (containsIgnoreCase(boss, findSearch)) {
                result.details.push_back({
                    "Item", boss, a, 0, ""
                });
            }
        }

        // Voucher
        std::string voucher = inst.nextVoucher(a);
        inst.lock(voucher);

        // Unlock next level voucher
        for (int i = 0; i < VOUCHERS.size(); i += 2) {
            if (VOUCHERS[i] == voucher && i + 1 < VOUCHERS.size()) {
                inst.unlock(VOUCHERS[i + 1]);
                break;
            }
        }

        for (const auto& voucherSearch : options.voucherTerms) {
            if (containsIgnoreCase(voucher, voucherSearch)) {
                result.details.push_back({
                    "Voucher", voucher, a, 0, ""
                });
            }
        }

        // Also check general find terms for vouchers
        for (const auto& findSearch : options.findTerms) {
            if (containsIgnoreCase(voucher, findSearch)) {
                result.details.push_back({
                    "Item", voucher, a, 0, ""
                });
            }
        }

        // Tags
        std::string tag1 = inst.nextTag(a);
        std::string tag2 = inst.nextTag(a);

        for (const auto& tagSearch : options.tagTerms) {
            if (containsIgnoreCase(tag1, tagSearch)) {
                result.details.push_back({
                    "Tag", tag1, a, 0, ""
                });
            }
            if (containsIgnoreCase(tag2, tagSearch)) {
                result.details.push_back({
                    "Tag", tag2, a, 0, ""
                });
            }
        }

        // Also check general find terms for tags
        for (const auto& findSearch : options.findTerms) {
            if (containsIgnoreCase(tag1, findSearch)) {
                result.details.push_back({
                    "Item", tag1, a, 0, ""
                });
            }
            if (containsIgnoreCase(tag2, findSearch)) {
                result.details.push_back({
                    "Item", tag2, a, 0, ""
                });
            }
        }

        // Shop items
        int shopLimit = (a - 1 < options.shopLimits.size())
            ? options.shopLimits[a - 1]
            : options.shopLimits.back();

        for (int i = 1; i <= shopLimit; i++) {
            try {
                ShopItem item = inst.nextShopItem(a);
                std::string itemName = item.item;

                if (item.type == "Joker") {
                    // Build full joker description
                    std::string jokerDesc = itemName;
                    if (item.jokerData.edition != "No Edition") {
                        jokerDesc = item.jokerData.edition + " " + jokerDesc;
                    }
                    if (item.jokerData.stickers.eternal) {
                        jokerDesc = "Eternal " + jokerDesc;
                    }
                    if (item.jokerData.stickers.perishable) {
                        jokerDesc = "Perishable " + jokerDesc;
                    }
                    if (item.jokerData.stickers.rental) {
                        jokerDesc = "Rental " + jokerDesc;
                    }

                    // Check joker search terms
                    for (const auto& jokerSearch : options.jokerTerms) {
                        if (containsIgnoreCase(jokerDesc, jokerSearch)) {
                            result.details.push_back({
                                "Joker", jokerDesc, a, i, ""
                            });
                        }
                    }

                    // Check general find terms
                    for (const auto& findSearch : options.findTerms) {
                        if (containsIgnoreCase(jokerDesc, findSearch)) {
                            result.details.push_back({
                                "Item", jokerDesc, a, i, ""
                            });
                        }
                    }
                } else {
                    // Non-joker items
                    for (const auto& findSearch : options.findTerms) {
                        if (containsIgnoreCase(itemName, findSearch)) {
                            result.details.push_back({
                                "Item", itemName, a, i, ""
                            });
                        }
                    }
                }
            } catch (...) {
                // If we hit memory issues, stop generating more items
                break;
            }
        }

        // Packs
        for (int p = 1; p <= 6; p++) {
            try {
                std::string packName = inst.nextPack(a);
                Pack packInfoData = ::packInfo(packName);

                if (packInfoData.type == "Celestial Pack") {
                    std::vector<std::string> cards = inst.nextCelestialPack(packInfoData.size, a);
                    for (const auto& card : cards) {
                        for (const auto& findSearch : options.findTerms) {
                            if (containsIgnoreCase(card, findSearch)) {
                                result.details.push_back({
                                    "Pack", card, a, 0, packName
                                });
                            }
                        }
                    }
                } else if (packInfoData.type == "Arcana Pack") {
                    std::vector<std::string> cards = inst.nextArcanaPack(packInfoData.size, a);
                    for (const auto& card : cards) {
                        std::string cardDesc = card;
                        if (card == "The Soul") {
                            // The Soul creates a legendary joker
                            JokerData legendaryJoker = inst.nextJoker("sou", a, true);
                            std::string jokerDesc = legendaryJoker.joker;
                            if (legendaryJoker.edition != "No Edition") {
                                jokerDesc = legendaryJoker.edition + " " + jokerDesc;
                            }
                            if (legendaryJoker.stickers.eternal) {
                                jokerDesc = "Eternal " + jokerDesc;
                            }
                            if (legendaryJoker.stickers.perishable) {
                                jokerDesc = "Perishable " + jokerDesc;
                            }
                            if (legendaryJoker.stickers.rental) {
                                jokerDesc = "Rental " + jokerDesc;
                            }
                            cardDesc = card + " (creates " + jokerDesc + ")";
                        }

                        for (const auto& findSearch : options.findTerms) {
                            if (containsIgnoreCase(cardDesc, findSearch)) {
                                result.details.push_back({
                                    "Pack", cardDesc, a, 0, packName
                                });
                            }
                        }
                    }
                } else if (packInfoData.type == "Spectral Pack") {
                    std::vector<std::string> cards = inst.nextSpectralPack(packInfoData.size, a);
                    for (const auto& card : cards) {
                        for (const auto& findSearch : options.findTerms) {
                            if (containsIgnoreCase(card, findSearch)) {
                                result.details.push_back({
                                    "Pack", card, a, 0, packName
                                });
                            }
                        }
                    }
                } else if (packInfoData.type == "Buffoon Pack") {
                    std::vector<JokerData> jokers = inst.nextBuffoonPack(packInfoData.size, a);
                    for (const auto& joker : jokers) {
                        for (const auto& findSearch : options.findTerms) {
                            if (containsIgnoreCase(joker.joker, findSearch)) {
                                result.details.push_back({
                                    "Pack", joker.joker, a, 0, packName
                                });
                            }
                        }
                        // Also check joker-specific searches
                        for (const auto& jokerSearch : options.jokerTerms) {
                            if (containsIgnoreCase(joker.joker, jokerSearch)) {
                                result.details.push_back({
                                    "Pack", joker.joker, a, 0, packName
                                });
                            }
                        }
                    }
                } else if (packInfoData.type == "Standard Pack") {
                    std::vector<Card> cards = inst.nextStandardPack(packInfoData.size, a);
                    // For standard packs, we typically only care about special cards
                    // We could add card formatting here if needed
                }
            } catch (...) {
                // If we hit memory issues, stop generating packs
                break;
            }
        }
    }

    return result;
}

// Worker thread function for multithreaded search
void searchWorker(const SearchOptions& options, const std::set<std::string>& requiredMatches,
                  std::vector<SeedMatch>& matches, std::mutex& matchesMutex,
                  std::atomic<int>& seedsChecked, std::atomic<bool>& shouldStop) {


    while (!shouldStop && (options.unlimited || seedsChecked.load() < options.maxSeeds)) {
        std::string seed = generateRandomSeed();
        SeedMatch result = analyzeSeedForSearch(seed, options);

        bool matched = false;
        if (options.matchAll && !requiredMatches.empty()) {
            // AND logic - need all terms
            std::set<std::string> foundMatches;
            for (const auto& detail : result.details) {
                if (detail.type == "Joker") {
                    for (const auto& term : options.jokerTerms) {
                        if (containsIgnoreCase(detail.name, term)) {
                            foundMatches.insert("joker:" + term);
                        }
                    }
                } else if (detail.type == "Voucher") {
                    for (const auto& term : options.voucherTerms) {
                        if (containsIgnoreCase(detail.name, term)) {
                            foundMatches.insert("voucher:" + term);
                        }
                    }
                } else if (detail.type == "Boss") {
                    for (const auto& term : options.bossTerms) {
                        if (containsIgnoreCase(detail.name, term)) {
                            foundMatches.insert("boss:" + term);
                        }
                    }
                } else if (detail.type == "Tag") {
                    for (const auto& term : options.tagTerms) {
                        if (containsIgnoreCase(detail.name, term)) {
                            foundMatches.insert("tag:" + term);
                        }
                    }
                } else if (detail.type == "Item") {
                    for (const auto& term : options.findTerms) {
                        if (containsIgnoreCase(detail.name, term)) {
                            foundMatches.insert("find:" + term);
                        }
                    }
                } else if (detail.type == "Pack") {
                    for (const auto& term : options.findTerms) {
                        if (containsIgnoreCase(detail.name, term)) {
                            foundMatches.insert("find:" + term);
                        }
                    }
                }
            }
            matched = (foundMatches == requiredMatches);
        } else {
            // OR logic - any match
            matched = !result.details.empty();
        }

        if (matched) {
            std::lock_guard<std::mutex> lock(matchesMutex);
            matches.push_back(result);
            if (options.stopOnFirst) {
                shouldStop = true;
                break;
            }
        }

        seedsChecked++;

    }
}

// Global pointer to current seeds checked counter
static std::atomic<int>* currentSeedsChecked = nullptr;

// Get current seed count for progress tracking
int getSeedsChecked() {
    return currentSeedsChecked ? currentSeedsChecked->load() : 0;
}

// Main search function
std::vector<SeedMatch> searchSeeds(const SearchOptions& options) {
    std::vector<SeedMatch> matches;
    std::mutex matchesMutex;
    std::atomic<int> seedsChecked(0);
    std::atomic<bool> shouldStop(false);

    // Set global pointer for progress tracking
    currentSeedsChecked = &seedsChecked;

    // Determine which terms we're looking for
    std::set<std::string> requiredMatches;
    if (options.matchAll) {
        for (const auto& term : options.findTerms) requiredMatches.insert("find:" + term);
        for (const auto& term : options.jokerTerms) requiredMatches.insert("joker:" + term);
        for (const auto& term : options.voucherTerms) requiredMatches.insert("voucher:" + term);
        for (const auto& term : options.bossTerms) requiredMatches.insert("boss:" + term);
        for (const auto& term : options.tagTerms) requiredMatches.insert("tag:" + term);
    }

    // Use multithreading if threads > 1
    if (options.threads > 1) {
        std::vector<std::thread> workers;

        for (int i = 0; i < options.threads; i++) {
            workers.emplace_back(searchWorker, std::cref(options), std::cref(requiredMatches),
                               std::ref(matches), std::ref(matchesMutex),
                               std::ref(seedsChecked), std::ref(shouldStop));
        }


        // Wait for all threads to complete
        for (auto& worker : workers) {
            worker.join();
        }
    } else {
        // Single-threaded version
        searchWorker(options, requiredMatches, matches, matchesMutex, seedsChecked, shouldStop);
    }


    return matches;
}