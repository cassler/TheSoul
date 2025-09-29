#pragma once

#include "../include/immolate.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>

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

inline std::string cardToString(const Card& card);
inline std::vector<std::string> jokerModifiers(const JokerData& data);

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
    config.cardsPerAnte = {15, 50, 50, 50, 50, 50, 50, 50};
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

} // namespace analysis
