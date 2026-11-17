#include "scapegoat.hpp"

#include <algorithm>
#include <array>
#include <cassert>

namespace churn {
namespace {

// Exact ceil((3/2)^d) thresholds through the 32-bit key range.
constexpr auto makeDepthThresholds() {
    std::array<std::uint64_t, 56> thresholds{};
    std::uint64_t whole = 1;
    std::uint64_t numerator = 0;
    std::uint64_t denominator = 1;
    for (auto& threshold : thresholds) {
        threshold = whole + (numerator != 0);
        const auto scaledWhole = 3 * whole;
        numerator = 3 * numerator + (scaledWhole % 2) * denominator;
        denominator *= 2;
        whole = scaledWhole / 2 + numerator / denominator;
        numerator %= denominator;
    }
    return thresholds;
}

constexpr auto depthThresholds = makeDepthThresholds();

bool tooHeavy(std::uint64_t child, std::uint64_t parent) noexcept {
    return 3 * child > 2 * parent;
}

// Both rebuilds use the Stout–Warren bottom-level layout.
struct PerfectShape {
    explicit PerfectShape(std::uint64_t count) noexcept {
        while (2 * slots <= count + 1) slots *= 2;
        holes = 2 * slots - 1 - count;
    }

    std::uint64_t holesThrough(std::uint64_t position) const noexcept {
        if (position == slots) return holes;
        return ((position + 1) * holes - 1) / slots;
    }

    std::uint64_t nodesIn(std::uint64_t offset,
                          std::uint64_t width) const noexcept {
        return 2 * width - 1 - (holesThrough(offset + width) - holesThrough(offset));
    }

    std::uint64_t holePosition(std::uint64_t index) const noexcept {
        return index * slots / holes;
    }

    std::uint64_t slots = 1;
    std::uint64_t holes = 0;
};

}

ScapegoatTree::ScapegoatTree(RebuildMode mode) noexcept : mode_(mode) {}

ScapegoatTree::~ScapegoatTree() { destroy(root_); }

bool ScapegoatTree::insert(Key key) {
    std::vector<Node**> ancestors;
    Node** link = &root_;
    while (*link) {
        Node* node = *link;
        if (key == node->key) return false;
        ancestors.push_back(link);
        link = key < node->key ? &node->left : &node->right;
    }

    const auto newSize = size() + 1;
    const auto depth = ancestors.size();
    Node** scapegoat = nullptr;
    std::vector<Node*> scratch;
    if (depth > depthLimit(newSize)) {
        // Check projected sizes after inserting the leaf.
        std::size_t childSize = 1;
        for (auto it = ancestors.rbegin(); it != ancestors.rend(); ++it) {
            const auto parentSize = (**it)->size + 1;
            if (tooHeavy(childSize, parentSize)) {
                scapegoat = *it;
                if (mode_ == RebuildMode::Array) scratch.reserve(parentSize);
                break;
            }
            childSize = parentSize;
        }
        assert(scapegoat != nullptr);
    }

    *link = new Node{key};
    for (Node** ancestor : ancestors) ++(*ancestor)->size;
    maxSize_ = std::max(maxSize_, newSize);
    totalDepth_ += depth;
    if (scapegoat) rebuild(*scapegoat, scratch);
    return true;
}

bool ScapegoatTree::erase(Key key) {
    std::vector<Node*> ancestors;
    Node** link = &root_;
    while (*link && (*link)->key != key) {
        Node* node = *link;
        ancestors.push_back(node);
        link = key < node->key ? &node->left : &node->right;
    }
    if (!*link) return false;

    Node* target = *link;
    if (target->left && target->right) {
        ancestors.push_back(target);
        link = &target->right;
        while ((*link)->left) {
            ancestors.push_back(*link);
            link = &(*link)->left;
        }
    }

    const auto newSize = size() - 1;
    const bool shrink = newSize != 0 &&
        3 * std::uint64_t{newSize} < 2 * std::uint64_t{maxSize_};
    std::vector<Node*> scratch;
    if (shrink && mode_ == RebuildMode::Array) scratch.reserve(newSize);

    Node* removed = *link;
    Node* promoted = removed->left ? removed->left : removed->right;
    target->key = removed->key;
    *link = promoted;
    totalDepth_ -= ancestors.size() + nodeSize(promoted);
    for (Node* ancestor : ancestors) --ancestor->size;
    delete removed;

    if (shrink) rebuild(root_, scratch);
    if (!root_) maxSize_ = 0;
    return true;
}

bool ScapegoatTree::contains(Key key) const {
    const Node* node = root_;
    while (node && node->key != key) {
        node = key < node->key ? node->left : node->right;
    }
    return node != nullptr;
}

std::size_t ScapegoatTree::size() const { return nodeSize(root_); }
std::uint64_t ScapegoatTree::totalDepth() const { return totalDepth_; }
std::uint64_t ScapegoatTree::rebuildCount() const { return rebuildCount_; }

void ScapegoatTree::inorder(std::vector<Key>& out) const {
    appendInorder(root_, out);
}

bool ScapegoatTree::validate() const {
    if (!root_) return maxSize_ == 0 && totalDepth_ == 0;
    if (size() > maxSize_ || 3 * std::uint64_t{size()} < 2 * std::uint64_t{maxSize_}) {
        return false;
    }
    std::size_t count = 0;
    std::uint64_t depth = 0;
    return validateSubtree(root_, nullptr, nullptr, 0, depthLimit(maxSize_),
                           count, depth) && count == size() && depth == totalDepth_;
}

std::string ScapegoatTree::name() const {
    return mode_ == RebuildMode::Array ? "scapegoat_array" : "scapegoat_inplace";
}

std::size_t ScapegoatTree::nodeSize(const Node* node) noexcept {
    return node ? node->size : 0;
}

std::size_t ScapegoatTree::depthLimit(std::size_t size) noexcept {
    assert(size > 0);
    const auto end = std::upper_bound(depthThresholds.begin(), depthThresholds.end(), size);
    return static_cast<std::size_t>(end - depthThresholds.begin() - 1);
}

void ScapegoatTree::destroy(Node* node) noexcept {
    if (!node) return;
    destroy(node->left);
    destroy(node->right);
    delete node;
}

void ScapegoatTree::appendInorder(const Node* node, std::vector<Key>& out) {
    if (!node) return;
    appendInorder(node->left, out);
    out.push_back(node->key);
    appendInorder(node->right, out);
}

bool ScapegoatTree::validateSubtree(const Node* node, const Node* lower,
                                   const Node* upper, std::size_t depth,
                                   std::size_t heightLimit, std::size_t& count,
                                   std::uint64_t& totalDepth) noexcept {
    if (!node) return true;
    if (depth > heightLimit || (lower && node->key <= lower->key) ||
        (upper && node->key >= upper->key)) return false;

    std::size_t leftSize = 0;
    std::size_t rightSize = 0;
    if (!validateSubtree(node->left, lower, node, depth + 1, heightLimit,
                         leftSize, totalDepth) ||
        !validateSubtree(node->right, node, upper, depth + 1, heightLimit,
                         rightSize, totalDepth)) return false;

    count = 1 + leftSize + rightSize;
    totalDepth += depth;
    return count == node->size;
}

void ScapegoatTree::collectNodes(Node* node, std::vector<Node*>& nodes,
                                std::uint64_t depth, std::uint64_t& totalDepth) {
    if (!node) return;
    collectNodes(node->left, nodes, depth + 1, totalDepth);
    nodes.push_back(node);
    totalDepth += depth;
    collectNodes(node->right, nodes, depth + 1, totalDepth);
}

void ScapegoatTree::rebuild(Node*& root, std::vector<Node*>& scratch) noexcept {
    const auto count = nodeSize(root);
    if (mode_ == RebuildMode::Array) {
        const PerfectShape shape(count);
        std::uint64_t oldDepth = 0;
        collectNodes(root, scratch, 0, oldDepth);
        totalDepth_ -= oldDepth;

        // Choose the median that matches the in-place layout.
        const auto build = [&](const auto& self, std::size_t first, std::size_t size,
                               std::uint64_t offset, std::uint64_t width,
                               std::uint64_t depth) -> Node* {
            if (size == 0) return nullptr;
            const auto half = width / 2;
            const auto leftSize = half == 0 ? 0 :
                static_cast<std::size_t>(shape.nodesIn(offset, half));
            Node* node = scratch[first + leftSize];
            node->left = self(self, first, leftSize, offset, half, depth + 1);
            node->right = self(self, first + leftSize + 1, size - leftSize - 1,
                               offset + half, half, depth + 1);
            node->size = size;
            totalDepth_ += depth;
            return node;
        };
        root = build(build, 0, count, 0, shape.slots, 0);
    } else {
        rebuildInPlace(root);
    }
    ++rebuildCount_;
    if (&root == &root_) maxSize_ = count;
}

void ScapegoatTree::rebuildInPlace(Node*& root) noexcept {
    const PerfectShape shape(nodeSize(root));

    // Rotate away left links to form a vine.
    Node** link = &root;
    while (*link) {
        if ((*link)->left) rotateRight(*link);
        else link = &(*link)->right;
    }

    // Spread bottom leaves evenly, leaving the final slot empty.
    std::uint64_t holeIndex = 1;
    link = &root;
    for (std::uint64_t slot = 1; slot < shape.slots; ++slot) {
        if (slot == shape.holePosition(holeIndex)) ++holeIndex;
        else rotateLeft(*link);
        link = &(*link)->right;
    }

    // Compress the spine of length 2^k - 1.
    for (auto spine = shape.slots - 1; spine > 1; spine /= 2) {
        link = &root;
        for (std::uint64_t i = 0; i < spine / 2; ++i) {
            rotateLeft(*link);
            link = &(*link)->right;
        }
    }
}

void ScapegoatTree::rotateLeft(Node*& root) noexcept {
    Node* down = root;
    Node* up = down->right;
    // The outside subtrees shift by one level in opposite directions.
    totalDepth_ += nodeSize(down->left);
    totalDepth_ -= nodeSize(up->right);
    up->size = down->size;
    down->right = up->left;
    down->size = 1 + nodeSize(down->left) + nodeSize(down->right);
    up->left = down;
    root = up;
}

void ScapegoatTree::rotateRight(Node*& root) noexcept {
    Node* down = root;
    Node* up = down->left;
    totalDepth_ += nodeSize(down->right);
    totalDepth_ -= nodeSize(up->left);
    up->size = down->size;
    down->left = up->right;
    down->size = 1 + nodeSize(down->left) + nodeSize(down->right);
    up->right = down;
    root = up;
}

}
