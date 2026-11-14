#pragma once
// Temporary test tree. Delete once the real structures exist.

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "dictionary.hpp"

namespace churn {

class StandinBst : public Dictionary {
    struct Node {
        Key key;
        Node* l = nullptr;
        Node* r = nullptr;
        explicit Node(Key k) : key(k) {}
    };
    Node* root_ = nullptr;
    std::size_t n_ = 0;

public:
    StandinBst() = default;
    StandinBst(const StandinBst&) = delete;
    StandinBst& operator=(const StandinBst&) = delete;
    ~StandinBst() override {
        std::vector<Node*> st;
        if (root_) st.push_back(root_);
        while (!st.empty()) {
            Node* x = st.back();
            st.pop_back();
            if (x->l) st.push_back(x->l);
            if (x->r) st.push_back(x->r);
            delete x;
        }
    }

    bool insert(Key k) override {
        Node** link = &root_;
        while (*link) {
            if (k == (*link)->key) return false;
            link = k < (*link)->key ? &(*link)->l : &(*link)->r;
        }
        *link = new Node(k);
        ++n_;
        return true;
    }

    bool erase(Key k) override {
        Node** link = &root_;
        while (*link && (*link)->key != k)
            link = k < (*link)->key ? &(*link)->l : &(*link)->r;
        if (!*link) return false;
        Node* z = *link;
        if (!z->l) {
            *link = z->r;
            delete z;
        } else if (!z->r) {
            *link = z->l;
            delete z;
        } else {
            Node** sl = &z->r;
            while ((*sl)->l) sl = &(*sl)->l;
            Node* s = *sl;
            *sl = s->r;
            z->key = s->key;
            delete s;
        }
        --n_;
        return true;
    }

    bool contains(Key k) const override {
        const Node* x = root_;
        while (x && x->key != k) x = k < x->key ? x->l : x->r;
        return x != nullptr;
    }

    std::size_t size() const override { return n_; }

    std::uint64_t totalDepth() const override {
        std::uint64_t sum = 0;
        std::vector<std::pair<const Node*, std::uint64_t>> st;
        if (root_) st.emplace_back(root_, 0);
        while (!st.empty()) {
            auto [x, d] = st.back();
            st.pop_back();
            sum += d;
            if (x->l) st.emplace_back(x->l, d + 1);
            if (x->r) st.emplace_back(x->r, d + 1);
        }
        return sum;
    }

    std::uint64_t rebuildCount() const override { return 0; }

    void inorder(std::vector<Key>& out) const override {
        std::vector<const Node*> st;
        const Node* x = root_;
        while (x || !st.empty()) {
            while (x) {
                st.push_back(x);
                x = x->l;
            }
            x = st.back();
            st.pop_back();
            out.push_back(x->key);
            x = x->r;
        }
    }

    bool validate() const override {
        std::vector<Key> v;
        inorder(v);
        if (v.size() != n_) return false;
        for (std::size_t i = 1; i < v.size(); ++i)
            if (v[i - 1] >= v[i]) return false;
        return true;
    }

    std::string name() const override { return "standin_bst"; }
};

}  // namespace churn