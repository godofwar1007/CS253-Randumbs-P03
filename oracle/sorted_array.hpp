#pragma once

#include "dictionary.hpp"

#include <algorithm>
#include <vector>
using namespace std;

namespace churn {

// Simple reference implementation: lookup O(log n), updates O(n).
class SortedArrayOracle final : public Dictionary {
private:
    vector<Key> keys;

public:
    bool insert(Key key) override 
    {
        auto pos = lower_bound(keys.begin(), keys.end(), key);
        if (pos != keys.end() && *pos == key) 
        {
            return false;
        }
        keys.insert(pos, key);
        return true;
    }

    bool erase(Key key) override 
    {
        auto pos = lower_bound(keys.begin(), keys.end(), key);
        if (pos == keys.end() || *pos != key) 
        {
            return false;
        }
        keys.erase(pos);
        return true;
    }

    bool contains(Key key) const override 
    {
        return binary_search(keys.begin(), keys.end(), key);
    }

    size_t size() const override { return keys.size(); }

    unsigned long long totalDepth() const override { return 0; }

    unsigned long long rebuildCount() const override { return 0; }

    void inorder(vector<Key>& out) const override {
        out.insert(out.end(), keys.begin(), keys.end());
    }

    bool validate() const override {
        for (size_t i = 1; i<keys.size(); ++i) {
            if (keys[i-1] >= keys[i]) {
                return false;
            }
        }
        return true;
    }

    string name() const override { return "oracle"; }
};

}
