#pragma once

#include "dictionary.hpp"
#include "rng.hpp"

#include <memory>
#include <string>

namespace churn {

inline std::unique_ptr<Dictionary> makeTestTree(const std::string&, Rng&) {
    return nullptr;
}

}
