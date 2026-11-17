#include "../app/factory.hpp"
#include "adversarial_workload.hpp"
#include "schedules.hpp"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string_view>

using namespace churn;

namespace {

std::uint64_t number(std::string_view value) {
    std::uint64_t parsed = 0;
    const auto end = value.data() + value.size();
    const auto result = std::from_chars(value.data(), end, parsed);
    if (result.ec != std::errc{} || result.ptr != end) {
        throw std::invalid_argument("expected a nonnegative integer");
    }
    return parsed;
}

std::unique_ptr<Schedule> scheduleFor(const std::string& name) {
    if (name == "min_delete_high_insert") return std::make_unique<MinDeleteHighInsertSchedule>();
    if (name == "max_delete_low_insert") return std::make_unique<MaxDeleteLowInsertSchedule>();
    if (name == "scapegoat_stress") return std::make_unique<ScapegoatStressSchedule>();
    if (name == "adversarial") return std::make_unique<AdversarialWorkload>();
    return makeSchedule(name);
}

void run(const std::string& strategy, const std::string& scheduleName,
         std::size_t n, std::uint64_t seed, std::uint64_t pairs, std::uint64_t sample) {
    if (strategy != "hibbard_succ" && strategy != "hibbard_rand" &&
        strategy != "scapegoat_array" && strategy != "scapegoat_inplace") {
        throw std::invalid_argument("unknown tree strategy");
    }
    Rng workload(seed);
    Rng policy(seed ^ 0x9e3779b97f4a7c15ULL);
    auto tree = makeDictionary(strategy, policy);
    auto schedule = scheduleFor(scheduleName);
    if (!schedule) throw std::invalid_argument("unknown schedule");

    std::vector<Key> live;
    live.reserve(n);
    while (live.size() < n) {
        const Key key = randomKey(workload);
        if (tree->insert(key)) live.push_back(key);
    }

    const auto start = std::chrono::steady_clock::now();
    std::cout << "strategy,schedule,n,seed,pair_index,avg_depth,rebuild_count,elapsed_seconds\n";
    std::cout << std::fixed << std::setprecision(9);
    auto row = [&](std::uint64_t pair) {
        const auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start);
        std::cout << strategy << ',' << schedule->name() << ',' << n << ',' << seed << ','
                  << pair << ',' << averageDepth(*tree) << ',' << tree->rebuildCount()
                  << ',' << elapsed.count() << '\n';
    };

    row(0);
    for (std::uint64_t completed = 0; completed < pairs; ++completed) {
        const auto index = schedule->pickDeleteIndex(live, workload);
        if (index >= live.size() || !tree->erase(live[index])) {
            throw std::runtime_error("invalid deletion");
        }
        live[index] = live.back();
        live.pop_back();
        Key key;
        do {
            key = schedule->pickInsertKey(workload);
        } while (!tree->insert(key));
        live.push_back(key);
        const auto pair = completed + 1;
        if (pair % sample == 0 || pair == pairs) row(pair);
    }

    std::vector<Key> actual;
    tree->inorder(actual);
    std::sort(live.begin(), live.end());
    if (actual != live || tree->size() != n || !tree->validate()) {
        throw std::runtime_error("final tree validation failed");
    }
    if (!std::cout) throw std::runtime_error("could not write CSV");
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 7) {
        std::cerr << "usage: run_workload STRATEGY SCHEDULE N SEED PAIRS SAMPLE\n";
        return 2;
    }
    try {
        const auto n = number(argv[3]);
        const auto pairs = number(argv[5]);
        const auto sample = number(argv[6]);
        if (n < 2 || n >= kKeyUniverse || pairs == 0 || sample == 0) {
            throw std::invalid_argument("require 2 <= N < key universe and positive PAIRS and SAMPLE");
        }
        run(argv[1], argv[2], static_cast<std::size_t>(n), number(argv[4]), pairs, sample);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
