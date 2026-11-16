#pragma once
// Pass seeded generators explicitly by reference. The simulator keeps its
// workload generator separate from the generator used by tree policies.

#include <cstdint>
#include <random>

namespace churn {

using Rng = std::mt19937_64;

// Upper bound (exclusive) of the key universe used by the simulator. The
// universe is far larger than any n used (at most 4000), so a fresh key is
// effectively a uniform draw from a continuum, as in the random-insertion
// model. Collisions are possible in principle and are handled by redrawing.
constexpr int kKeyUniverse = 1 << 30;

// Uniform integer in [lo, hi], inclusive.
inline std::uint64_t uniformInt(Rng& rng, std::uint64_t lo, std::uint64_t hi) {
    std::uniform_int_distribution<std::uint64_t> dist(lo, hi);
    return dist(rng);
}

// Uniform key in [0, kKeyUniverse).
inline int randomKey(Rng& rng) {
    return static_cast<int>(uniformInt(rng, 0, kKeyUniverse - 1));
}

}  // namespace churn