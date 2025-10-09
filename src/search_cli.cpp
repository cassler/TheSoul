#include "analysis.hpp"

#include <algorithm>
#include <atomic>
#include <array>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace {

constexpr int kSeedLength = 8;
constexpr int kAlphabetSize = 35; // 26 letters + digits 1-9

constexpr std::uint64_t kTotalSeeds = []() constexpr {
    std::uint64_t value = 1;
    for (int i = 0; i < kSeedLength; ++i) {
        value *= kAlphabetSize;
    }
    return value;
}();

enum class ColorMode {
    Auto,
    Always,
    Never
};

enum class RowTone {
    Header,
    Normal,
    Rare,
    Legendary
};

constexpr const char* kAnsiReset = "\033[0m";
constexpr const char* kAnsiBold = "\033[1m";
constexpr const char* kAnsiDim = "\033[2m";
constexpr const char* kAnsiHeader = "\033[97;1m";
constexpr const char* kAnsiPrimary = "\033[96;1m";
constexpr const char* kAnsiAccent = "\033[94;1m";
constexpr const char* kAnsiRare = "\033[96m";
constexpr const char* kAnsiLegendary = "\033[93;1m";
constexpr const char* kAnsiMuted = "\033[90m";
constexpr const char* kAnsiHighlight = "\033[95;1m";

bool gColorEnabled = false;

#ifdef _WIN32
bool enableVirtualTerminalProcessing() {
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (handle == INVALID_HANDLE_VALUE) {
        return false;
    }
    DWORD mode = 0;
    if (!GetConsoleMode(handle, &mode)) {
        return false;
    }
    if (mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) {
        return true;
    }
    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    return SetConsoleMode(handle, mode) != 0;
}

bool stdoutIsTerminal() {
    return _isatty(_fileno(stdout)) != 0;
}
#else
bool stdoutIsTerminal() {
    return isatty(STDOUT_FILENO) != 0;
}
#endif

bool detectAutoColorSupport() {
    const char* forceColor = std::getenv("FORCE_COLOR");
    if (forceColor && forceColor[0] != '\0') {
        return std::string(forceColor) != "0";
    }
    if (std::getenv("NO_COLOR")) {
        return false;
    }
    if (!stdoutIsTerminal()) {
        return false;
    }
#ifdef _WIN32
    return enableVirtualTerminalProcessing();
#else
    const char* term = std::getenv("TERM");
    if (!term) {
        return false;
    }
    std::string value(term);
    if (value == "dumb") {
        return false;
    }
    return true;
#endif
}

bool computeColorEnabled(ColorMode mode) {
    switch (mode) {
    case ColorMode::Always:
        return true;
    case ColorMode::Never:
        return false;
    case ColorMode::Auto:
    default:
        return detectAutoColorSupport();
    }
}

std::string styleIf(bool enabled, const char* code, const std::string& text) {
    if (!enabled || !code || !*code) {
        return text;
    }
    return std::string(code) + text + kAnsiReset;
}

char digitToChar(int value) {
    if (value < 26) {
        return static_cast<char>('A' + value);
    }
    return static_cast<char>('1' + (value - 26));
}

int charToDigit(char ch) {
    if (ch >= 'A' && ch <= 'Z') {
        return ch - 'A';
    }
    if (ch >= '1' && ch <= '9') {
        return 26 + (ch - '1');
    }
    return -1;
}

std::string normalizeSeed(const std::string& input) {
    std::string filtered;
    filtered.reserve(kSeedLength);
    for (char ch : input) {
        if (!std::isalnum(static_cast<unsigned char>(ch))) {
            continue;
        }
        char upper = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
        if (upper == '0') {
            upper = 'O';
        }
        filtered.push_back(upper);
        if (filtered.size() == kSeedLength) {
            break;
        }
    }
    return filtered;
}

bool seedToNumber(const std::string& seed, std::uint64_t& valueOut) {
    if (seed.size() != kSeedLength) {
        return false;
    }
    std::uint64_t value = 0;
    for (char ch : seed) {
        int digit = charToDigit(ch);
        if (digit < 0) {
            return false;
        }
        value = value * kAlphabetSize + static_cast<std::uint64_t>(digit);
    }
    valueOut = value;
    return true;
}

std::string numberToSeed(std::uint64_t value) {
    std::string seed(kSeedLength, 'A');
    for (int i = kSeedLength - 1; i >= 0; --i) {
        int digit = static_cast<int>(value % kAlphabetSize);
        seed[i] = digitToChar(digit);
        value /= kAlphabetSize;
    }
    return seed;
}

struct Options {
    std::vector<std::string> explicitSeeds;
    bool hasRange = false;
    std::uint64_t rangeStart = 0;
    std::uint64_t rangeCount = 0;
    int limit = 1;
    int threadCount = 1;
    int early = 8;
    bool showProgress = true;
    bool wrapRange = false;
    bool randomizeStart = false;
    analysis::SearchCriteria criteria;
    analysis::AnalysisConfig baseConfig = analysis::makeDefaultConfig("AAAAAAAA");
    ColorMode colorMode = ColorMode::Auto;
    bool colorize = false;
};

struct TableRow {
    std::array<std::string, 6> cells{};
    RowTone tone = RowTone::Normal;
};

const char* toneColor(RowTone tone) {
    switch (tone) {
    case RowTone::Header:
        return kAnsiHeader;
    case RowTone::Rare:
        return kAnsiRare;
    case RowTone::Legendary:
        return kAnsiLegendary;
    case RowTone::Normal:
    default:
        return nullptr;
    }
}

std::string joinWith(const std::vector<std::string>& values, const std::string& delimiter) {
    if (values.empty()) {
        return "-";
    }
    std::ostringstream oss;
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i > 0) {
            oss << delimiter;
        }
        oss << values[i];
    }
    return oss.str();
}

std::string joinDetails(const std::vector<std::string>& details) {
    if (details.empty()) {
        return "-";
    }
    std::ostringstream oss;
    for (std::size_t i = 0; i < details.size(); ++i) {
        if (i > 0) oss << "; ";
        oss << details[i];
    }
    return oss.str();
}

std::string repeatGlyph(const std::string& glyph, std::size_t count) {
    if (glyph.empty() || count == 0) {
        return "";
    }
    std::string result;
    result.reserve(glyph.size() * count);
    for (std::size_t i = 0; i < count; ++i) {
        result += glyph;
    }
    return result;
}

RowTone toneFromText(const std::string& text) {
    if (text.empty()) {
        return RowTone::Normal;
    }
    std::string upper = analysis::normalizeToken(text);
    if (upper.find("LEGENDARY") != std::string::npos) {
        return RowTone::Legendary;
    }
    if (upper.find("RARE") != std::string::npos) {
        return RowTone::Rare;
    }
    return RowTone::Normal;
}

RowTone toneForEvent(const analysis::MatchEvent& event) {
    if (!event.details.empty()) {
        for (const auto& detail : event.details) {
            RowTone tone = toneFromText(detail);
            if (tone == RowTone::Legendary) return tone;
        }
        for (const auto& detail : event.details) {
            RowTone tone = toneFromText(detail);
            if (tone == RowTone::Rare) return tone;
        }
    }
    RowTone nameTone = toneFromText(event.name);
    if (nameTone != RowTone::Normal) {
        return nameTone;
    }
    RowTone packTone = toneFromText(event.packName);
    if (packTone != RowTone::Normal) {
        return packTone;
    }
    return RowTone::Normal;
}

std::string padCell(const std::string& value, std::size_t width) {
    std::ostringstream oss;
    oss << std::left << std::setw(static_cast<int>(width)) << value;
    return oss.str();
}

void printTableSection(const std::array<std::size_t, 6>& widths,
                       const std::vector<TableRow>& rows,
                       const std::string& indent) {
    if (rows.empty()) {
        return;
    }

    auto emitRow = [&](const TableRow& row) {
        std::cout << indent;
        for (std::size_t i = 0; i < row.cells.size(); ++i) {
            std::string cell = padCell(row.cells[i], widths[i]);
            std::cout << styleIf(gColorEnabled, toneColor(row.tone), cell);
            if (i + 1 < row.cells.size()) {
                std::cout << "  ";
            }
        }
        std::cout << "\n";
    };

    TableRow header;
    header.cells = {"Name", "Location", "Ante", "Slot", "Pack", "Details"};
    header.tone = RowTone::Header;
    emitRow(header);

    std::string divider;
    for (std::size_t i = 0; i < widths.size(); ++i) {
        if (i > 0) divider += "  ";
        divider.append(widths[i], '-');
    }
    std::cout << indent << styleIf(gColorEnabled, kAnsiMuted, divider) << "\n";

    for (const auto& row : rows) {
        emitRow(row);
    }
}

void printHeadlineBox(const std::vector<std::string>& lines) {
    if (lines.empty()) {
        return;
    }
    std::size_t width = 0;
    for (const auto& line : lines) {
        width = std::max(width, line.size());
    }

    std::string top = "╭" + repeatGlyph("─", width + 2) + "╮";
    std::string bottom = "╰" + repeatGlyph("─", width + 2) + "╯";
    std::cout << styleIf(gColorEnabled, kAnsiPrimary, top) << "\n";
    for (std::size_t i = 0; i < lines.size(); ++i) {
        std::string padded = lines[i];
        if (padded.size() < width) {
            padded.append(width - padded.size(), ' ');
        }
        std::string textColor = (i == 0) ? std::string(kAnsiHeader) : std::string(kAnsiAccent);
        if (!gColorEnabled) {
            textColor.clear();
        }
        std::string left = styleIf(gColorEnabled, kAnsiPrimary, "│ ");
        std::string content = textColor.empty() ? padded : textColor + padded + kAnsiReset;
        std::string right = styleIf(gColorEnabled, kAnsiPrimary, " │");
        std::cout << left << content << right << "\n";
    }
    std::cout << styleIf(gColorEnabled, kAnsiPrimary, bottom) << "\n";
}

void printUsage() {
    std::cerr << "Usage: search_cli [options]\n"
              << "  --seed SEED            Search a specific seed (may repeat)\n"
              << "  --start SEED           Starting seed for sequential search (requires --count)\n"
              << "  --count N              Number of sequential seeds to examine\n"
              << "  --joker NAME           Require a joker match (case-insensitive exact)\n"
              << "  --find TEXT            Require substring match (case-insensitive)\n"
              << "  --any N               Require at least N --joker/--find matches (default 1)\n"
              << "  --all                  Require all --joker/--find (default)\n"
              << "  --negative N, --neg N  Require N matched jokers with Negative edition\n"
              << "  --poly N              Require N matched jokers with Polychrome edition\n"
              << "  --holo N              Require N matched jokers with Holographic edition\n"
              << "  --foil N              Require N matched jokers with Foil edition\n"
              << "  --early N              Only check antes 1..N (default 8)\n"
              << "  --limit N              Stop after N matches (default 1)\n"
              << "  --threads N            Worker threads for sequential search\n"
              << "  --deck NAME            Deck name (default Red Deck)\n"
              << "  --stake NAME           Stake name (default White Stake)\n"
              << "  --version N            Game version (default 10106)\n"
              << "  --progress             Show periodic progress updates (default)\n"
              << "  --no-progress          Suppress progress output\n"
              << "  --color               Force colored output\n"
              << "  --no-color            Disable colored output\n";
}

bool parsePositiveInt(const std::string& text, int& valueOut) {
    try {
        int value = std::stoi(text);
        if (value <= 0) {
            return false;
        }
        valueOut = value;
        return true;
    } catch (...) {
        return false;
    }
}

bool parsePositiveUInt64(const std::string& text, std::uint64_t& valueOut) {
    try {
        std::uint64_t value = std::stoull(text);
        if (value == 0) {
            return false;
        }
        valueOut = value;
        return true;
    } catch (...) {
        return false;
    }
}

bool parseArguments(int argc, char** argv, Options& opts) {
    opts.threadCount = static_cast<int>(std::max(1u, std::thread::hardware_concurrency()));

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--joker") {
            if (i + 1 >= argc) {
                std::cerr << "Missing value after --joker\n";
                return false;
            }
            opts.criteria.jokerNeedles.emplace_back(argv[++i]);
        } else if (arg == "--find") {
            if (i + 1 >= argc) {
                std::cerr << "Missing value after --find\n";
                return false;
            }
            opts.criteria.textNeedles.emplace_back(argv[++i]);
        } else if (arg == "--seed") {
            if (i + 1 >= argc) {
                std::cerr << "Missing value after --seed\n";
                return false;
            }
            std::string normalized = normalizeSeed(argv[++i]);
            if (normalized.empty()) {
                std::cerr << "Seed provided to --seed is invalid\n";
                return false;
            }
            opts.explicitSeeds.push_back(normalized);
        } else if (arg == "--start") {
            if (i + 1 >= argc) {
                std::cerr << "Missing value after --start\n";
                return false;
            }
            std::string normalized = normalizeSeed(argv[++i]);
            if (normalized.size() != kSeedLength) {
                std::cerr << "--start requires an 8-character alphanumeric seed\n";
                return false;
            }
            std::uint64_t value = 0;
            if (!seedToNumber(normalized, value)) {
                std::cerr << "--start seed contains unsupported characters\n";
                return false;
            }
            opts.rangeStart = value;
            opts.hasRange = true;
        } else if (arg == "--count") {
            if (i + 1 >= argc) {
                std::cerr << "Missing value after --count\n";
                return false;
            }
            std::uint64_t count = 0;
            if (!parsePositiveUInt64(argv[++i], count)) {
                std::cerr << "--count requires a positive integer\n";
                return false;
            }
            opts.rangeCount = count;
        } else if (arg == "--limit") {
            if (i + 1 >= argc) {
                std::cerr << "Missing value after --limit\n";
                return false;
            }
            int limit = 0;
            if (!parsePositiveInt(argv[++i], limit)) {
                std::cerr << "--limit requires a positive integer\n";
                return false;
            }
            opts.limit = limit;
        } else if (arg == "--threads") {
            if (i + 1 >= argc) {
                std::cerr << "Missing value after --threads\n";
                return false;
            }
            int threadCount = 0;
            if (!parsePositiveInt(argv[++i], threadCount)) {
                std::cerr << "--threads requires a positive integer\n";
                return false;
            }
            opts.threadCount = threadCount;
        } else if (arg == "--early") {
            if (i + 1 >= argc) {
                std::cerr << "Missing value after --early\n";
                return false;
            }
            int early = 0;
            if (!parsePositiveInt(argv[++i], early)) {
                std::cerr << "--early requires a positive integer\n";
                return false;
            }
            opts.early = early;
        } else if (arg == "--any") {
            if (i + 1 >= argc) {
                std::cerr << "Missing value after --any\n";
                return false;
            }
            int minMatches = 0;
            if (!parsePositiveInt(argv[++i], minMatches)) {
                std::cerr << "--any requires a positive integer\n";
                return false;
            }
            opts.criteria.requireAll = false;
            opts.criteria.minMatches = minMatches;
        } else if (arg == "--all") {
            opts.criteria.requireAll = true;
            opts.criteria.minMatches = 1;
        } else if (arg == "--negative" || arg == "--neg") {
            if (i + 1 >= argc) {
                std::cerr << "Missing value after " << arg << "\n";
                return false;
            }
            int count = 0;
            if (!parsePositiveInt(argv[++i], count)) {
                std::cerr << arg << " requires a positive integer\n";
                return false;
            }
            opts.criteria.minNegative = count;
        } else if (arg == "--poly" || arg == "--polychrome") {
            if (i + 1 >= argc) {
                std::cerr << "Missing value after " << arg << "\n";
                return false;
            }
            int count = 0;
            if (!parsePositiveInt(argv[++i], count)) {
                std::cerr << arg << " requires a positive integer\n";
                return false;
            }
            opts.criteria.minPolychrome = count;
        } else if (arg == "--holo" || arg == "--holographic") {
            if (i + 1 >= argc) {
                std::cerr << "Missing value after " << arg << "\n";
                return false;
            }
            int count = 0;
            if (!parsePositiveInt(argv[++i], count)) {
                std::cerr << arg << " requires a positive integer\n";
                return false;
            }
            opts.criteria.minHolographic = count;
        } else if (arg == "--foil") {
            if (i + 1 >= argc) {
                std::cerr << "Missing value after --foil\n";
                return false;
            }
            int count = 0;
            if (!parsePositiveInt(argv[++i], count)) {
                std::cerr << "--foil requires a positive integer\n";
                return false;
            }
            opts.criteria.minFoil = count;
        } else if (arg == "--progress") {
            opts.showProgress = true;
        } else if (arg == "--no-progress") {
            opts.showProgress = false;
        } else if (arg == "--color") {
            opts.colorMode = ColorMode::Always;
        } else if (arg == "--no-color") {
            opts.colorMode = ColorMode::Never;
        } else if (arg == "--deck") {
            if (i + 1 >= argc) {
                std::cerr << "Missing value after --deck\n";
                return false;
            }
            opts.baseConfig.deck = argv[++i];
        } else if (arg == "--stake") {
            if (i + 1 >= argc) {
                std::cerr << "Missing value after --stake\n";
                return false;
            }
            opts.baseConfig.stake = argv[++i];
        } else if (arg == "--version") {
            if (i + 1 >= argc) {
                std::cerr << "Missing value after --version\n";
                return false;
            }
            try {
                opts.baseConfig.version = std::stol(argv[++i]);
            } catch (...) {
                std::cerr << "--version requires an integer\n";
                return false;
            }
        } else if (arg == "--help" || arg == "-h") {
            printUsage();
            std::exit(0);
        } else {
            std::cerr << "Unrecognized argument: " << arg << "\n";
            return false;
        }
    }

    if (opts.criteria.jokerNeedles.empty() && opts.criteria.textNeedles.empty()) {
        std::cerr << "At least one --joker or --find argument is required\n";
        return false;
    }

    int totalConditions = static_cast<int>(opts.criteria.jokerNeedles.size() + opts.criteria.textNeedles.size());
    if (!opts.criteria.requireAll) {
        if (opts.criteria.minMatches <= 0) {
            opts.criteria.minMatches = 1;
        }
        if (opts.criteria.minMatches > totalConditions) {
            std::cerr << "--any value cannot exceed total number of conditions (" << totalConditions << ")\n";
            return false;
        }
    } else {
        opts.criteria.minMatches = std::max(1, totalConditions);
    }

    // Validate edition requirements
    int totalEditionReqs = opts.criteria.minNegative + opts.criteria.minPolychrome +
                           opts.criteria.minHolographic + opts.criteria.minFoil;
    if (totalEditionReqs > 0) {
        int targetMatches = opts.criteria.requireAll ? totalConditions : opts.criteria.minMatches;
        if (totalEditionReqs > targetMatches) {
            std::cerr << "Total edition requirements (" << totalEditionReqs
                      << ") cannot exceed target matches (" << targetMatches << ")\n";
            return false;
        }
    }

    if (!opts.hasRange && opts.explicitSeeds.empty()) {
        opts.hasRange = true;
        opts.rangeStart = 0;
        opts.randomizeStart = true;
        if (opts.rangeCount == 0) {
            opts.rangeCount = kTotalSeeds;
            opts.wrapRange = true;
        } else if (opts.rangeCount >= kTotalSeeds) {
            opts.rangeCount = kTotalSeeds;
            opts.wrapRange = true;
        }
    }

    if (opts.hasRange) {
        if (opts.wrapRange) {
            if (opts.rangeCount == 0 || opts.rangeCount > kTotalSeeds) {
                opts.rangeCount = kTotalSeeds;
            }
            if (opts.rangeStart >= kTotalSeeds) {
                opts.rangeStart %= kTotalSeeds;
            }
        } else {
            if (opts.rangeStart >= kTotalSeeds) {
                std::cerr << "--start seed is outside supported range\n";
                return false;
            }
            if (opts.rangeCount == 0) {
                opts.rangeCount = kTotalSeeds - opts.rangeStart;
            } else if (opts.rangeCount > kTotalSeeds - opts.rangeStart) {
                std::cerr << "--count exceeds remaining seed space after --start\n";
                return false;
            }
        }
    }

    if (opts.limit <= 0) {
        opts.limit = 1;
    }

    if (opts.threadCount <= 0) {
        opts.threadCount = 1;
    }

    analysis::normalizeSearchCriteria(opts.criteria);

    opts.colorize = computeColorEnabled(opts.colorMode);
    gColorEnabled = opts.colorize;

    return true;
}

void printMatch(const std::string& seed, const analysis::SearchMatch& match, std::mutex& outputMutex) {
    std::lock_guard<std::mutex> lock(outputMutex);

    std::vector<std::string> headline;
    {
        std::ostringstream title;
        title << "Seed " << seed;
        if (match.ante > 0) {
            title << " • Ante " << match.ante;
        }
        headline.push_back(title.str());
    }
    {
        std::ostringstream stats;
        stats << "Events: " << match.events.size();
        if (!match.highlights.empty()) {
            stats << " • Highlights: " << match.highlights.size();
        }
        headline.push_back(stats.str());
    }
    if (!match.boss.empty() || !match.voucher.empty()) {
        std::ostringstream meta;
        meta << "Boss: " << (match.boss.empty() ? "-" : match.boss)
             << "  |  Voucher: " << (match.voucher.empty() ? "-" : match.voucher);
        headline.push_back(meta.str());
    }
    if (!match.tags.empty()) {
        std::ostringstream tags;
        tags << "Tags: " << joinWith(match.tags, ", ");
        headline.push_back(tags.str());
    }

    printHeadlineBox(headline);
    std::cout << "\n";

    if (match.events.empty()) {
        std::cout << styleIf(gColorEnabled, kAnsiDim, "  (No detailed events recorded)") << "\n\n";
        return;
    }

    std::array<std::size_t, 6> widths = {4, 8, 4, 4, 4, 7};
    std::vector<TableRow> rows;
    rows.reserve(match.events.size());
    for (const auto& event : match.events) {
        TableRow row;
        row.cells[0] = event.name;
        row.cells[1] = event.location.empty() ? "-" : event.location;
        row.cells[2] = (event.ante > 0) ? std::to_string(event.ante) : "-";
        row.cells[3] = (event.slot > 0) ? std::to_string(event.slot) : "-";
        row.cells[4] = event.packName.empty() ? "-" : event.packName;
        row.cells[5] = joinDetails(event.details);
        row.tone = toneForEvent(event);
        for (std::size_t i = 0; i < row.cells.size(); ++i) {
            widths[i] = std::max(widths[i], row.cells[i].size());
        }
        rows.push_back(std::move(row));
    }

    printTableSection(widths, rows, "  ");
    std::cout << "\n";

    if (!match.highlights.empty()) {
        std::array<std::size_t, 6> hWidths = {4, 8, 4, 4, 4, 7};
        std::vector<TableRow> highlightRows;
        highlightRows.reserve(match.highlights.size());
        for (const auto& event : match.highlights) {
            TableRow row;
            row.cells[0] = event.name;
            row.cells[1] = event.location.empty() ? "-" : event.location;
            row.cells[2] = (event.ante > 0) ? std::to_string(event.ante) : "-";
            row.cells[3] = (event.slot > 0) ? std::to_string(event.slot) : "-";
            row.cells[4] = event.packName.empty() ? "-" : event.packName;
            row.cells[5] = joinDetails(event.details);
            row.tone = toneForEvent(event);
            if (row.tone == RowTone::Normal) {
                row.tone = RowTone::Rare;
            }
            for (std::size_t i = 0; i < row.cells.size(); ++i) {
                hWidths[i] = std::max(hWidths[i], row.cells[i].size());
            }
            highlightRows.push_back(std::move(row));
        }

        std::cout << styleIf(gColorEnabled, kAnsiHighlight, "  Highlights") << "\n";
        printTableSection(hWidths, highlightRows, "    ");
        std::cout << "\n";
    } else {
        std::cout << "\n";
    }
}

void searchExplicitSeeds(const Options& opts, std::atomic<int>& matchesFound, std::mutex& outputMutex) {
    analysis::AnalysisConfig config = opts.baseConfig;
    analysis::SearchMatch match;
    match.events.reserve(8);
    match.tags.reserve(4);
    for (const auto& seed : opts.explicitSeeds) {
        if (matchesFound.load() >= opts.limit) {
            break;
        }
        config.seed = seed;
        if (analysis::searchSeed(config, opts.criteria, match)) {
            int previous = matchesFound.fetch_add(1);
            if (previous < opts.limit) {
                printMatch(seed, match, outputMutex);
            }
            if (previous + 1 >= opts.limit) {
                break;
            }
        }
    }
}

void searchSequentialRange(const Options& opts,
                           std::atomic<int>& matchesFound,
                           std::mutex& outputMutex) {
    if (!opts.hasRange || matchesFound.load() >= opts.limit) {
        return;
    }

    std::uint64_t start = opts.rangeStart;
    std::uint64_t count = opts.rangeCount;
    if (count == 0) {
        return;
    }
    if (!opts.wrapRange) {
        if (start >= kTotalSeeds || count > kTotalSeeds - start) {
            std::cerr << "Specified range exceeds supported seed space\n";
            return;
        }
    } else {
        start %= kTotalSeeds;
        if (count > kTotalSeeds) {
            count = kTotalSeeds;
        }
    }

    std::atomic<std::uint64_t> nextIndex{0};
    std::atomic<bool> stop{false};
    std::atomic<std::uint64_t> processedSeeds{0};
    std::atomic<bool> progressDone{false};
    auto startTime = std::chrono::steady_clock::now();

    std::thread progressThread;
    if (opts.showProgress) {
        progressThread = std::thread([&]() {
            using clock = std::chrono::steady_clock;
            while (!progressDone.load()) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                if (progressDone.load()) {
                    break;
                }
                auto now = clock::now();
                double elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now - startTime).count();
                if (elapsed <= 0.0) elapsed = 1e-9;
                auto processed = processedSeeds.load(std::memory_order_relaxed);
                double pct = count ? (static_cast<double>(processed) / static_cast<double>(count)) * 100.0 : 0.0;
                double rate = static_cast<double>(processed) / elapsed;
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(2);
                oss << "Progress: " << processed << " / " << count
                    << " (" << pct << "%, " << rate << " seeds/s)";
                {
                    std::lock_guard<std::mutex> lock(outputMutex);
                    std::cerr << '\r' << oss.str() << std::flush;
                }
            }

            auto now = std::chrono::steady_clock::now();
            double elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now - startTime).count();
            if (elapsed <= 0.0) elapsed = 1e-9;
            auto processed = processedSeeds.load(std::memory_order_relaxed);
            double pct = count ? (static_cast<double>(processed) / static_cast<double>(count)) * 100.0 : 0.0;
            double rate = static_cast<double>(processed) / elapsed;
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2);
            oss << "Progress: " << processed << " / " << count
                << " (" << pct << "%, " << rate << " seeds/s)";
            {
                std::lock_guard<std::mutex> lock(outputMutex);
                std::cerr << '\r' << oss.str() << std::endl;
            }
        });
    }

    auto worker = [&](int) {
        analysis::AnalysisConfig config = opts.baseConfig;
        analysis::SearchMatch match;
        match.events.reserve(8);
        match.tags.reserve(4);
        while (!stop.load()) {
            std::uint64_t idx = nextIndex.fetch_add(1);
            if (idx >= count) {
                break;
            }
            if (matchesFound.load() >= opts.limit) {
                break;
            }
            std::uint64_t value = start + idx;
            if (opts.wrapRange) {
                value %= kTotalSeeds;
            }
            std::string seed = numberToSeed(value);
            config.seed = seed;
            bool hit = analysis::searchSeed(config, opts.criteria, match);
            processedSeeds.fetch_add(1, std::memory_order_relaxed);
            if (hit) {
                int previous = matchesFound.fetch_add(1);
                if (previous < opts.limit) {
                    printMatch(seed, match, outputMutex);
                }
                if (previous + 1 >= opts.limit) {
                    stop.store(true);
                    break;
                }
            }
        }
    };

    int threads = std::max(1, opts.threadCount);
    std::vector<std::thread> workers;
    workers.reserve(threads);
    for (int t = 0; t < threads; ++t) {
        workers.emplace_back(worker, t);
    }
    for (auto& thread : workers) {
        thread.join();
    }

    if (progressThread.joinable()) {
        progressDone.store(true);
        progressThread.join();
    }
}

} // namespace

int main(int argc, char** argv) {
    Options opts;
    if (!parseArguments(argc, argv, opts)) {
        printUsage();
        return 1;
    }

    opts.criteria.maxAnte = opts.early;
    opts.baseConfig.maxAnte = opts.early;

    if (opts.hasRange && opts.randomizeStart) {
        std::uint64_t maxStart = 0;
        if (opts.wrapRange) {
            maxStart = kTotalSeeds - 1;
        } else {
            if (opts.rangeCount > kTotalSeeds) {
                opts.rangeCount = kTotalSeeds;
            }
            if (opts.rangeCount >= kTotalSeeds) {
                maxStart = 0;
            } else {
                maxStart = kTotalSeeds - opts.rangeCount;
            }
        }

        std::random_device rd;
        auto now = static_cast<std::uint64_t>(
            std::chrono::high_resolution_clock::now().time_since_epoch().count());
        std::seed_seq seedSeq{
            static_cast<unsigned int>(rd()),
            static_cast<unsigned int>(now & 0xFFFFFFFFu),
            static_cast<unsigned int>((now >> 32) & 0xFFFFFFFFu)
        };
        std::mt19937_64 rng(seedSeq);
        std::uint64_t startValue = 0;
        if (maxStart > 0) {
            std::uniform_int_distribution<std::uint64_t> dist(0, maxStart);
            startValue = dist(rng);
        }
        opts.rangeStart = startValue;

        std::string startingSeed = numberToSeed(opts.rangeStart % kTotalSeeds);
        std::cerr << "Randomized starting seed: " << startingSeed << "\n";
    }

    std::atomic<int> matchesFound{0};
    std::mutex outputMutex;

    if (!opts.explicitSeeds.empty()) {
        searchExplicitSeeds(opts, matchesFound, outputMutex);
    }

    if (matchesFound.load() < opts.limit) {
        searchSequentialRange(opts, matchesFound, outputMutex);
    }

    if (matchesFound.load() == 0) {
        std::cout << "No matches found." << std::endl;
        return 1;
    }

    return 0;
}
