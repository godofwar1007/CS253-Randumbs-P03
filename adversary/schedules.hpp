
#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "../app/schedule.hpp"

namespace churn {

class MinDeleteHighInsertSchedule : public Schedule {
private:
    Key nextKey = kKeyUniverse - 1;

public:
    std::size_t pickDeleteIndex(const std::vector<Key>& live, Rng& rng) override {
        (void)rng;

        if (live.empty()) return 0;

        std::size_t index = 0;

        for (std::size_t i = 1; i < live.size(); i++) {
            if (live[i] < live[index]) index = i;
        }

        return index;
    }

    Key pickInsertKey(Rng& rng) override {
        (void)rng;

        Key key = nextKey;

        if (nextKey == 0) nextKey = kKeyUniverse - 1;
        else nextKey--;

        return key;
    }

    std::string name() const override {
        return "min_delete_high_insert";
    }
};

class MaxDeleteLowInsertSchedule : public Schedule {
private:
    Key nextKey = 0;

public:
    std::size_t pickDeleteIndex(const std::vector<Key>& live, Rng& rng) override {
        (void)rng;

        if (live.empty()) return 0;

        std::size_t index = 0;

        for (std::size_t i = 1; i < live.size(); i++) {
            if (live[i] > live[index]) index = i;
        }

        return index;
    }

    Key pickInsertKey(Rng& rng) override {
        (void)rng;

        Key key = nextKey;

        if (nextKey == kKeyUniverse - 1) nextKey = 0;
        else nextKey++;

        return key;
    }

    std::string name() const override {
        return "max_delete_low_insert";
    }
};

class ScapegoatStressSchedule : public Schedule {
private:
    Key nextKey = 0;

public:
    std::size_t pickDeleteIndex(const std::vector<Key>& live, Rng& rng) override {
        if (live.empty()) return 0;

        return static_cast<std::size_t>(
            uniformInt(rng, 0, live.size() - 1)
        );
    }

    Key pickInsertKey(Rng& rng) override {
        (void)rng;

        Key key = nextKey;

        if (nextKey == kKeyUniverse - 1) nextKey = 0;
        else nextKey++;

        return key;
    }

    std::string name() const override {
        return "scapegoat_stress";
    }
};

}
