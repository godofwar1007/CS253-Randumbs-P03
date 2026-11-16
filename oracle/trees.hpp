#pragma once

#include "dictionary.hpp"
#include "rng.hpp"
#include "../src/structA/hibbard.hpp"
#include "../src/structB/scapegoat.hpp"

#include <memory>
#include <string>

namespace churn {

inline std::unique_ptr<Dictionary> makeTestTree(const std::string& name, Rng& rng) {
    if (name == "hibbard_succ")
        return std::unique_ptr<Dictionary>(new HibbardBst(HibbardPolicy::Successor, rng));
    if (name == "hibbard_rand")
        return std::unique_ptr<Dictionary>(new HibbardBst(HibbardPolicy::Random, rng));
    if (name == "scapegoat_array")
        return std::unique_ptr<Dictionary>(new ScapegoatTree(ScapegoatTree::RebuildMode::Array));
    if (name == "scapegoat_inplace")
        return std::unique_ptr<Dictionary>(new ScapegoatTree(ScapegoatTree::RebuildMode::InPlace));
    return nullptr;
}

}
