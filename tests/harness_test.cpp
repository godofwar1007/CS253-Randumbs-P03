#include "harness.hpp"

#include <iostream>
#include <set>

using namespace churn;

class TestSet : public Dictionary {
    std::set<Key> keys;
    std::string mode;

public:
    explicit TestSet(const std::string& name) : mode(name) {}

    bool insert(Key key) override { return keys.insert(key).second; }

    bool erase(Key key) override {
        if (mode == "bad_erase" && key == 7 && contains(8)) return false;
        return keys.erase(key) != 0;
    }

    bool contains(Key key) const override { return keys.count(key) != 0; }
    std::size_t size() const override { return keys.size(); }
    std::uint64_t totalDepth() const override { return 0; }
    std::uint64_t rebuildCount() const override { return 0; }

    void inorder(std::vector<Key>& out) const override {
        if (mode == "bad_append") out.clear();
        out.insert(out.end(), keys.begin(), keys.end());
    }

    bool validate() const override {
        return mode != "bad_invariant" || !contains(9);
    }

    std::string name() const override { return mode; }
};

std::unique_ptr<Dictionary> makeSet(const std::string& name, Rng&) {
    return std::unique_ptr<Dictionary>(new TestSet(name));
}

void check(bool passed, const std::string& message) {
    if (!passed) throw std::runtime_error(message);
}

int main() {
    try {
        std::uint64_t seed = 20260928;
        std::vector<Operation> ops = makeOperations(seed, 1000000);
        check(ops.size() == 1000000, "wrong operation count");
        check(runTest("test_set", ops, seed, makeSet).error.empty(), "reference test failed");
        std::vector<Operation> same = makeOperations(seed, 1000000);
        check(ops.size() == same.size(), "wrong operation count");
        for (std::size_t i = 0; i < ops.size(); ++i)
            check(ops[i].type == same[i].type && ops[i].key == same[i].key,
                  "seed did not reproduce operations");
        std::vector<Operation> failing = {
            {"insert", 100}, {"contains", 100}, {"insert", 7},
            {"insert", 8}, {"contains", -1}, {"erase", 7}, {"erase", 100}};
        TestResult result = runTest("bad_erase", failing, seed, makeSet);
        check(result.error == "erase result mismatch", "missed broken deletion");
        std::vector<Operation> small = shrinkFailure("bad_erase", failing, seed, makeSet, result.error);
        check(small.size() == 3, "shrinking kept unnecessary operations");
        check(runTest("bad_erase", small, seed, makeSet).error == result.error,
              "small sequence did not reproduce");
        for (std::size_t i = 0; i < small.size(); ++i) {
            std::vector<Operation> shorter = small;
            shorter.erase(shorter.begin() + i);
            check(runTest("bad_erase", shorter, seed, makeSet).error != result.error,
                  "small sequence is not deletion-minimal");
        }
        check(runTest("bad_invariant", {{"insert", 9}}, seed, makeSet).error ==
              "tree invariant check failed", "missed broken invariant");
        result = runTest("bad_append", failing, seed, makeSet);
        check(!result.error.empty(), "missed broken traversal");
        check(shrinkFailure("bad_append", failing, seed, makeSet, result.error).empty(),
              "initial failure did not shrink to empty");
        std::cout << "HARNESS TEST PASSED: 1000000 operations, seed=" << seed
                  << "; fault detection and shrinking passed\n"
                     "Real project trees are checked separately by differential.\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "HARNESS TEST FAILED: " << e.what() << '\n';
        return 1;
    }
}
