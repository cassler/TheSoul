#include "analysis.hpp"

#include <algorithm>
#include <cctype>
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

} // namespace

int main(int argc, char** argv) {
    std::string rawSeed = (argc > 1) ? argv[1] : "ABCD";
    std::string seed = filterSeed(rawSeed);
    if (seed.empty()) {
        std::cerr << "Seed must contain at least one alphanumeric character." << std::endl;
        return 1;
    }

    analysis::AnalysisConfig config = analysis::makeDefaultConfig(seed);
    config.maxAnte = 1;
    auto result = analysis::runAnalysis(config);
    if (result.antes.empty()) {
        std::cerr << "No analysis data produced." << std::endl;
        return 1;
    }

    const auto& ante = result.antes.front();
    std::cout << "==ANTE " << ante.ante << "==\n";
    std::cout << "Boss: " << ante.boss << "\n";
    std::cout << "Voucher: " << ante.voucher << "\n";
    std::cout << "Tags: " << ante.tags[0] << ", " << ante.tags[1] << "\n\n";

    std::cout << "Shop Queue: \n";
    for (const auto& item : ante.shop) {
        std::cout << item.index << ") " << item.display << "\n";
    }

    return 0;
}
