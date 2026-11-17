#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

namespace churn {

using Key = int;

class Dictionary {
public:
    virtual ~Dictionary() = default;

    // Returns false and leaves the tree unchanged for a duplicate key.
    virtual bool insert(Key key) = 0;

    // Returns false and leaves the tree unchanged for a missing key.
    virtual bool erase(Key key) = 0;

    virtual bool contains(Key key) const = 0;

    virtual std::size_t size() const = 0;

    // Sum of node depths from root depth 0. The sorted-array oracle returns 0.
    // Tree implementations maintain this during updates.
    virtual std::uint64_t totalDepth() const = 0;

    // Counts subtree and whole-tree rebuilds. Non-rebuilding strategies return 0.
    virtual std::uint64_t rebuildCount() const = 0;

    // Appends keys in ascending order.
    virtual void inorder(std::vector<Key>& out) const = 0;

    // Checks structural and bookkeeping invariants. May take O(n).
    virtual bool validate() const = 0;

    // Strategy label for CSV output.
    virtual std::string name() const = 0;
};

inline double averageDepth(const Dictionary& d) {
    return d.size() == 0 ? 0.0
                         : static_cast<double>(d.totalDepth()) /
                               static_cast<double>(d.size());
}

}