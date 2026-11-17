#include <charconv>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <unordered_set>
#include <vector>

#include "dictionary.hpp"
#include "factory.hpp"
#include "rng.hpp"
#include "schedule.hpp"

using namespace churn;

static std::map<std::string, std::string> parseArgs(int argc, char** argv) {
    std::map<std::string, std::string> a;
    for (int i = 1; i < argc; ++i) {
        std::string k = argv[i];
        if (k.rfind("--", 0) != 0 || i + 1 >= argc) {
            std::cerr << "bad argument: " << k << "\n";
            std::exit(2);
        }
        a[k.substr(2)] = argv[++i];
    }
    return a;
}

static std::uint64_t parseUnsigned(const char* option, const std::string& value) {
    std::uint64_t result = 0;
    const char* end = value.data() + value.size();
    const auto parsed = std::from_chars(value.data(), end, result);
    if (parsed.ec != std::errc{} || parsed.ptr != end) {
        std::cerr << "invalid value for --" << option << ": " << value << '\n';
        std::exit(2);
    }
    return result;
}

int main(int argc, char** argv) {
    auto args = parseArgs(argc, argv);
    if (!args.count("strategy") || !args.count("n") || !args.count("seed")) {
        std::cerr << "usage: simulator --strategy NAME --n N --seed S "
                     "[--pairs P] [--sample K] [--schedule NAME] "
                     "[--out FILE]\n";
        return 2;
    }
    const std::string strategy = args["strategy"];
    const std::uint64_t requestedSize = parseUnsigned("n", args["n"]);
    if (requestedSize < 2 || requestedSize > kKeyUniverse) {
        std::cerr << "n must be between 2 and " << kKeyUniverse << '\n';
        return 2;
    }
    const auto n = static_cast<std::size_t>(requestedSize);
    const std::uint64_t seed = parseUnsigned("seed", args["seed"]);
    const std::uint64_t pairs =
        args.count("pairs") ? parseUnsigned("pairs", args["pairs"])
                            : static_cast<std::uint64_t>(n) * n;
    const std::uint64_t sample =
        args.count("sample") ? parseUnsigned("sample", args["sample"]) : n;
    const std::string schedName =
        args.count("schedule") ? args["schedule"] : "uniform";
    if (sample == 0) {
        std::cerr << "sample must be at least 1\n";
        return 2;
    }

    Rng workloadRng(seed);
    std::seed_seq deletionSeed{static_cast<std::uint32_t>(seed),
                               static_cast<std::uint32_t>(seed >> 32), 1u};
    Rng deletionRng(deletionSeed);
    auto dict = makeDictionary(strategy, deletionRng);
    if (!dict) {
        std::cerr << "unknown strategy: " << strategy << "\n";
        return 2;
    }
    auto sched = makeSchedule(schedName);
    if (!sched) {
        std::cerr << "unknown schedule: " << schedName << "\n";
        return 2;
    }

    std::ofstream file;
    std::ostream* out = &std::cout;
    if (args.count("out") && args["out"] != "-") {
        file.open(args["out"]);
        if (!file) {
            std::cerr << "cannot open " << args["out"] << "\n";
            return 2;
        }
        out = &file;
    }
    *out << "strategy,n,seed,pair_index,avg_depth,rebuild_count\n";
    *out << std::fixed << std::setprecision(6);

    auto writeRow = [&](std::uint64_t p) {
        *out << dict->name() << ',' << n << ',' << seed << ',' << p << ','
             << averageDepth(*dict) << ',' << dict->rebuildCount() << '\n';
    };

    std::vector<Key> live;
    live.reserve(n);
    std::unordered_set<Key> liveKeys;
    liveKeys.reserve(n);
    while (live.size() < n) {
        const Key key = randomKey(workloadRng);
        if (liveKeys.insert(key).second) live.push_back(key);
    }
    for (const Key key : live) {
        if (!dict->insert(key)) {
            std::cerr << "error: insert failed for initial key " << key << '\n';
            return 1;
        }
    }

    const auto t0 = std::chrono::steady_clock::now();
    writeRow(0);
    for (std::uint64_t i = 0; i < pairs; ++i) {
        const std::uint64_t pairIndex = i + 1;
        const std::size_t index = sched->pickDeleteIndex(live, workloadRng);
        const Key victim = live[index];
        live[index] = live.back();
        live.pop_back();
        liveKeys.erase(victim);

        Key key;
        do {
            key = sched->pickInsertKey(workloadRng);
        } while (!liveKeys.insert(key).second);
        live.push_back(key);

        if (!dict->erase(victim)) {
            std::cerr << "error: erase failed for stored key at pair " << pairIndex << '\n';
            return 1;
        }
        if (!dict->insert(key)) {
            std::cerr << "error: insert failed for fresh key at pair " << pairIndex << '\n';
            return 1;
        }

        if (pairIndex % sample == 0) writeRow(pairIndex);
    }
    if (pairs % sample != 0) writeRow(pairs);

    const double secs = std::chrono::duration<double>(
                            std::chrono::steady_clock::now() - t0)
                            .count();
    std::cerr << strategy << " n=" << n << " seed=" << seed
              << " schedule=" << schedName << " pairs=" << pairs
              << " time=" << secs << "s\n";

    if (dict->size() != n) {
        std::cerr << "error: final size " << dict->size() << " != " << n
                  << "\n";
        return 1;
    }
    if (!dict->validate()) {
        std::cerr << "error: validate() failed at end of run\n";
        return 1;
    }
    return 0;
}

// ./simulator --strategy NAME --n N --seed S [--pairs P] [--sample K]
//            [--schedule NAME] [--out FILE]

// Strategies:
//   hibbard_succ     - Hibbard BST with always-successor deletion
//   hibbard_rand     - Hibbard BST with random predecessor or successor deletion
//   scapegoat_array  - Scapegoat tree with array-based rebuilding
//   scapegoat_inplace - Scapegoat tree with in-place Stout–Warren rebuilding
//   oracle          - Sorted-array reference implementation
//   standin_bst     - Stand-in BST for testing