
#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "../app/schedule.hpp"

namespace churn {

class AdversarialWorkload : public Schedule {
private:
    Key nextKey = kKeyUniverse - 1;

public:
    std::size_t pickDeleteIndex(const std::vector<Key>& live, Rng& rng) override {
        (void)rng;

        std::size_t index = 0;

        for (std::size_t i = 1; i < live.size(); i++) {
            if (live[i] < live[index]) {
                index = i;
            }
        }

        return index;
    }

    Key pickInsertKey(Rng& rng) override {
        (void)rng;

        Key key = nextKey;

        if (nextKey == 0) {
            nextKey = kKeyUniverse - 1;
        }
        else {
            nextKey--;
        }

        return key;
    }

    std::string name() const override {
        return "adversarial";
    }
};

}
