#include "../include/immolate.hpp"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace {

std::string filterSeed(const std::string& input) {
    std::string filtered;
    filtered.reserve(8);
    for (char ch : input) {
        if (!std::isalnum(static_cast<unsigned char>(ch))) {
            continue;
        }
        char upper = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
        if (upper == '0') {
            upper = 'O';
        }
        filtered.push_back(upper);
        if (filtered.size() == 8) {
            break;
        }
    }
    return filtered;
}

const std::vector<std::string> kDefaultLockedVouchers = {
    "Overstock Plus", "Liquidation", "Glow Up",      "Reroll Glut",
    "Omen Globe",     "Observatory", "Nacho Tong",   "Recyclomancy",
    "Tarot Tycoon",   "Planet Tycoon", "Money Tree", "Antimatter",
    "Illusion",       "Petroglyph",  "Retcon",      "Palette"
};

const std::vector<std::string> kOptionalUnlocks = {
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

void runSingleAnteAnalysis(const std::string& seed) {
    Instance inst(seed);
    inst.params = InstParams("Red Deck", "White Stake", false, 10106);
    inst.initLocks(1, false, false);

    for (const auto& voucher : kDefaultLockedVouchers) {
        inst.lock(voucher);
    }

    for (const auto& item : kOptionalUnlocks) {
        inst.unlock(item);
    }

    inst.setStake("White Stake");
    inst.setDeck("Red Deck");

    std::cout << "==ANTE 1==\n";

    inst.initUnlocks(1, false);

    std::string boss = inst.nextBoss(1);
    std::cout << "Boss: " << boss << "\n";

    std::string voucher = inst.nextVoucher(1);
    std::cout << "Voucher: " << voucher << "\n";

    inst.lock(voucher);
    for (std::size_t i = 0; i + 1 < VOUCHERS.size(); i += 2) {
        if (VOUCHERS[i] == voucher) {
            inst.unlock(VOUCHERS[i + 1]);
            break;
        }
    }

    std::string tag1 = inst.nextTag(1);
    std::string tag2 = inst.nextTag(1);
    std::cout << "Tags: " << tag1 << ", " << tag2 << "\n\n";

    std::cout << "Shop Queue: \n";
    for (int i = 1; i <= 15; ++i) {
        ShopItem item = inst.nextShopItem(1);
        std::cout << i << ") ";
        if (item.type == "Joker") {
            if (item.jokerData.stickers.eternal) {
                std::cout << "Eternal ";
            }
            if (item.jokerData.stickers.perishable) {
                std::cout << "Perishable ";
            }
            if (item.jokerData.stickers.rental) {
                std::cout << "Rental ";
            }
            if (item.jokerData.edition != "No Edition") {
                std::cout << item.jokerData.edition << ' ';
            }
        }
        std::cout << item.item << "\n";
    }
}

} // namespace

int main(int argc, char** argv) {
    std::string rawSeed = (argc > 1) ? argv[1] : "ABCD";
    std::string seed = filterSeed(rawSeed);
    if (seed.empty()) {
        std::cerr << "Seed must contain at least one alphanumeric character." << std::endl;
        return 1;
    }

    runSingleAnteAnalysis(seed);
    return 0;
}
