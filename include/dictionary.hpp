#pragma once
// Shared contract for every dictionary in our Project.
// All strategies (Structure A policies, Structure B, the sorted-array oracle)
// We are implementing this, so the harness and the simulator never depends on a concrete type

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

namespace churn {

using Key = int;  // Keys are 32-bit ints. Duplicates are rejected.

class Dictionary {
public:
    virtual ~Dictionary() = default;

    // Inserts key. Returns true if inserted, false if it was already present
    // (in which case the structure is unchanged).
    virtual bool insert(Key key) = 0;

    // Erases key. Returns true if erased, false if it was absent
    // (in which case the structure is unchanged).
    virtual bool erase(Key key) = 0;

    virtual bool contains(Key key) const = 0;

    virtual std::size_t size() const = 0;

    // Internal path length: the sum over all stored keys of the depth of the
    // node holding the key, with the ROOT AT DEPTH 0 (depth = number of edges
    // from the root). Average depth is totalDepth() / size().
    // Implementations should maintain this incrementally, in O(1) per
    // operation amortised over the work the operation already does. The
    // oracle (a sorted array) has no tree, so it returns 0.
    virtual std::uint64_t totalDepth() const = 0;

    // Number of rebuild events performed so far. A scapegoat subtree rebuild
    // and a full-tree rebuild each count as one event. Structures that never
    // rebuild return 0.
    virtual std::uint64_t rebuildCount() const = 0;

    // Appends all keys to out in ascending order. Used by the differential
    // tests; not on any hot path.
    virtual void inorder(std::vector<Key>& out) const = 0;

    // Checks the structure's own invariants (BST order, size fields, depth
    // bookkeeping, weight balance where applicable). Returns true if valid.
    // Used by the tests only; may be O(n).
    virtual bool validate() const = 0;

    // Short label used in CSV output, for example "hibbard_succ",
    // "hibbard_rand", "scapegoat_array", "scapegoat_inplace", "oracle".
    virtual std::string name() const = 0;
};

// Average node depth, root at depth 0. Returns 0 for an empty structure.
inline double averageDepth(const Dictionary& d) {
    return d.size() == 0 ? 0.0
                         : static_cast<double>(d.totalDepth()) /
                               static_cast<double>(d.size());
}

}  // namespace churn