#pragma once

#include "dictionary.hpp"

#include <algorithm>
#include <vector>

namespace churn {

// Simple reference implementation: lookup O(log n), updates O(n).
class SortedArrayOracle final : public Dictionary {
private:
    std::vector<Key> keys_;

public:
    bool insert(Key key) override {
        auto pos = std::lower_bound(keys_.begin(), keys_.end(), key);
        if (pos != keys_.end() && *pos == key) {
            return false;
        }
        keys_.insert(pos, key);
        return true;
    }

    bool erase(Key key) override {
        auto pos = std::lower_bound(keys_.begin(), keys_.end(), key);
        if (pos == keys_.end() || *pos != key) {
            return false;
        }
        keys_.erase(pos);
        return true;
    }

    bool contains(Key key) const override {
        return std::binary_search(keys_.begin(), keys_.end(), key);
    }

    std::size_t size() const override { return keys_.size(); }

    std::uint64_t totalDepth() const override { return 0; }

    std::uint64_t rebuildCount() const override { return 0; }

    void inorder(std::vector<Key>& out) const override {
        out.insert(out.end(), keys_.begin(), keys_.end());
    }

    bool validate() const override {
        for (std::size_t i = 1; i < keys_.size(); ++i) {
            if (keys_[i - 1] >= keys_[i]) {
                return false;
            }
        }
        return true;
    }

    std::string name() const override { return "oracle"; }
};

} // namespace churn
