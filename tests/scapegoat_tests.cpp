#include "scapegoat.hpp"
#include "../oracle/sorted_array.hpp"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <optional>
#include <random>
#include <stdexcept>
#include <utility>

namespace churn {

struct ScapegoatTreeTestAccess {
    static bool isPerfect(const ScapegoatTree& tree, bool skipRoot = false) {
        const auto* node = tree.root_;
        return perfectSize(skipRoot && node ? node->right : node).has_value();
    }

    static auto shape(const ScapegoatTree& tree) {
        std::vector<std::optional<Key>> keys;
        preorder(tree.root_, keys);
        return keys;
    }

    static std::size_t depthLimit(std::size_t size) {
        return ScapegoatTree::depthLimit(size);
    }

    static void setHighWaterMarkToLiveSize(ScapegoatTree& tree) {
        tree.maxSize_ = tree.size();
    }

private:
    static std::optional<std::size_t> perfectSize(const ScapegoatTree::Node* node) {
        if (!node) return 0;
        const auto left = perfectSize(node->left);
        const auto right = perfectSize(node->right);
        if (!left || !right || *left > *right + 1 || *right > *left + 1) {
            return std::nullopt;
        }
        return *left + *right + 1;
    }

    static void preorder(const ScapegoatTree::Node* node,
                         std::vector<std::optional<Key>>& keys) {
        if (!node) {
            keys.emplace_back(std::nullopt);
            return;
        }
        keys.emplace_back(node->key);
        preorder(node->left, keys);
        preorder(node->right, keys);
    }
};

}

namespace {

using churn::Key;
using churn::ScapegoatTree;
using churn::ScapegoatTreeTestAccess;
using Mode = ScapegoatTree::RebuildMode;

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void testUpdateAccounting(Mode mode) {
    ScapegoatTree tree(mode);
    require(tree.name() == (mode == Mode::Array ? "scapegoat_array" : "scapegoat_inplace"),
            "strategy name must match the dictionary contract");
    require(tree.validate() && tree.size() == 0 && tree.totalDepth() == 0,
            "new tree must be empty and valid");
    require(!tree.erase(10) && !tree.contains(10), "empty tree lookup and erase");

    const Key minimum = std::numeric_limits<Key>::min();
    const Key maximum = std::numeric_limits<Key>::max();
    for (Key key : {10, 0, 30, minimum, 20, maximum, 25}) {
        require(tree.insert(key), "distinct key must be inserted");
    }
    require(tree.validate() && tree.totalDepth() == 11 && tree.rebuildCount() == 0,
            "unexpected depth or rebuild after building the deletion fixture");

    const auto initialShape = ScapegoatTreeTestAccess::shape(tree);
    require(!tree.insert(20) && !tree.erase(21), "duplicate and missing updates must fail");
    require(tree.size() == 7 && tree.totalDepth() == 11 && tree.rebuildCount() == 0 &&
                ScapegoatTreeTestAccess::shape(tree) == initialShape,
            "failed updates must preserve size, depth, rebuild count, and shape");

    require(tree.erase(10) && tree.totalDepth() == 8 && tree.validate(),
            "deep successor with a right child must account for promotion");
    require(tree.erase(0) && tree.totalDepth() == 6 && tree.validate(),
            "one-child erase must lower every promoted node's depth");
    require(tree.erase(maximum) && tree.totalDepth() == 4 && tree.validate() &&
                tree.rebuildCount() == 1,
            "leaf erase below the shrink threshold must rebuild the tree");
    require(ScapegoatTreeTestAccess::isPerfect(tree), "shrink rebuild must be perfect");

    std::vector<Key> ordered{42};
    tree.inorder(ordered);
    require(ordered == std::vector<Key>{42, minimum, 20, 25, 30},
            "inorder must append sorted keys to the supplied vector");
    for (Key key : {minimum, 20, 25, 30}) {
        require(tree.erase(key) && !tree.contains(key) && tree.validate(), "delete to empty");
    }
    require(tree.totalDepth() == 0 && tree.insert(maximum) && tree.validate(),
            "empty tree must accept a fresh root");

    ScapegoatTree immediate(mode);
    for (Key key : {10, 0, 20, 30}) require(immediate.insert(key), "insert immediate fixture");
    require(immediate.erase(10) && immediate.totalDepth() == 2 && immediate.validate(),
            "immediate successor replacement must account for its right child");
}

void testInsertionRebuild(Mode mode) {
    ScapegoatTree tree(mode);
    for (Key key = 1; key < 5; ++key) {
        require(tree.insert(key) && tree.rebuildCount() == 0, "unexpected early rebuild");
    }
    const bool inserted = tree.insert(5);
    require(inserted && tree.rebuildCount() == 1 && tree.totalDepth() == 8 && tree.validate(),
            "fifth sorted insertion must rebuild the four-node subtree");
    require(ScapegoatTreeTestAccess::shape(tree).front() == 1 &&
                ScapegoatTreeTestAccess::isPerfect(tree, true),
            "insertion rebuild must preserve the root and perfectly balance its right child");
}

void testValidationHeight(Mode mode) {
    ScapegoatTree tree(mode);
    for (Key key : {10, 20, 30, 40, 5, 50}) {
        require(tree.insert(key), "height fixture insertion failed");
    }
    require(tree.erase(5) && tree.size() == 5 && tree.totalDepth() == 10 &&
                tree.rebuildCount() == 0 && tree.validate(),
            "the five-node chain is valid with a high-water mark of six");

    ScapegoatTreeTestAccess::setHighWaterMarkToLiveSize(tree);
    require(!tree.validate(), "the same chain exceeds the height bound at a high-water mark of five");
}

void insertBalanced(churn::Dictionary& tree, Key first, Key last) {
    // Insert medians in level order so the fixture starts without rebuilds.
    std::vector<std::pair<Key, Key>> ranges{{first, last}};
    for (std::size_t i = 0; i < ranges.size(); ++i) {
        const auto [low, high] = ranges[i];
        const auto middle = low + (high - low) / 2;
        require(tree.insert(middle), "balanced fixture insertion failed");
        if (low < middle) ranges.emplace_back(low, middle);
        if (middle + 1 < high) ranges.emplace_back(middle + 1, high);
    }
}

void testPerfectRebuildSizes() {
    for (Key remaining = 1; remaining <= 260; ++remaining) {
        ScapegoatTree array(Mode::Array);
        ScapegoatTree inPlace(Mode::InPlace);
        const Key highWater = 3 * (remaining + 1) / 2;
        for (auto* tree : {&array, &inPlace}) {
            insertBalanced(*tree, 0, highWater);
            for (Key key = highWater - 1; key >= remaining; --key) {
                require(tree->rebuildCount() == 0, "rebuild fired before crossing shrink threshold");
                require(tree->erase(key) && tree->validate(), "shrink fixture erase failed");
            }
            require(tree->rebuildCount() == 1 && tree->size() == static_cast<std::size_t>(remaining),
                    "crossing shrink threshold must cause exactly one rebuild");
            require(ScapegoatTreeTestAccess::isPerfect(*tree), "rebuilt subtree is not perfect");
        }
        require(ScapegoatTreeTestAccess::shape(array) == ScapegoatTreeTestAccess::shape(inPlace),
                "the two rebuilders must yield the same shape");
    }
}

void testMixedUpdates() {
    ScapegoatTree array(Mode::Array);
    ScapegoatTree inPlace(Mode::InPlace);
    churn::SortedArrayOracle oracle;
    std::mt19937_64 rng(20261117);
    std::uniform_int_distribution<Key> keyDistribution(-511, 512);
    for (int step = 1; step <= 100'000; ++step) {
        const auto key = keyDistribution(rng);
        switch (rng() % 3) {
        case 0: {
            const bool inserted = oracle.insert(key);
            require(array.insert(key) == inserted && inPlace.insert(key) == inserted,
                    "insert disagrees with the oracle");
            break;
        }
        case 1: {
            const bool erased = oracle.erase(key);
            require(array.erase(key) == erased && inPlace.erase(key) == erased,
                    "erase disagrees with the oracle");
            break;
        }
        default:
            require(array.contains(key) == oracle.contains(key) &&
                        inPlace.contains(key) == oracle.contains(key),
                    "lookup disagrees with the oracle");
        }
        require(array.size() == oracle.size() && inPlace.size() == oracle.size(),
                "size disagrees with the oracle");
        require(array.totalDepth() == inPlace.totalDepth() &&
                    array.rebuildCount() == inPlace.rebuildCount(),
                "the rebuild modes diverged on the same update sequence");
        if (step % 250 == 0) {
            std::vector<Key> expected;
            oracle.inorder(expected);
            for (const auto* tree : {&array, &inPlace}) {
                std::vector<Key> keys;
                tree->inorder(keys);
                require(tree->validate() && keys == expected,
                        "checkpoint failed structure or ordered-contents validation");
            }
        }
    }
}

void testMonotoneInsertions() {
    for (Key direction : {-1, 1}) {
        ScapegoatTree array(Mode::Array);
        ScapegoatTree inPlace(Mode::InPlace);
        for (Key value = 1; value <= 4000; ++value) {
            require(array.insert(direction * value) && inPlace.insert(direction * value),
                    "monotone insertion failed");
            require(array.totalDepth() == inPlace.totalDepth() &&
                        array.rebuildCount() == inPlace.rebuildCount(),
                    "rebuild modes diverged on monotone insertions");
        }
        require(array.validate() && inPlace.validate() && array.rebuildCount() > 0,
                "monotone insertion must trigger valid rebuilds");
    }
}

}

int main() {
    try {
        require(ScapegoatTreeTestAccess::depthLimit(4) == 3 &&
                    ScapegoatTreeTestAccess::depthLimit(5) == 3 &&
                    ScapegoatTreeTestAccess::depthLimit(6) == 4,
                "depth thresholds must apply exact ceiling boundaries");
        for (auto mode : {Mode::Array, Mode::InPlace}) {
            testUpdateAccounting(mode);
            testInsertionRebuild(mode);
            testValidationHeight(mode);
        }
        testPerfectRebuildSizes();
        testMixedUpdates();
        testMonotoneInsertions();
        std::cout << "Scapegoat tests passed\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
