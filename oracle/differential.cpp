#include "harness.hpp"
#include "trees.hpp"

#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>

using namespace churn;

struct TestCase {
    std::uint64_t seed = 20260928;
    std::string tree = "all";
    std::vector<Operation> ops;
};

std::uint64_t readNumber(const std::string& text) {
    if (text.empty() || text[0] == '-') throw std::runtime_error("invalid number: " + text);
    std::size_t used = 0;
    std::uint64_t value = std::stoull(text, &used);
    if (used != text.size()) throw std::runtime_error("invalid number: " + text);
    return value;
}

void saveCase(const std::string& path, const TestCase& test) {
    std::ofstream file(path);
    if (!file) throw std::runtime_error("cannot write " + path);
    file << "seed " << test.seed << "\ntree " << test.tree << '\n';
    for (const Operation& op : test.ops) file << op.type << ' ' << op.key << '\n';
    file.close();
    if (!file) throw std::runtime_error("failed writing " + path);
}

TestCase loadCase(const std::string& path) {
    std::ifstream file(path);
    if (!file) throw std::runtime_error("cannot read " + path);
    TestCase test;
    std::string line;
    std::size_t lineNumber = 0;
    bool seedFound = false, treeFound = false;
    while (std::getline(file, line)) {
        ++lineNumber;
        std::istringstream row(line);
        std::string type, value, extra;
        if (!(row >> type)) continue;
        if (!(row >> value) || (row >> extra))
            throw std::runtime_error("invalid line " + std::to_string(lineNumber));
        if (type == "seed" && !seedFound && test.ops.empty()) {
            test.seed = readNumber(value);
            seedFound = true;
        } else if (type == "tree" && !treeFound && test.ops.empty()) {
            test.tree = value;
            treeFound = true;
        } else if (type == "insert" || type == "erase" || type == "contains") {
            if (!seedFound || !treeFound) throw std::runtime_error("seed and tree headers required first");
            std::size_t used = 0;
            long long key = std::stoll(value, &used);
            if (used != value.size() || key < INT_MIN || key > INT_MAX)
                throw std::runtime_error("invalid key at line " + std::to_string(lineNumber));
            test.ops.push_back({type, static_cast<Key>(key)});
        } else {
            throw std::runtime_error("invalid line " + std::to_string(lineNumber));
        }
    }
    if (!file.eof() || !seedFound || !treeFound)
        throw std::runtime_error("incomplete replay file");
    return test;
}

int main(int argc, char** argv) {
    try {
        TestCase test;
        std::size_t count = 1000000;
        std::string input, output = "oracle_failure";
        bool generationOptions = false;
        for (int i = 1; i < argc; ++i) {
            std::string option = argv[i];
            if (option == "--help") {
                std::cout << "differential [--tree all|NAME] [--seed N] [--operations N]\n"
                             "differential --replay FILE\n"
                             "Use --output PATH to choose a failure-file prefix.\n";
                return 0;
            }
            if (i + 1 == argc) throw std::runtime_error("missing value for " + option);
            std::string value = argv[++i];
            if (option == "--tree") { test.tree = value; generationOptions = true; }
            else if (option == "--seed") { test.seed = readNumber(value); generationOptions = true; }
            else if (option == "--operations") {
                std::uint64_t n = readNumber(value);
                if (n == 0 || n > std::numeric_limits<std::size_t>::max())
                    throw std::runtime_error("operation count must be positive and fit size_t");
                count = static_cast<std::size_t>(n);
                generationOptions = true;
            } else if (option == "--replay") input = value;
            else if (option == "--output") output = value;
            else throw std::runtime_error("unknown option: " + option);
        }
        if (!input.empty()) {
            if (generationOptions) throw std::runtime_error("replay already contains the tree and seed");
            test = loadCase(input);
        }
        std::vector<std::string> names;
        if (test.tree == "all")
            names = {"hibbard_succ", "hibbard_rand", "scapegoat_array", "scapegoat_inplace"};
        else names.push_back(test.tree);
        for (const std::string& name : names) {
            Rng rng(test.seed);
            if (!makeTestTree(name, rng))
                throw std::runtime_error("tree is not connected: " + name + "; see oracle/trees.hpp");
        }
        if (input.empty()) test.ops = makeOperations(test.seed, count);
        for (const std::string& name : names) {
            std::cout << "Testing " << name << ", seed=" << test.seed
                      << ", operations=" << test.ops.size() << std::endl;
            TestResult result = runTest(name, test.ops, test.seed, makeTestTree);
            if (!result.error.empty()) {
                std::cerr << "FAIL at step " << result.step << ": " << result.error << '\n';
                if (result.step > 0) {
                    const Operation& op = test.ops[result.step - 1];
                    std::cerr << op.type << ' ' << op.key << '\n';
                }
                test.tree = name;
                test.ops.resize(result.step);
                saveCase(output + ".full.txt", test);
                std::cerr << "Saved failing sequence. Shrinking...\n";
                test.ops = shrinkFailure(name, test.ops, test.seed, makeTestTree, result.error);
                saveCase(output + ".small.txt", test);
                std::cerr << "Saved " << output << ".small.txt with " << test.ops.size()
                          << " operations\nReplay: differential --replay \""
                          << output << ".small.txt\"\n";
                return 1;
            }
            std::cout << "PASS " << name << '\n';
        }
        std::cout << "ALL SELECTED TREES PASSED\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << '\n';
        return 2;
    }
}
