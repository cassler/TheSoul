#ifndef SEARCH_HPP
#define SEARCH_HPP

#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <thread>
#include <mutex>
#include <atomic>
#include "instance.hpp"
#include "items.hpp"

// Search result structures
struct MatchDetail {
    std::string type;    // "Joker", "Voucher", "Boss", "Tag", "Pack", "Item"
    std::string name;    // The item name
    int ante;           // Which ante it appears in
    int slot;           // Shop slot position (0 for non-shop items)
    std::string extra;   // Extra info (e.g., pack name for pack cards)
};

struct SeedMatch {
    std::string seed;
    std::vector<MatchDetail> details;
};

struct SearchOptions {
    std::vector<std::string> findTerms;
    std::vector<std::string> jokerTerms;
    std::vector<std::string> voucherTerms;
    std::vector<std::string> bossTerms;
    std::vector<std::string> tagTerms;

    std::string deck = "Red Deck";
    std::string stake = "White Stake";
    int maxSeeds = 5000;
    int maxAnte = 8;
    int earlyExit = -1;  // Stop searching after this ante (-1 = no early exit)
    bool matchAll = false;  // true = AND, false = OR
    bool stopOnFirst = false;
    bool unlimited = false;  // Keep searching indefinitely
    int threads = 1;  // Number of threads to use (default: single-threaded)

    // Shop limits per ante (default: 20,40,60,75,75,75,75,75)
    std::vector<int> shopLimits = {20, 40, 60, 75, 75, 75, 75, 75};
};

// Utility function for case-insensitive string search
inline bool containsIgnoreCase(const std::string& haystack, const std::string& needle) {
    if (needle.empty()) return true;

    auto it = std::search(
        haystack.begin(), haystack.end(),
        needle.begin(), needle.end(),
        [](char ch1, char ch2) { return std::toupper(ch1) == std::toupper(ch2); }
    );
    return it != haystack.end();
}

// Main search function
std::vector<SeedMatch> searchSeeds(const SearchOptions& options);

// Single seed analyzer
SeedMatch analyzeSeedForSearch(const std::string& seed, const SearchOptions& options);

// Random seed generator
std::string generateRandomSeed();

// Get current seed count for progress tracking
int getSeedsChecked();

#endif // SEARCH_HPP