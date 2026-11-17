#pragma once

#include "harness.hpp"
#include "../adversary/adversarial_workload.hpp"

namespace churn {

inline std::vector<Operation> makeAdversarialOperations(std::uint64_t seed,
        std::size_t count, std::size_t initialSize) {
    if (initialSize < 2 || initialSize > static_cast<std::size_t>(kKeyUniverse))
        throw std::runtime_error("adversarial size must be between 2 and the key universe");
    if (count < initialSize || count - initialSize < 2 || (count - initialSize) % 2 != 0)
        throw std::runtime_error("operations must equal size plus a positive even number");

    Rng rng(seed);
    SortedArrayOracle current;
    AdversarialWorkload schedule;
    std::vector<Key> live;
    std::vector<Operation> ops;
    live.reserve(initialSize);
    ops.reserve(count);

    while (live.size() < initialSize) {
        Key key = randomKey(rng);
        if (current.insert(key)) {
            live.push_back(key);
            ops.push_back({"insert", key});
        }
    }

    while (ops.size() < count) {
        std::size_t index = schedule.pickDeleteIndex(live, rng);
        if (index >= live.size()) throw std::runtime_error("adversary returned an invalid index");
        Key victim = live[index];
        current.erase(victim);
        live[index] = live.back();
        live.pop_back();
        ops.push_back({"erase", victim});

        Key key = schedule.pickInsertKey(rng);
        while (!current.insert(key)) key = schedule.pickInsertKey(rng);
        live.push_back(key);
        ops.push_back({"insert", key});
    }
    return ops;
}

}
