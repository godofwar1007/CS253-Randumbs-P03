#include "harness.hpp"
#include "trees.hpp"

#include <iostream>

using namespace churn;

void require(bool passed, const std::string& message) {
    if (!passed) throw std::runtime_error(message);
}

namespace churn {

struct HibbardBstTestAccess {
    static void check(HibbardBst& tree) {
        ++tree.root_->subtreeSize;
        require(!tree.validate(), "Hibbard checker missed incorrect subtree size");
        --tree.root_->subtreeSize;
        ++tree.size_;
        require(!tree.validate(), "Hibbard checker missed incorrect node count");
        --tree.size_;
        ++tree.totalDepth_;
        require(!tree.validate(), "Hibbard checker missed incorrect depth sum");
        --tree.totalDepth_;
        Key saved = tree.root_->left->key;
        tree.root_->left->key = tree.root_->key;
        require(!tree.validate(), "Hibbard checker missed incorrect ordering");
        tree.root_->left->key = saved;
        require(tree.validate(), "Hibbard fixture was not restored");
    }
};

struct ScapegoatTreeTestAccess {
    static void check(ScapegoatTree& tree) {
        ++tree.root_->size;
        require(!tree.validate(), "Scapegoat checker missed incorrect subtree size");
        --tree.root_->size;
        ++tree.totalDepth_;
        require(!tree.validate(), "Scapegoat checker missed incorrect depth sum");
        --tree.totalDepth_;
        std::size_t savedSize = tree.maxSize_;
        tree.maxSize_ = 0;
        require(!tree.validate(), "Scapegoat checker missed incorrect high-water size");
        tree.maxSize_ = savedSize;
        Key savedKey = tree.root_->left->key;
        tree.root_->left->key = tree.root_->key;
        require(!tree.validate(), "Scapegoat checker missed incorrect ordering");
        tree.root_->left->key = savedKey;
        require(tree.validate(), "Scapegoat fixture was not restored");
    }
};

}

void checkUpdates(const std::string& name) {
    Rng rng(42);
    auto tree = makeTestTree(name, rng);
    SortedArrayOracle oracle;
    auto verify = [&]() {
        require(checkState(*tree, oracle).empty(), name + " failed state check");
    };
    for (Key key : {20, 10, 30, 25, 27, 5, INT_MIN, INT_MAX}) {
        require(tree->insert(key) == oracle.insert(key), name + " insert mismatch");
        verify();
    }
    auto depth = tree->totalDepth();
    auto rebuilds = tree->rebuildCount();
    require(!tree->insert(20) && !tree->erase(999) && tree->contains(20),
            name + " duplicate, missing-key or contains check failed");
    require(tree->totalDepth() == depth && tree->rebuildCount() == rebuilds,
            name + " unsuccessful update changed depth or rebuild count");
    for (Key key : {20, 10, 5, INT_MIN, INT_MAX, 25, 27, 30}) {
        require(tree->erase(key) == oracle.erase(key), name + " erase mismatch");
        verify();
    }
    require(tree->insert(1) && tree->erase(1) && tree->size() == 0 && tree->validate(),
            name + " single-root deletion failed");
    for (int key = 0; key < 512; ++key) {
        tree->insert(key);
        oracle.insert(key);
        verify();
    }
    if (name.find("scapegoat") == 0)
        require(tree->rebuildCount() > rebuilds, name + " did not rebuild on sorted insertion");
    rebuilds = tree->rebuildCount();
    for (int key = 511; key >= 0; --key) {
        require(tree->erase(key) && oracle.erase(key), name + " drain failed");
        verify();
    }
    if (name.find("scapegoat") == 0)
        require(tree->rebuildCount() > rebuilds, name + " did not rebuild on deletion shrinkage");
}

int main() {
    try {
        for (const std::string name : {"hibbard_succ", "hibbard_rand",
                                       "scapegoat_array", "scapegoat_inplace"}) {
            checkUpdates(name);
        }
        Rng rng(42);
        for (auto policy : {HibbardPolicy::Successor, HibbardPolicy::Random}) {
            HibbardBst tree(policy, rng);
            for (int key : {20, 10, 30}) tree.insert(key);
            HibbardBstTestAccess::check(tree);
        }
        for (auto mode : {ScapegoatTree::RebuildMode::Array, ScapegoatTree::RebuildMode::InPlace}) {
            ScapegoatTree tree(mode);
            for (int key : {20, 10, 30}) tree.insert(key);
            ScapegoatTreeTestAccess::check(tree);
        }
        std::cout << "ORACLE CHECKS PASSED: deletion cases, rebuilds and corrupted metadata\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "ORACLE CHECKS FAILED: " << e.what() << '\n';
        return 1;
    }
}
