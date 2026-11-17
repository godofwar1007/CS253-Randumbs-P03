#include "adversarial.hpp"

#include <iostream>
#include <utility>

using namespace churn;

void require(bool passed, const std::string& message) {
    if (!passed) throw std::runtime_error(message);
}

int main() {
    try {
        std::size_t size = 16;
        auto ops = makeAdversarialOperations(42, 1024, size);
        auto same = makeAdversarialOperations(42, 1024, size);
        auto different = makeAdversarialOperations(43, 1024, size);
        require(ops.size() == 1024, "incorrect operation count");
        bool changed = false;
        for (std::size_t i = 0; i < ops.size(); ++i) {
            require(ops[i].type == same[i].type && ops[i].key == same[i].key,
                    "seed did not reproduce workload");
            if (ops[i].key != different[i].key) changed = true;
        }
        require(changed, "different seed did not change initial keys");

        SortedArrayOracle oracle;
        for (std::size_t i = 0; i < size; ++i)
            require(ops[i].type == "insert" && oracle.insert(ops[i].key),
                    "initial keys are not distinct insertions");
        Key next = kKeyUniverse - 1;
        for (std::size_t i = size; i < ops.size(); i += 2) {
            std::vector<Key> ordered;
            oracle.inorder(ordered);
            require(oracle.size() == size, "size changed between pairs");
            require(ops[i].type == "erase" && ops[i].key == ordered.front(),
                    "schedule did not delete the minimum key");
            require(oracle.erase(ops[i].key) && oracle.size() == size - 1,
                    "deletion did not remove a live key");
            while (oracle.contains(next)) --next;
            require(ops[i + 1].type == "insert" && ops[i + 1].key == next,
                    "schedule did not insert decreasing fresh keys");
            require(oracle.insert(ops[i + 1].key) && oracle.size() == size,
                    "insertion did not restore the size");
            --next;
        }

        for (auto values : {std::pair<std::size_t, std::size_t>{10, 0},
                            {10, 1}, {10, 10}, {17, 16}, {19, 16}, {18, kKeyUniverse + 1u}}) {
            bool rejected = false;
            try {
                makeAdversarialOperations(42, values.first, values.second);
            } catch (const std::runtime_error&) {
                rejected = true;
            }
            require(rejected, "invalid size or incomplete pair was accepted");
        }
        require(makeAdversarialOperations(42, 4, 2).size() == 4,
                "smallest valid workload was rejected");
        std::cout << "ADVERSARIAL GENERATOR TEST PASSED\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "ADVERSARIAL GENERATOR TEST FAILED: " << e.what() << '\n';
        return 1;
    }
}
