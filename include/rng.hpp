#pragma once
// Keep workload and tree-policy RNG streams separate.

#include <cstdint>
#include <random>

namespace churn {

using Rng = std::mt19937_64;

// Exclusive upper bound for generated keys.
constexpr int kKeyUniverse = 1 << 30;

// Uniform integer in [lo, hi], inclusive.
inline std::uint64_t uniformInt(Rng& rng, std::uint64_t lo, std::uint64_t hi) {
    std::uniform_int_distribution<std::uint64_t> dist(lo, hi);
    return dist(rng);
}

inline int randomKey(Rng& rng) {
    return static_cast<int>(uniformInt(rng, 0, kKeyUniverse - 1));
}

}