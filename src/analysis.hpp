#pragma once

#include "../include/immolate.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <unordered_set>
#include <utility>

namespace analysis {
inline const std::vector<std::string> kDefaultLockedVouchers = {
    "Overstock Plus", "Liquidation", "Glow Up",      "Reroll Glut",
    "Omen Globe",     "Observatory", "Nacho Tong",   "Recyclomancy",
    "Tarot Tycoon",   "Planet Tycoon", "Money Tree", "Antimatter",
    "Illusion",       "Petroglyph",  "Retcon",      "Palette"
};

inline const std::vector<std::string> kOptionalUnlocks = {
    "Negative Tag",   "Foil Tag",       "Holographic Tag", "Polychrome Tag",
    "Rare Tag",       "Golden Ticket",  "Mr. Bones",       "Acrobat",
    "Sock and Buskin","Swashbuckler",   "Troubadour",      "Certificate",
    "Smeared Joker",  "Throwback",      "Hanging Chad",    "Rough Gem",
    "Bloodstone",     "Arrowhead",      "Onyx Agate",      "Glass Joker",
    "Showman",        "Flower Pot",     "Blueprint",       "Wee Joker",
    "Merry Andy",     "Oops! All 6s",   "The Idol",        "Seeing Double",
    "Matador",        "Hit the Road",   "The Duo",         "The Trio",
    "The Family",     "The Order",      "The Tribe",       "Stuntman",
    "Invisible Joker","Brainstorm",     "Satellite",       "Shoot the Moon",
    "Driver's License","Cartomancer",    "Astronomer",      "Burnt Joker",
    "Bootstraps",     "Overstock Plus", "Liquidation",     "Glow Up",
    "Reroll Glut",    "Omen Globe",     "Observatory",     "Nacho Tong",
    "Recyclomancy",   "Tarot Tycoon",   "Planet Tycoon",   "Money Tree",
    "Antimatter",     "Illusion",       "Petroglyph",      "Retcon",
    "Palette"
};

struct JokerInfo {
    std::string name;
    std::vector<std::string> modifiers;
};

struct ShopEntry {
    int index;
    std::string type;
    std::string display;
    JokerInfo joker;
};

struct PackEntry {
    std::string name;
    std::vector<std::string> contents;
};

struct AnteResult {
    int ante = 0;
    std::string boss;
    std::string voucher;
    std::vector<std::string> tags;
    std::vector<ShopEntry> shop;
    std::vector<PackEntry> packs;
};

struct AnalysisConfig {
    std::string seed;
    int maxAnte = 8;
    std::vector<int> cardsPerAnte;
    std::string deck = "Red Deck";
    std::string stake = "White Stake";
    long version = 10106;
    bool unlockAll = true;
    std::vector<std::string> forcedLocks;
    std::vector<std::string> forcedUnlocks;
};

struct AnalysisResult {
    std::string seed;
    std::vector<AnteResult> antes;
};

struct SearchCriteria {
    std::vector<std::string> jokerNeedles;
    std::vector<std::string> textNeedles;
    std::vector<std::string> shopNeedles;
    std::vector<std::string> voucherNeedles;
    bool requireAll = true;
    int minMatches = 1;
    int maxAnte = 8;
    int minNegative = 0;
    int minPolychrome = 0;
    int minHolographic = 0;
    int minFoil = 0;
    std::vector<std::string> normalizedJokerNeedles;
    std::vector<std::string> normalizedTextNeedles;
    std::vector<std::string> normalizedShopNeedles;
    std::vector<std::string> normalizedVoucherNeedles;
};

struct MatchEvent {
    std::string name;
    std::string location;
    std::string packName;
    std::string edition;  // Edition of joker (Negative, Polychrome, Holographic, Foil, or empty)
    int ante = 0;
    int slot = -1;
    std::vector<std::string> details;
};

struct AnteSummary {
    int ante = 0;
    std::string boss;
    std::string voucher;
    std::vector<std::string> tags;
    std::vector<std::string> tagJokers;  // Jokers yielded by skip tags
};

struct SearchMatch {
    std::vector<MatchEvent> events;
    std::vector<MatchEvent> highlights;
    std::vector<AnteSummary> anteSummaries;  // Summary of all antes through 8
    int ante = 0;
    std::string boss;
    std::string voucher;
    std::vector<std::string> tags;
};

inline std::string cardToString(const Card& card);
inline std::vector<std::string> jokerModifiers(const JokerData& data);
inline std::string jokerRarityName(const std::string& rarity);
inline std::string describeJoker(const JokerData& data);
inline std::string normalizeToken(const std::string& value) {
    std::string result = value;
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });
    return result;
}

inline void normalizeSearchCriteria(SearchCriteria& criteria) {
    criteria.normalizedJokerNeedles.clear();
    criteria.normalizedTextNeedles.clear();
    criteria.normalizedShopNeedles.clear();
    criteria.normalizedVoucherNeedles.clear();
    criteria.normalizedJokerNeedles.reserve(criteria.jokerNeedles.size());
    criteria.normalizedTextNeedles.reserve(criteria.textNeedles.size());
    criteria.normalizedShopNeedles.reserve(criteria.shopNeedles.size());
    criteria.normalizedVoucherNeedles.reserve(criteria.voucherNeedles.size());
    for (const auto& needle : criteria.jokerNeedles) {
        criteria.normalizedJokerNeedles.push_back(normalizeToken(needle));
    }
    for (const auto& needle : criteria.textNeedles) {
        criteria.normalizedTextNeedles.push_back(normalizeToken(needle));
    }
    for (const auto& needle : criteria.shopNeedles) {
        criteria.normalizedShopNeedles.push_back(normalizeToken(needle));
    }
    for (const auto& needle : criteria.voucherNeedles) {
        criteria.normalizedVoucherNeedles.push_back(normalizeToken(needle));
    }

    if (criteria.minMatches < 1) {
        criteria.minMatches = 1;
    }
    const int totalNeedles = static_cast<int>(criteria.normalizedJokerNeedles.size() + criteria.normalizedTextNeedles.size() + criteria.normalizedShopNeedles.size() + criteria.normalizedVoucherNeedles.size());
    if (criteria.requireAll) {
        if (totalNeedles > 0) {
            criteria.minMatches = totalNeedles;
        }
    } else if (totalNeedles > 0 && criteria.minMatches > totalNeedles) {
        criteria.minMatches = totalNeedles;
    }
}

namespace detail {

inline std::string toRank(const std::string& code) {
    if (code == "T") return "10";
    if (code == "J") return "Jack";
    if (code == "Q") return "Queen";
    if (code == "K") return "King";
    if (code == "A") return "Ace";
    return code;
}

inline std::string toSuit(char suit) {
    switch (suit) {
    case 'C': return "Clubs";
    case 'S': return "Spades";
    case 'D': return "Diamonds";
    case 'H': return "Hearts";
    default: return std::string(1, suit);
    }
}

inline void applyLocks(Instance& inst, const AnalysisConfig& config) {
    inst.initLocks(1, false, false);
    for (const auto& voucher : kDefaultLockedVouchers) {
        inst.lock(voucher);
    }
    if (config.unlockAll) {
        for (const auto& name : kOptionalUnlocks) {
            inst.unlock(name);
        }
    }
    for (const auto& name : config.forcedLocks) {
        inst.lock(name);
    }
    for (const auto& name : config.forcedUnlocks) {
        inst.unlock(name);
    }
}

inline void handleVoucherUnlocks(Instance& inst, const std::string& voucher) {
    inst.lock(voucher);
    for (std::size_t i = 0; i + 1 < VOUCHERS.size(); i += 2) {
        if (VOUCHERS[i] == voucher) {
            inst.unlock(VOUCHERS[i + 1]);
            break;
        }
    }
}

inline std::vector<std::string> packContents(Instance& inst, const Pack& info, int ante) {
    std::vector<std::string> result;
    if (info.type == "Celestial Pack") {
        auto cards = inst.nextCelestialPack(info.size, ante);
        result.assign(cards.begin(), cards.end());
    } else if (info.type == "Arcana Pack") {
        auto cards = inst.nextArcanaPack(info.size, ante);
        result.assign(cards.begin(), cards.end());
    } else if (info.type == "Spectral Pack") {
        auto cards = inst.nextSpectralPack(info.size, ante);
        result.assign(cards.begin(), cards.end());
    } else if (info.type == "Buffoon Pack") {
        auto jokers = inst.nextBuffoonPack(info.size, ante);
        for (const auto& data : jokers) {
            std::ostringstream oss;
            auto modifiers = jokerModifiers(data);
            for (const auto& mod : modifiers) {
                oss << mod << ' ';
            }
            oss << data.joker;
            result.emplace_back(oss.str());
        }
    } else if (info.type == "Standard Pack") {
        auto cards = inst.nextStandardPack(info.size, ante);
        for (const auto& card : cards) {
            result.emplace_back(cardToString(card));
        }
    }
    return result;
}

} // namespace detail

inline AnalysisConfig makeDefaultConfig(const std::string& seed) {
    AnalysisConfig config;
    config.seed = seed;
    config.cardsPerAnte = {8, 12, 32, 48, 72, 96, 96, 96};
    return config;
}

inline std::string cardToString(const Card& card) {
    std::ostringstream oss;
    if (card.seal != "No Seal") {
        oss << card.seal << ' ';
    }
    if (card.edition != "No Edition") {
        oss << card.edition << ' ';
    }
    if (card.enhancement != "No Enhancement") {
        oss << card.enhancement << ' ';
    }
    if (card.base.size() < 3 || card.base[1] != '_') {
        oss << card.base;
        return oss.str();
    }
    std::string rank = detail::toRank(card.base.substr(2));
    std::string suit = detail::toSuit(card.base[0]);
    oss << rank << " of " << suit;
    return oss.str();
}

inline std::vector<std::string> jokerModifiers(const JokerData& data) {
    std::vector<std::string> mods;
    if (data.stickers.eternal) mods.emplace_back("Eternal");
    if (data.stickers.perishable) mods.emplace_back("Perishable");
    if (data.stickers.rental) mods.emplace_back("Rental");
    if (data.edition != "No Edition") mods.emplace_back(data.edition);
    return mods;
}

inline std::string jokerRarityName(const std::string& rarity) {
    if (rarity == "4" || rarity == "Legendary") return "Legendary";
    if (rarity == "3" || rarity == "Rare") return "Rare";
    if (rarity == "2" || rarity == "Uncommon") return "Uncommon";
    if (rarity == "1" || rarity == "Common") return "Common";
    return rarity.empty() ? "Unknown" : rarity;
}

inline std::string describeJoker(const JokerData& data) {
    std::ostringstream oss;
    oss << jokerRarityName(data.rarity) << ' ' << data.joker;
    auto mods = jokerModifiers(data);
    if (!mods.empty()) {
        oss << " [";
        for (std::size_t i = 0; i < mods.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << mods[i];
        }
        oss << ']';
    }
    return oss.str();
}

inline AnalysisResult runAnalysis(const AnalysisConfig& config) {
    AnalysisResult result;
    result.seed = config.seed;

    Instance inst(config.seed);
    inst.params = InstParams(config.deck, config.stake, false, config.version);

    detail::applyLocks(inst, config);
    inst.setStake(config.stake);
    inst.setDeck(config.deck);

    int maxAnte = config.maxAnte;
    std::vector<int> cardsPerAnte = config.cardsPerAnte;
    if (cardsPerAnte.size() < static_cast<std::size_t>(maxAnte)) {
        int fill = cardsPerAnte.empty() ? 0 : cardsPerAnte.back();
        cardsPerAnte.resize(maxAnte, fill);
    }

    for (int ante = 1; ante <= maxAnte; ++ante) {
        inst.initUnlocks(ante, false);

        AnteResult anteResult;
        anteResult.ante = ante;
        anteResult.boss = inst.nextBoss(ante);
        anteResult.voucher = inst.nextVoucher(ante);
        detail::handleVoucherUnlocks(inst, anteResult.voucher);
        anteResult.tags.push_back(inst.nextTag(ante));
        anteResult.tags.push_back(inst.nextTag(ante));

        int queueCount = (ante - 1 < static_cast<int>(cardsPerAnte.size())) ? cardsPerAnte[ante - 1] : 0;

        for (int idx = 1; idx <= queueCount; ++idx) {
            ShopItem item = inst.nextShopItem(ante);
            ShopEntry entry;
            entry.index = idx;
            entry.type = item.type;
            entry.display = item.item;
            if (item.type == "Joker") {
                entry.joker.name = item.item;
                entry.joker.modifiers = jokerModifiers(item.jokerData);
                std::ostringstream oss;
                for (const auto& mod : entry.joker.modifiers) {
                    oss << mod << ' ';
                }
                oss << item.item;
                entry.display = oss.str();
            }
            anteResult.shop.push_back(std::move(entry));
        }

        int numPacks = (ante == 1) ? 4 : 6;
        for (int p = 0; p < numPacks; ++p) {
            std::string packName = inst.nextPack(ante);
            Pack info = packInfo(packName);
            PackEntry packEntry;
            packEntry.name = packName;
            packEntry.contents = detail::packContents(inst, info, ante);
            anteResult.packs.push_back(std::move(packEntry));
        }

        result.antes.push_back(std::move(anteResult));
    }

    return result;
}

inline bool searchSeed(const AnalysisConfig& config, const SearchCriteria& criteria, SearchMatch& match) {
    match.events.clear();
    match.highlights.clear();
    match.anteSummaries.clear();
    match.ante = 0;
    match.boss.clear();
    match.voucher.clear();
    match.tags.clear();

    Instance inst(config.seed);
    inst.params = InstParams(config.deck, config.stake, false, config.version);

    detail::applyLocks(inst, config);
    inst.setStake(config.stake);
    inst.setDeck(config.deck);

    const int maxAnte = std::min(criteria.maxAnte, config.maxAnte);
    if (maxAnte <= 0) {
        return false;
    }
    thread_local std::vector<std::string> jokerNeedleScratch;
    thread_local std::vector<std::string> textNeedleScratch;
    thread_local std::vector<std::string> shopNeedleScratch;
    thread_local std::vector<std::string> voucherNeedleScratch;

    const std::vector<std::string>* jokerNeedlesPtr = &criteria.normalizedJokerNeedles;
    if (jokerNeedlesPtr->empty() && !criteria.jokerNeedles.empty()) {
        jokerNeedleScratch.clear();
        jokerNeedleScratch.reserve(criteria.jokerNeedles.size());
        for (const auto& needle : criteria.jokerNeedles) {
            jokerNeedleScratch.push_back(normalizeToken(needle));
        }
        jokerNeedlesPtr = &jokerNeedleScratch;
    }

    const std::vector<std::string>* textNeedlesPtr = &criteria.normalizedTextNeedles;
    if (textNeedlesPtr->empty() && !criteria.textNeedles.empty()) {
        textNeedleScratch.clear();
        textNeedleScratch.reserve(criteria.textNeedles.size());
        for (const auto& needle : criteria.textNeedles) {
            textNeedleScratch.push_back(normalizeToken(needle));
        }
        textNeedlesPtr = &textNeedleScratch;
    }

    const std::vector<std::string>* shopNeedlesPtr = &criteria.normalizedShopNeedles;
    if (shopNeedlesPtr->empty() && !criteria.shopNeedles.empty()) {
        shopNeedleScratch.clear();
        shopNeedleScratch.reserve(criteria.shopNeedles.size());
        for (const auto& needle : criteria.shopNeedles) {
            shopNeedleScratch.push_back(normalizeToken(needle));
        }
        shopNeedlesPtr = &shopNeedleScratch;
    }

    const std::vector<std::string>* voucherNeedlesPtr = &criteria.normalizedVoucherNeedles;
    if (voucherNeedlesPtr->empty() && !criteria.voucherNeedles.empty()) {
        voucherNeedleScratch.clear();
        voucherNeedleScratch.reserve(criteria.voucherNeedles.size());
        for (const auto& needle : criteria.voucherNeedles) {
            voucherNeedleScratch.push_back(normalizeToken(needle));
        }
        voucherNeedlesPtr = &voucherNeedleScratch;
    }

    const auto& jokerNeedles = *jokerNeedlesPtr;
    const auto& textNeedles = *textNeedlesPtr;
    const auto& shopNeedles = *shopNeedlesPtr;
    const auto& voucherNeedles = *voucherNeedlesPtr;

    std::vector<bool> jokerFound(jokerNeedles.size(), false);
    std::vector<bool> textFound(textNeedles.size(), false);
    std::vector<bool> shopFound(shopNeedles.size(), false);
    std::vector<bool> voucherFound(voucherNeedles.size(), false);
    const int totalNeedles = static_cast<int>(jokerNeedles.size() + textNeedles.size() + shopNeedles.size() + voucherNeedles.size());
    int satisfiedCount = 0;
    int jokerMatchCount = 0;  // Track how many joker needles have been matched
    int negativeCount = 0;
    int polychromeCount = 0;
    int holographicCount = 0;
    int foilCount = 0;
    const int threshold = [&]() {
        if (criteria.requireAll) {
            return totalNeedles;
        }
        if (totalNeedles == 0) {
            return 0;
        }
        int requested = criteria.minMatches;
        if (requested < 1) requested = 1;
        if (requested > totalNeedles) requested = totalNeedles;
        return requested;
    }();

    std::vector<MatchEvent> matchedEvents;
    std::vector<MatchEvent> matchedHighlights;
    std::unordered_set<std::string> highlightKeys;

    auto queueCountForAnte = [&](int ante) -> int {
        if (config.cardsPerAnte.empty()) return 0;
        std::size_t index = static_cast<std::size_t>(ante - 1);
        if (index < config.cardsPerAnte.size()) {
            return config.cardsPerAnte[index];
        }
        return config.cardsPerAnte.back();
    };

    auto requirementsSatisfied = [&]() {
        if (criteria.requireAll) {
            if (totalNeedles == 0) {
                return false;
            }
            for (bool hit : jokerFound) {
                if (!hit) return false;
            }
            for (bool hit : textFound) {
                if (!hit) return false;
            }
            for (bool hit : shopFound) {
                if (!hit) return false;
            }
            for (bool hit : voucherFound) {
                if (!hit) return false;
            }
        } else {
            if (threshold == 0) {
                return false;
            }
            if (satisfiedCount < threshold) {
                return false;
            }
        }

        // Check edition requirements
        // Edition counts track all joker matches (via --joker, --find, or --shop)
        if (criteria.minNegative > 0 && negativeCount < criteria.minNegative) return false;
        if (criteria.minPolychrome > 0 && polychromeCount < criteria.minPolychrome) return false;
        if (criteria.minHolographic > 0 && holographicCount < criteria.minHolographic) return false;
        if (criteria.minFoil > 0 && foilCount < criteria.minFoil) return false;

        return true;
    };

    auto recordEvent = [&](const std::string& name,
                           const std::string& location,
                           int ante,
                           int slot,
                           const std::string& packName,
                           const std::string& edition,
                           const std::vector<std::string>* details) {
        MatchEvent event;
        event.name = name;
        event.location = location;
        event.ante = ante;
        event.slot = slot;
        event.packName = packName;
        event.edition = edition;
        if (details && !details->empty()) {
            event.details = *details;
        }
        matchedEvents.push_back(std::move(event));
    };

    auto registerHighlight = [&](const std::string& name,
                                 const std::string& location,
                                 int anteValue,
                                 int slotValue,
                                 const std::string& packNameValue,
                                 const std::string& editionValue,
                                 const std::vector<std::string>& details) {
        std::ostringstream keyBuilder;
        keyBuilder << normalizeToken(name) << '|' << normalizeToken(location)
                   << '|' << anteValue << '|' << slotValue << '|' << normalizeToken(packNameValue);
        std::string key = keyBuilder.str();
        if (!highlightKeys.insert(key).second) {
            return;
        }
        MatchEvent event;
        event.name = name;
        event.location = location;
        event.ante = anteValue;
        event.slot = slotValue;
        event.packName = packNameValue;
        event.edition = editionValue;
        event.details = details;
    matchedHighlights.push_back(std::move(event));
    };

    auto considerText = [&](const std::string& candidate,
                            const std::string& location,
                            int ante,
                            int slot,
                            const std::string& packName,
                            const std::vector<std::string>* details = nullptr) -> bool {
        if (textNeedles.empty()) return false;

        std::string upper = normalizeToken(candidate);
        bool candidateMatched = false;

        auto evaluate = [&](const std::string& value) {
            std::string valueUpper = normalizeToken(value);
            bool hit = false;
            for (std::size_t i = 0; i < textNeedles.size(); ++i) {
                if (!textFound[i] && valueUpper.find(textNeedles[i]) != std::string::npos) {
                    textFound[i] = true;
                    ++satisfiedCount;

                    // Track if the main candidate matched (not just details)
                    if (value == candidate) {
                        candidateMatched = true;
                    }

                    hit = true;
                    if (!criteria.requireAll) break;
                }
            }
            return hit;
        };

        bool newHit = evaluate(candidate);
        if (details) {
            for (const auto& detail : *details) {
                if (evaluate(detail)) {
                    newHit = true;
                }
            }
        }

        // If the candidate itself matched and has edition keywords, track them
        // This handles --find matching joker names like "Perkeo" in "Negative Perkeo"
        if (candidateMatched) {
            if (upper.find("NEGATIVE") != std::string::npos) ++negativeCount;
            if (upper.find("POLYCHROME") != std::string::npos) ++polychromeCount;
            if (upper.find("HOLOGRAPHIC") != std::string::npos) ++holographicCount;
            if (upper.find("FOIL") != std::string::npos) ++foilCount;
        }

        if (newHit) {
            // Extract edition from candidate string
            std::string edition;
            std::string upperCand = normalizeToken(candidate);
            if (upperCand.find("NEGATIVE") != std::string::npos) edition = "Negative";
            else if (upperCand.find("POLYCHROME") != std::string::npos) edition = "Polychrome";
            else if (upperCand.find("HOLOGRAPHIC") != std::string::npos) edition = "Holographic";
            else if (upperCand.find("FOIL") != std::string::npos) edition = "Foil";
            recordEvent(candidate, location, ante, slot, packName, edition, details);
        }
        return newHit;
    };

    auto considerJoker = [&](const std::string& candidate,
                             const std::string& location,
                             int ante,
                             int slot,
                             const std::string& packName,
                             const std::vector<std::string>* details = nullptr) -> bool {
        if (jokerNeedles.empty()) return false;
        std::string upper = normalizeToken(candidate);
        bool newHit = false;

        for (std::size_t i = 0; i < jokerNeedles.size(); ++i) {
            if (jokerFound[i]) continue;

            // Check if needle matches candidate
            // If needle is "BLUEPRINT", match "BLUEPRINT" or "NEGATIVE BLUEPRINT"
            // If needle is "NEGATIVE BLUEPRINT", only match exact "NEGATIVE BLUEPRINT"
            bool matches = false;
            if (upper == jokerNeedles[i]) {
                // Exact match
                matches = true;
            } else if (upper.find(jokerNeedles[i]) != std::string::npos) {
                // Substring match only if the needle doesn't contain edition keywords
                // (so "Blueprint" matches "Negative Blueprint", but "Negative Blueprint" doesn't match "Blueprint")
                const std::string& needle = jokerNeedles[i];
                bool needleHasEdition = (needle.find("NEGATIVE") != std::string::npos ||
                                         needle.find("POLYCHROME") != std::string::npos ||
                                         needle.find("HOLOGRAPHIC") != std::string::npos ||
                                         needle.find("FOIL") != std::string::npos);
                if (!needleHasEdition) {
                    matches = true;
                }
            }

            if (matches) {
                jokerFound[i] = true;
                ++satisfiedCount;
                ++jokerMatchCount;

                // Track editions for this match
                if (upper.find("NEGATIVE") != std::string::npos) ++negativeCount;
                if (upper.find("POLYCHROME") != std::string::npos) ++polychromeCount;
                if (upper.find("HOLOGRAPHIC") != std::string::npos) ++holographicCount;
                if (upper.find("FOIL") != std::string::npos) ++foilCount;

                newHit = true;
                if (!criteria.requireAll) break;
            }
        }
        if (newHit) {
            // Extract edition from candidate string
            std::string edition;
            std::string upperCand = normalizeToken(candidate);
            if (upperCand.find("NEGATIVE") != std::string::npos) edition = "Negative";
            else if (upperCand.find("POLYCHROME") != std::string::npos) edition = "Polychrome";
            else if (upperCand.find("HOLOGRAPHIC") != std::string::npos) edition = "Holographic";
            else if (upperCand.find("FOIL") != std::string::npos) edition = "Foil";
            recordEvent(candidate, location, ante, slot, packName, edition, details);
        }
        return newHit;
    };

    auto considerShop = [&](const std::string& candidate,
                            const std::string& location,
                            int ante,
                            int slot,
                            const std::string& packName,
                            const std::vector<std::string>* details = nullptr) -> bool {
        if (shopNeedles.empty()) return false;

        std::string upper = normalizeToken(candidate);
        bool candidateMatched = false;

        auto evaluate = [&](const std::string& value) {
            std::string valueUpper = normalizeToken(value);
            bool hit = false;
            for (std::size_t i = 0; i < shopNeedles.size(); ++i) {
                if (!shopFound[i] && valueUpper.find(shopNeedles[i]) != std::string::npos) {
                    shopFound[i] = true;
                    ++satisfiedCount;

                    // Track if the main candidate matched (not just details)
                    if (value == candidate) {
                        candidateMatched = true;
                    }

                    hit = true;
                    if (!criteria.requireAll) break;
                }
            }
            return hit;
        };

        bool newHit = evaluate(candidate);
        if (details) {
            for (const auto& detail : *details) {
                if (evaluate(detail)) {
                    newHit = true;
                }
            }
        }

        // If the candidate itself matched and has edition keywords, track them
        if (candidateMatched) {
            if (upper.find("NEGATIVE") != std::string::npos) ++negativeCount;
            if (upper.find("POLYCHROME") != std::string::npos) ++polychromeCount;
            if (upper.find("HOLOGRAPHIC") != std::string::npos) ++holographicCount;
            if (upper.find("FOIL") != std::string::npos) ++foilCount;
        }

        if (newHit) {
            // Extract edition from candidate string
            std::string edition;
            std::string upperCand = normalizeToken(candidate);
            if (upperCand.find("NEGATIVE") != std::string::npos) edition = "Negative";
            else if (upperCand.find("POLYCHROME") != std::string::npos) edition = "Polychrome";
            else if (upperCand.find("HOLOGRAPHIC") != std::string::npos) edition = "Holographic";
            else if (upperCand.find("FOIL") != std::string::npos) edition = "Foil";
            recordEvent(candidate, location, ante, slot, packName, edition, details);
        }
        return newHit;
    };

    auto considerVoucher = [&](const std::string& candidate,
                               const std::string& location,
                               int ante,
                               int slot,
                               const std::string& packName,
                               const std::vector<std::string>* details = nullptr) -> bool {
        if (voucherNeedles.empty()) return false;
        std::string upper = normalizeToken(candidate);
        bool newHit = false;

        for (std::size_t i = 0; i < voucherNeedles.size(); ++i) {
            if (voucherFound[i]) continue;

            // Check if needle matches candidate (exact or substring)
            bool matches = false;
            if (upper == voucherNeedles[i]) {
                matches = true;
            } else if (upper.find(voucherNeedles[i]) != std::string::npos) {
                matches = true;
            }

            if (matches) {
                voucherFound[i] = true;
                ++satisfiedCount;
                newHit = true;
                if (!criteria.requireAll) break;
            }
        }

        if (newHit) {
            recordEvent(candidate, location, ante, slot, packName, "", details);
        }
        return newHit;
    };

    auto registerDetailHighlights = [&](const std::vector<std::string>& details,
                                        const std::string& location,
                                        int anteValue,
                                        int slotValue,
                                        const std::string& packNameValue) {
        for (const auto& detail : details) {
            std::string upper = normalizeToken(detail);
            if (upper.find("LEGENDARY") == std::string::npos &&
                upper.find("RARE") == std::string::npos) {
                continue;
            }
            std::string label = detail;
            auto colon = detail.find(':');
            if (colon != std::string::npos && colon + 1 < detail.size()) {
                label = detail.substr(colon + 1);
                auto first = label.find_first_not_of(' ');
                if (first != std::string::npos) {
                    label = label.substr(first);
                }
            }
            registerHighlight(label, location, anteValue, slotValue, packNameValue, "", {detail});
        }
    };

    auto predictSoul = [&](int ante) {
        Instance snapshot = inst;
        JokerData result = snapshot.nextJoker("sou", ante, false);
        return describeJoker(result);
    };

    auto predictJudgement = [&](int ante) {
        Instance snapshot = inst;
        JokerData result = snapshot.nextJoker("jud", ante, true);
        return describeJoker(result);
    };

    auto predictRareTag = [&](int ante) {
        Instance snapshot = inst;
        JokerData result = snapshot.nextJoker("rta", ante, true);
        return describeJoker(result);
    };

    auto predictUncommonTag = [&](int ante) {
        Instance snapshot = inst;
        JokerData result = snapshot.nextJoker("uta", ante, true);
        return describeJoker(result);
    };

    for (int ante = 1; ante <= maxAnte; ++ante) {
        bool anteMatched = false;

        inst.initUnlocks(ante, false);

        std::string boss = inst.nextBoss(ante);
        std::string voucher = inst.nextVoucher(ante);
        detail::handleVoucherUnlocks(inst, voucher);

        std::vector<std::string> anteTags;
        anteTags.push_back(inst.nextTag(ante));
        anteTags.push_back(inst.nextTag(ante));

        auto updateMatchFlag = [&]() {
            if (requirementsSatisfied()) {
                anteMatched = true;
            }
        };

        if (considerText(boss, "Boss", ante, -1, "")) {
            updateMatchFlag();
        }
        if (considerVoucher(voucher, "Voucher", ante, -1, "")) {
            updateMatchFlag();
        }
        if (considerText(voucher, "Voucher", ante, -1, "")) {
            updateMatchFlag();
        }
        for (std::size_t tagIndex = 0; tagIndex < anteTags.size(); ++tagIndex) {
            const std::string& tag = anteTags[tagIndex];
            std::vector<std::string> tagDetails;
            std::string jokerYield;

            // Predict jokers yielded by skip tags
            // Second skip (tagIndex == 1) advances to next ante
            int skipAnte = (tagIndex == 1) ? ante + 1 : ante;

            if (tag == "Rare Tag") {
                jokerYield = predictRareTag(skipAnte);
                tagDetails.emplace_back("Yields: " + jokerYield);
            } else if (tag == "Uncommon Tag") {
                jokerYield = predictUncommonTag(skipAnte);
                tagDetails.emplace_back("Yields: " + jokerYield);
            }

            const std::vector<std::string>* tagDetailsPtr = tagDetails.empty() ? nullptr : &tagDetails;

            // Check if the yielded joker matches search criteria
            if (!jokerYield.empty()) {
                if (considerJoker(jokerYield, "Tag", ante, static_cast<int>(tagIndex) + 1, "", tagDetailsPtr)) {
                    updateMatchFlag();
                }
            }

            // Also check if the tag name itself matches text search
            if (considerText(tag, "Tag", ante, static_cast<int>(tagIndex) + 1, "", tagDetailsPtr)) {
                updateMatchFlag();
            }

            if (tagDetailsPtr) {
                registerDetailHighlights(*tagDetailsPtr, "Tag Detail", ante, static_cast<int>(tagIndex) + 1, "");
            }
        }

        int queueCount = queueCountForAnte(ante);
        for (int idx = 1; idx <= queueCount; ++idx) {
            ShopItem item = inst.nextShopItem(ante);
            std::string display = item.item;
            std::string location = "Shop";
            std::vector<std::string> extraDetails;
            std::string jokerYield;

            if (item.type == "Spectral" && item.item == "The Soul") {
                jokerYield = predictSoul(ante);
                extraDetails.emplace_back("Yields: " + jokerYield);
            } else if (item.type == "Tarot" && item.item == "Judgement") {
                jokerYield = predictJudgement(ante);
                extraDetails.emplace_back("Yields: " + jokerYield);
            }
            const std::vector<std::string>* detailsPtr = extraDetails.empty() ? nullptr : &extraDetails;

            if (item.type == "Joker") {
                auto mods = jokerModifiers(item.jokerData);
                std::ostringstream oss;
                for (const auto& mod : mods) {
                    oss << mod << ' ';
                }
                oss << item.item;
                display = oss.str();

                if (considerJoker(display, location, ante, idx, "", detailsPtr)) {
                    updateMatchFlag();
                }

                std::string rarityName = jokerRarityName(item.jokerData.rarity);
                bool isRareOrLegendary = (rarityName == "Rare" || rarityName == "Legendary");
                bool isNegative = (item.jokerData.edition == "Negative");

                if (isRareOrLegendary || isNegative) {
                    std::vector<std::string> highlightDetails;
                    if (isRareOrLegendary) {
                        highlightDetails.push_back("Rarity: " + rarityName);
                    }
                    if (isNegative) {
                        highlightDetails.push_back("Edition: Negative");
                    }
                    if (detailsPtr) {
                        highlightDetails.insert(highlightDetails.end(), detailsPtr->begin(), detailsPtr->end());
                    }
                    std::string edition = (item.jokerData.edition != "No Edition") ? item.jokerData.edition : "";
                    registerHighlight(describeJoker(item.jokerData), location, ante, idx, "", edition, highlightDetails);
                }
            }

            // Check if the yielded joker matches search criteria (for Soul/Judgement)
            if (!jokerYield.empty()) {
                if (considerJoker(jokerYield, location, ante, idx, "", detailsPtr)) {
                    updateMatchFlag();
                }
            }

            // Check the item against --shop needles
            if (considerShop(display, location, ante, idx, "", detailsPtr)) {
                updateMatchFlag();
            }

            // Also check the item name itself against --find
            if (considerText(display, location, ante, idx, "", detailsPtr)) {
                updateMatchFlag();
            }
            if (detailsPtr) {
                registerDetailHighlights(*detailsPtr, location + " Detail", ante, idx, "");
            }
        }

        int numPacks = (ante == 1) ? 4 : 6;
        for (int p = 0; p < numPacks; ++p) {
            std::string packName = inst.nextPack(ante);
            if (considerText(packName, "Pack", ante, p + 1, "")) {
                updateMatchFlag();
            }

            Pack info = packInfo(packName);
            if (info.type == "Buffoon Pack") {
                auto jokers = inst.nextBuffoonPack(info.size, ante);
                int entrySlot = 1;
                for (const auto& data : jokers) {
                    auto mods = jokerModifiers(data);
                    std::ostringstream oss;
                    for (const auto& mod : mods) {
                        oss << mod << ' ';
                    }
                    oss << data.joker;
                    std::string display = oss.str();

                    if (considerText(display, "Pack Card", ante, entrySlot, packName)) {
                        updateMatchFlag();
                    }
                    if (considerJoker(display, "Pack Card", ante, entrySlot, packName)) {
                        updateMatchFlag();
                    }

                    std::string rarityName = jokerRarityName(data.rarity);
                    bool isRareOrLegendary = (rarityName == "Rare" || rarityName == "Legendary");
                    bool isNegative = (data.edition == "Negative");

                    if (isRareOrLegendary || isNegative) {
                        std::vector<std::string> highlightDetails;
                        if (isRareOrLegendary) {
                            highlightDetails.push_back("Rarity: " + rarityName);
                        }
                        if (isNegative) {
                            highlightDetails.push_back("Edition: Negative");
                        }
                        std::string edition = (data.edition != "No Edition") ? data.edition : "";
                        registerHighlight(describeJoker(data), "Pack Card", ante, entrySlot, packName, edition, highlightDetails);
                    }

                    ++entrySlot;
                }
            } else {
                auto contents = detail::packContents(inst, info, ante);
                int entrySlot = 1;
                for (const auto& entry : contents) {
                    std::vector<std::string> extraDetails;
                    std::string jokerYield;

                    if (entry == "The Soul") {
                        jokerYield = predictSoul(ante);
                        extraDetails.emplace_back("Yields: " + jokerYield);
                    } else if (entry == "Judgement") {
                        jokerYield = predictJudgement(ante);
                        extraDetails.emplace_back("Yields: " + jokerYield);
                    }
                    const std::vector<std::string>* cardDetails = extraDetails.empty() ? nullptr : &extraDetails;

                    // Check if the yielded joker matches search criteria
                    if (!jokerYield.empty()) {
                        if (considerJoker(jokerYield, "Pack Card", ante, entrySlot, packName, cardDetails)) {
                            updateMatchFlag();
                        }
                    }

                    // Also check the card name itself
                    if (considerText(entry, "Pack Card", ante, entrySlot, packName, cardDetails)) {
                        updateMatchFlag();
                    }
                    if (considerJoker(entry, "Pack Card", ante, entrySlot, packName, cardDetails)) {
                        updateMatchFlag();
                    }
                    if (cardDetails) {
                        registerDetailHighlights(*cardDetails, "Pack Detail", ante, entrySlot, packName);
                    }
                    ++entrySlot;
                }
            }
        }

        if (anteMatched || requirementsSatisfied()) {
            // Found a match! Save the match details but continue through ante 8 for highlights and summaries
            match.ante = ante;
            match.boss = boss;
            match.voucher = voucher;
            match.tags = anteTags;
            match.events = std::move(matchedEvents);

            // Continue processing remaining antes (up to 8) to collect highlights
            for (int highlightAnte = ante + 1; highlightAnte <= 8; ++highlightAnte) {
                inst.initUnlocks(highlightAnte, false);

                std::string highlightBoss = inst.nextBoss(highlightAnte);
                std::string highlightVoucher = inst.nextVoucher(highlightAnte);
                detail::handleVoucherUnlocks(inst, highlightVoucher);

                // Process tags for predictions
                std::vector<std::string> highlightTags;
                highlightTags.push_back(inst.nextTag(highlightAnte));
                highlightTags.push_back(inst.nextTag(highlightAnte));

                for (std::size_t tagIndex = 0; tagIndex < highlightTags.size(); ++tagIndex) {
                    const std::string& tag = highlightTags[tagIndex];
                    std::vector<std::string> tagDetails;

                    // Second skip (tagIndex == 1) advances to next ante
                    int skipAnte = (tagIndex == 1) ? highlightAnte + 1 : highlightAnte;

                    if (tag == "Rare Tag") {
                        std::string joker = predictRareTag(skipAnte);
                        tagDetails.emplace_back("Yields: " + joker);
                    } else if (tag == "Uncommon Tag") {
                        std::string joker = predictUncommonTag(skipAnte);
                        tagDetails.emplace_back("Yields: " + joker);
                    }

                    if (!tagDetails.empty()) {
                        registerDetailHighlights(tagDetails, "Tag Detail", highlightAnte, static_cast<int>(tagIndex) + 1, "");
                    }
                }

                int highlightQueueCount = queueCountForAnte(highlightAnte);
                for (int idx = 1; idx <= highlightQueueCount; ++idx) {
                    ShopItem item = inst.nextShopItem(highlightAnte);
                    std::vector<std::string> extraDetails;

                    if (item.type == "Spectral" && item.item == "The Soul") {
                        extraDetails.emplace_back("Yields: " + predictSoul(highlightAnte));
                        registerDetailHighlights(extraDetails, "Shop Detail", highlightAnte, idx, "");
                    } else if (item.type == "Tarot" && item.item == "Judgement") {
                        extraDetails.emplace_back("Yields: " + predictJudgement(highlightAnte));
                        registerDetailHighlights(extraDetails, "Shop Detail", highlightAnte, idx, "");
                    } else if (item.type == "Joker") {
                        std::string rarityName = jokerRarityName(item.jokerData.rarity);
                        bool isRareOrLegendary = (rarityName == "Rare" || rarityName == "Legendary");
                        bool isNegative = (item.jokerData.edition == "Negative");

                        if (isRareOrLegendary || isNegative) {
                            std::vector<std::string> highlightDetails;
                            if (isRareOrLegendary) {
                                highlightDetails.push_back("Rarity: " + rarityName);
                            }
                            if (isNegative) {
                                highlightDetails.push_back("Edition: Negative");
                            }
                            std::string edition = (item.jokerData.edition != "No Edition") ? item.jokerData.edition : "";
                            registerHighlight(describeJoker(item.jokerData), "Shop", highlightAnte, idx, "", edition, highlightDetails);
                        }
                    }
                }

                int numPacks = (highlightAnte == 1) ? 4 : 6;
                for (int p = 0; p < numPacks; ++p) {
                    std::string packName = inst.nextPack(highlightAnte);
                    Pack info = packInfo(packName);

                    if (info.type == "Buffoon Pack") {
                        auto jokers = inst.nextBuffoonPack(info.size, highlightAnte);
                        int entrySlot = 1;
                        for (const auto& data : jokers) {
                            std::string rarityName = jokerRarityName(data.rarity);
                            bool isRareOrLegendary = (rarityName == "Rare" || rarityName == "Legendary");
                            bool isNegative = (data.edition == "Negative");

                            if (isRareOrLegendary || isNegative) {
                                std::vector<std::string> highlightDetails;
                                if (isRareOrLegendary) {
                                    highlightDetails.push_back("Rarity: " + rarityName);
                                }
                                if (isNegative) {
                                    highlightDetails.push_back("Edition: Negative");
                                }
                                std::string edition = (data.edition != "No Edition") ? data.edition : "";
                                registerHighlight(describeJoker(data), "Pack Card", highlightAnte, entrySlot, packName, edition, highlightDetails);
                            }
                            ++entrySlot;
                        }
                    } else {
                        auto contents = detail::packContents(inst, info, highlightAnte);
                        int entrySlot = 1;
                        for (const auto& entry : contents) {
                            std::vector<std::string> cardDetails;

                            if (entry == "The Soul") {
                                cardDetails.emplace_back("Yields: " + predictSoul(highlightAnte));
                                registerDetailHighlights(cardDetails, "Pack Detail", highlightAnte, entrySlot, packName);
                            } else if (entry == "Judgement") {
                                cardDetails.emplace_back("Yields: " + predictJudgement(highlightAnte));
                                registerDetailHighlights(cardDetails, "Pack Detail", highlightAnte, entrySlot, packName);
                            }
                            ++entrySlot;
                        }
                    }
                }
            }

            // Build ante summaries for antes 1 through 8 using a fresh instance
            Instance summaryInst(config.seed);
            summaryInst.params = InstParams(config.deck, config.stake, false, config.version);
            detail::applyLocks(summaryInst, config);
            summaryInst.setStake(config.stake);
            summaryInst.setDeck(config.deck);

            // Always show summaries through ante 8 regardless of search limit
            for (int a = 1; a <= 8; ++a) {
                summaryInst.initUnlocks(a, false);

                AnteSummary summary;
                summary.ante = a;
                summary.boss = summaryInst.nextBoss(a);
                summary.voucher = summaryInst.nextVoucher(a);
                detail::handleVoucherUnlocks(summaryInst, summary.voucher);

                summary.tags.push_back(summaryInst.nextTag(a));
                summary.tags.push_back(summaryInst.nextTag(a));

                // Predict jokers from skip tags
                for (std::size_t tagIndex = 0; tagIndex < summary.tags.size(); ++tagIndex) {
                    const std::string& tag = summary.tags[tagIndex];
                    int skipAnte = (tagIndex == 1) ? a + 1 : a;

                    if (tag == "Rare Tag") {
                        summary.tagJokers.push_back(predictRareTag(skipAnte));
                    } else if (tag == "Uncommon Tag") {
                        summary.tagJokers.push_back(predictUncommonTag(skipAnte));
                    }
                }

                match.anteSummaries.push_back(std::move(summary));
            }

            match.highlights = std::move(matchedHighlights);
            return true;
        }
    }

    return false;
}

} // namespace analysis
