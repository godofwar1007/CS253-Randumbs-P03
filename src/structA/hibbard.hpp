#pragma once

#include "../../include/dictionary.hpp"
#include "../../include/rng.hpp"

namespace churn {

enum class HibbardPolicy { Successor, Random };

class HibbardBst final : public Dictionary {
public:
    HibbardBst(HibbardPolicy policy, Rng& rng);

    HibbardBst(const HibbardBst&) = delete;
    HibbardBst& operator=(const HibbardBst&) = delete;
    ~HibbardBst() override;

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
    friend struct HibbardBstTestAccess;

    struct Node {
        explicit Node(Key value) : key(value) {}

        Key key;
        Node* left = nullptr;
        Node* right = nullptr;
        std::size_t subtreeSize = 1;
    };

    Node* root_ = nullptr;
    std::size_t size_ = 0;
    std::uint64_t totalDepth_ = 0;
    HibbardPolicy policy_;
    Rng& rng_;
};

}
