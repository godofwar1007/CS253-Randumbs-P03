#pragma once
#include <memory>
#include <string>

#include "dictionary.hpp"
#include "rng.hpp"
#include "schedule.hpp"
#include "standin_bst.hpp"

#include "hibbard.hpp"
#include "scapegoat.hpp"
#include "../oracle/sorted_array.hpp"
#include "../adversary/schedules.hpp"

namespace churn {

inline std::unique_ptr<Dictionary> makeDictionary(const std::string& name, Rng& rng) {
    if (name == "standin_bst") return std::make_unique<StandinBst>();
    if (name == "hibbard_succ") {
        return std::make_unique<HibbardBst>(HibbardPolicy::Successor, rng);
    }
    if (name == "hibbard_rand") {
        return std::make_unique<HibbardBst>(HibbardPolicy::Random, rng);
    }
    if (name == "scapegoat_array") {
        return std::make_unique<ScapegoatTree>(ScapegoatTree::RebuildMode::Array);
    }
    if (name == "scapegoat_inplace") {
        return std::make_unique<ScapegoatTree>(ScapegoatTree::RebuildMode::InPlace);
    }
    if (name == "oracle") return std::make_unique<SortedArrayOracle>();
    return nullptr;
}

inline std::unique_ptr<Schedule> makeSchedule(const std::string& name) {
    if (name == "uniform") return std::make_unique<UniformSchedule>();
    if (name == "min_delete_high_insert") return std::make_unique<MinDeleteHighInsertSchedule>();
    if (name == "max_delete_low_insert") return std::make_unique<MaxDeleteLowInsertSchedule>();
    if (name == "scapegoat_stress") return std::make_unique<ScapegoatStressSchedule>();
    return nullptr;
}

}  // namespace churn
