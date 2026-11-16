#pragma once
#include <cstddef>
#include <string>
#include <vector>

#include "dictionary.hpp"
#include "rng.hpp"

namespace churn {

class Schedule {
public:
    virtual ~Schedule() = default;
    virtual std::size_t pickDeleteIndex(const std::vector<Key>& live, Rng& rng) = 0;
    virtual Key pickInsertKey(Rng& rng) = 0;
    virtual std::string name() const = 0;
};

class UniformSchedule : public Schedule {
public:
    std::size_t pickDeleteIndex(const std::vector<Key>& live, Rng& rng) override {
        return static_cast<std::size_t>(uniformInt(rng, 0, live.size() - 1));
    }
    Key pickInsertKey(Rng& rng) override { return randomKey(rng); }
    std::string name() const override { return "uniform"; }
};

}  // namespace churn