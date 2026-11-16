// Usage: simulator --strategy NAME --n N --seed S [--pairs P] [--sample K]
//                  [--schedule NAME] [--out FILE]

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
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

int main(int argc, char** argv) {
    auto args = parseArgs(argc, argv);
    if (!args.count("strategy") || !args.count("n") || !args.count("seed")) {
        std::cerr << "usage: simulator --strategy NAME --n N --seed S "
                     "[--pairs P] [--sample K] [--schedule NAME] "
                     "[--out FILE]\n";
        return 2;
    }
    const std::string strategy = args["strategy"];
    const std::size_t n = std::stoull(args["n"]);
    const std::uint64_t seed = std::stoull(args["seed"]);
    const std::uint64_t pairs =
        args.count("pairs") ? std::stoull(args["pairs"])
                            : static_cast<std::uint64_t>(n) * n;
    const std::uint64_t sample =
        args.count("sample") ? std::stoull(args["sample"]) : n;
    const std::string schedName =
        args.count("schedule") ? args["schedule"] : "uniform";
    if (n < 2 || sample == 0) {
        std::cerr << "n must be at least 2 and sample at least 1\n";
        return 2;
    }

    Rng rng(seed);
    auto dict = makeDictionary(strategy, rng);
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
    while (live.size() < n) {
        Key k = randomKey(rng);
        if (dict->insert(k)) live.push_back(k);
    }

    const auto t0 = std::chrono::steady_clock::now();
    writeRow(0);
    for (std::uint64_t p = 1; p <= pairs; ++p) {
        std::size_t idx = sched->pickDeleteIndex(live, rng);
        Key victim = live[idx];
        live[idx] = live.back();
        live.pop_back();
        if (!dict->erase(victim)) {
            std::cerr << "error: erase failed for stored key at pair " << p
                      << "\n";
            return 1;
        }
        Key k = sched->pickInsertKey(rng);
        while (!dict->insert(k)) k = sched->pickInsertKey(rng);
        live.push_back(k);

        if (p % sample == 0) writeRow(p);
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