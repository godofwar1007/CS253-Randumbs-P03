#include "hibbard.hpp"

#include <vector>

namespace churn {

HibbardBst::HibbardBst(HibbardPolicy policy, Rng& rng)
    : policy_(policy), rng_(rng) {}

HibbardBst::~HibbardBst() {
    // Iterative cleanup also works for a highly skewed tree.
    std::vector<Node*> pending;
    if (root_) pending.push_back(root_);
    while (!pending.empty()) {
        Node* node = pending.back();
        pending.pop_back();
        if (node->left) pending.push_back(node->left);
        if (node->right) pending.push_back(node->right);
        delete node;
    }
}

bool HibbardBst::insert(Key k) {
    Node* x = root_;
    Node* y = nullptr;
    std::vector<Node*> path;
    std::uint64_t depth = 0;

    while (x) {
        if (k == x->key) return false;
        y = x;
        path.push_back(x);
        x = k < x->key ? x->left : x->right;
        ++depth;
    }

    Node* z = new Node(k);
    if (y == nullptr) root_ = z;
    else if (k < y->key) y->left = z;
    else y->right = z;

    for (Node* x : path) ++x->subtreeSize;
    ++size_;
    totalDepth_ += depth;
    return true;
}

bool HibbardBst::erase(Key k) {
    Node** link = &root_;
    Node* x = root_;
    std::vector<Node*> path;
    std::uint64_t depth = 0;

    while (x && x->key != k) {
        path.push_back(x);
        link = k < x->key ? &x->left : &x->right;
        x = *link;
        ++depth;
    }
    if (x == nullptr) return false;

    path.push_back(x);

    if (x->left == nullptr || x->right == nullptr) {
        Node* child = x->left ? x->left : x->right;
        const std::uint64_t childSize = child ? child->subtreeSize : 0;
        totalDepth_ -= depth + childSize;
        *link = child;
        path.pop_back();  // x is removed, so only its surviving ancestors remain.
        for (Node* y : path) --y->subtreeSize;
        delete x;
    } else {
        const bool usePredecessor =
            policy_ == HibbardPolicy::Random && uniformInt(rng_, 0, 1) == 0;

        Node** successorLink = usePredecessor ? &x->left : &x->right;
        std::vector<Node*> successorAncestors;
        while (usePredecessor ? (*successorLink)->right : (*successorLink)->left) {
            successorAncestors.push_back(*successorLink);
            Node* y = *successorLink;
            successorLink = usePredecessor ? &y->right : &y->left;
        }

        Node* s = *successorLink;
        Node* child = usePredecessor ? s->left : s->right;
        const std::uint64_t successorDepth = depth + 1 + successorAncestors.size();
        const std::uint64_t childSize = child ? child->subtreeSize : 0;

        // Removing the replacement subtracts its old depth. Its one possible
        // child moves up one level, subtracting one more per node in that subtree.
        totalDepth_ -= successorDepth + childSize;
        x->key = s->key;
        *successorLink = child;
        delete s;

        for (Node* y : successorAncestors) --y->subtreeSize;
        for (Node* y : path) --y->subtreeSize;
    }

    --size_;
    return true;
}

bool HibbardBst::contains(Key key) const {
    const Node* node = root_;
    while (node && node->key != key) {
        node = key < node->key ? node->left : node->right;
    }
    return node != nullptr;
}

std::size_t HibbardBst::size() const { return size_; }

std::uint64_t HibbardBst::totalDepth() const { return totalDepth_; }

std::uint64_t HibbardBst::rebuildCount() const { return 0; }

void HibbardBst::inorder(std::vector<Key>& out) const {
    std::vector<const Node*> stack;
    const Node* x = root_;
    while (x || !stack.empty()) {
        while (x) {
            stack.push_back(x);
            x = x->left;
        }
        x = stack.back();
        stack.pop_back();
        out.push_back(x->key);
        x = x->right;
    }
}

bool HibbardBst::validate() const {
    struct Frame {
        const Node* node;
        bool hasLower;
        Key lower;
        bool hasUpper;
        Key upper;
        std::uint64_t depth;
    };

    std::vector<Frame> pending;
    if (root_) {
        pending.push_back({root_, false, Key{}, false, Key{}, 0});
    }

    std::size_t countedNodes = 0;
    std::uint64_t countedDepth = 0;
    while (!pending.empty()) {
        const Frame frame = pending.back();
        pending.pop_back();
        const Node* node = frame.node;

        if ((frame.hasLower && node->key <= frame.lower) ||
            (frame.hasUpper && node->key >= frame.upper)) {
            return false;
        }

        const std::size_t expectedSize = 1 +
            (node->left ? node->left->subtreeSize : 0) +
            (node->right ? node->right->subtreeSize : 0);
        if (node->subtreeSize != expectedSize) return false;

        ++countedNodes;
        countedDepth += frame.depth;

        if (node->left) {
            pending.push_back({node->left, frame.hasLower, frame.lower,
                               true, node->key, frame.depth + 1});
        }
        if (node->right) {
            pending.push_back({node->right, true, node->key,
                               frame.hasUpper, frame.upper, frame.depth + 1});
        }
    }

    return countedNodes == size_ && countedDepth == totalDepth_ &&
           (root_ ? root_->subtreeSize : 0) == size_;
}

std::string HibbardBst::name() const {
    return policy_ == HibbardPolicy::Successor ? "hibbard_succ" : "hibbard_rand";
}

}  // namespace churn
