#pragma once
#include <memory>
#include <string>

#include "dictionary.hpp"
#include "rng.hpp"
#include "schedule.hpp"
#include "standin_bst.hpp"

// #include "../src/structA/hibbard.hpp"
// #include "../src/structB/scapegoat.hpp"
#include "../oracle/sorted_array.hpp"

namespace churn {

inline std::unique_ptr<Dictionary> makeDictionary(const std::string& name, Rng& rng) {
    (void)rng;
    if (name == "standin_bst") return std::make_unique<StandinBst>();
    // if (name == "hibbard_succ")      return std::make_unique<...>();
    // if (name == "hibbard_rand")      return std::make_unique<...>(rng);
    // if (name == "scapegoat_array")   return std::make_unique<...>();
    // if (name == "scapegoat_inplace") return std::make_unique<...>();
    if (name == "oracle") return std::make_unique<SortedArrayOracle>();
    return nullptr;
}

inline std::unique_ptr<Schedule> makeSchedule(const std::string& name) {
    if (name == "uniform") return std::make_unique<UniformSchedule>();
    return nullptr;
}

}  // namespace churn