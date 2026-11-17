#pragma once

#include "dictionary.hpp"
#include "rng.hpp"
#include "sorted_array.hpp"

#include <algorithm>
#include <climits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

namespace churn {

struct Operation {
    string type;
    Key key;
};

struct TestResult {
    string error;
    size_t step = 0;
};

using TreeMaker = unique_ptr<Dictionary> (*)(const string&, Rng&);

inline vector<Operation> makeOperations(unsigned long long seed, size_t count) {
    vector<Operation> ops;
    for (Key key : {0, INT_MIN, INT_MAX, -1, 1}) {
        ops.push_back({"contains", key});
        ops.push_back({"erase", key});
        ops.push_back({"insert", key});
        ops.push_back({"insert", key});
        ops.push_back({"contains", key});
    }
    for (Key key : {0, INT_MIN, INT_MAX, -1, 1}) {
        ops.push_back({"erase", key});
        ops.push_back({"erase", key});
    }
    for (int key = 0; key < 32; ++key) ops.push_back({"insert", key});
    for (int key = 31; key >= 0; --key) ops.push_back({"erase", key});
    for (int key = 31; key >= 0; --key) ops.push_back({"insert", key});
    for (int key = 0; key < 32; ++key) ops.push_back({"erase", key});
    if (ops.size() > count) ops.resize(count);
    ops.reserve(count);
    Rng rng(seed);
    while (ops.size() < count) {
        int choice = static_cast<int>(rng() % 10);
        Key key = static_cast<Key>(rng() % 257) - 128;
        string type = "contains";
        if (choice < 4) type = "insert";
        else if (choice < 8) type = "erase";
        ops.push_back({type, key});
    }
    return ops;
}

inline string checkState(const Dictionary& tree, const SortedArrayOracle& oracle) {
    if (!oracle.validate()) return "oracle ordering is invalid";
    if (!tree.validate()) return "tree invariant check failed";
    if (tree.size() != oracle.size()) return "size mismatch";
    vector<Key> expected{INT_MIN};
    vector<Key> actual{INT_MIN};
    oracle.inorder(expected);
    tree.inorder(actual);
    if (actual != expected) return "sorted contents or append behaviour mismatch";
    if (tree.size() == 0 && tree.totalDepth() != 0)
        return "empty tree has nonzero total depth";
    return "";
}

inline TestResult runTest(const string& name, const vector<Operation>& ops,
                          unsigned long long seed, TreeMaker makeTree) {
    size_t step = 0;
    try {
        Rng rng(seed);
        auto tree = makeTree(name, rng);
        if (!tree) throw runtime_error("tree is not connected: " + name);
        SortedArrayOracle oracle;
        if (tree->name() != name) return {"tree name mismatch", 0};
        string error = checkState(*tree, oracle);
        if (!error.empty()) return {error, 0};
        unsigned long long rebuilds = tree->rebuildCount();
        if (rebuilds != 0) return {"new tree has nonzero rebuild count", 0};
        for (const Operation& op : ops) {
            ++step;
            bool expected, actual;
            if (op.type == "insert") {
                expected = oracle.insert(op.key);
                actual = tree->insert(op.key);
            } else if (op.type == "erase") {
                expected = oracle.erase(op.key);
                actual = tree->erase(op.key);
            } else if (op.type == "contains") {
                expected = oracle.contains(op.key);
                actual = tree->contains(op.key);
            } else {
                throw runtime_error("invalid operation: " + op.type);
            }
            if (actual != expected) return {op.type + " result mismatch", step};
            if (tree->contains(op.key) != oracle.contains(op.key))
                return {"membership mismatch", step};
            error = checkState(*tree, oracle);
            if (!error.empty()) return {error, step};
            if (tree->rebuildCount() < rebuilds)
                return {"rebuild count decreased", step};
            rebuilds = tree->rebuildCount();
            if ((name == "hibbard_succ" || name == "hibbard_rand") && rebuilds != 0)
                return {"Hibbard tree reported a rebuild", step};
        }
    } catch (const exception& e) {
        return {string("exception: ") + e.what(), step};
    }
    return {};
}

inline vector<Operation> shrinkFailure(const string& name,
        vector<Operation> ops, unsigned long long seed, TreeMaker makeTree,
        const string& error) {
    if (error.empty() || runTest(name, ops, seed, makeTree).error != error)
        throw runtime_error("failure did not reproduce");
    if (runTest(name, {}, seed, makeTree).error == error) return {};
    size_t chunk = max<size_t>(1, ops.size() / 2);
    while (!ops.empty()) {
        bool removed = false;
        for (size_t start = 0; start < ops.size(); start += chunk) {
            size_t end = min(ops.size(), start + chunk);
            vector<Operation> shorter = ops;
            shorter.erase(shorter.begin() + start, shorter.begin() + end);
            if (runTest(name, shorter, seed, makeTree).error == error) {
                ops.swap(shorter);
                removed = true;
                break;
            }
        }
        if (removed) continue;
        if (chunk == 1) break;
        chunk = max<size_t>(1, chunk / 2);
    }
    return ops;
}

}
