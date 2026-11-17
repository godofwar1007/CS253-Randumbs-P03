#pragma once

#include "dictionary.hpp"

namespace churn {

class ScapegoatTree final : public Dictionary {
public:
    enum class RebuildMode { Array, InPlace };

    explicit ScapegoatTree(RebuildMode mode) noexcept;
    ~ScapegoatTree() override;

    ScapegoatTree(const ScapegoatTree&) = delete;
    ScapegoatTree& operator=(const ScapegoatTree&) = delete;

    bool insert(Key key) override;
    bool erase(Key key) override;
    bool contains(Key key) const override;
    std::size_t size() const override;
    std::uint64_t totalDepth() const override;
    std::uint64_t rebuildCount() const override;
    void inorder(std::vector<Key>& out) const override;
    bool validate() const override;
    std::string name() const override;

private:
    struct Node {
        Key key;
        Node* left = nullptr;
        Node* right = nullptr;
        std::size_t size = 1;
    };

    friend struct ScapegoatTreeTestAccess;

    static std::size_t nodeSize(const Node* node) noexcept;
    static std::size_t depthLimit(std::size_t size) noexcept;
    static void destroy(Node* node) noexcept;
    static void appendInorder(const Node* node, std::vector<Key>& out);
    static bool validateSubtree(const Node* node, const Node* lower,
                                const Node* upper, std::size_t depth,
                                std::size_t heightLimit, std::size_t& count,
                                std::uint64_t& totalDepth) noexcept;
    static void collectNodes(Node* node, std::vector<Node*>& nodes,
                             std::uint64_t depth, std::uint64_t& totalDepth);

    void rebuild(Node*& root, std::vector<Node*>& scratch) noexcept;
    void rebuildInPlace(Node*& root) noexcept;
    void rotateLeft(Node*& root) noexcept;
    void rotateRight(Node*& root) noexcept;

    Node* root_ = nullptr;
    RebuildMode mode_;
    std::size_t maxSize_ = 0;
    std::uint64_t totalDepth_ = 0;
    std::uint64_t rebuildCount_ = 0;
};

}
