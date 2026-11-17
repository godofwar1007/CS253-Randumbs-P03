#include "harness.hpp"
#include "trees.hpp"
#include "adversarial.hpp"

#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>

using namespace churn;
using namespace std;

struct TestCase {
    unsigned long long seed = 20260928;
    string tree = "all";
    string workload = "mixed";
    size_t size = 128;
    vector<Operation> ops;
};

unsigned long long readNumber(const string& text) 
{
    if (text.empty() || text[0] == '-') throw runtime_error("invalid number: " + text);
    size_t used = 0;
    unsigned long long value = stoull(text, &used);
    if (used != text.size()) throw runtime_error("invalid number: " + text);
    return value;
}

void saveCase(const string& path, const TestCase& test) {
    ofstream file(path);
    if (!file) throw runtime_error("cannot write " + path);
    file << "seed " << test.seed << "\ntree " << test.tree << '\n';
    if (test.workload == "adversarial")
        file << "workload adversarial\nsize " << test.size << '\n';
    for (const Operation& op : test.ops) file << op.type << ' ' << op.key << '\n';
    file.close();
    if (!file) throw runtime_error("failed writing " + path);
}

TestCase loadCase(const string& path) {
    ifstream file(path);
    if (!file) throw runtime_error("cannot read " + path);
    TestCase test;
    string line;
    size_t lineNumber = 0;
    bool seedFound = false, treeFound = false;
    bool workloadFound = false, sizeFound = false;
    while (getline(file, line)) {
        ++lineNumber;
        istringstream row(line);
        string type, value, extra;
        if (!(row >> type)) continue;
        if (!(row >> value) || (row >> extra))
            throw runtime_error("invalid line " + to_string(lineNumber));
        if (type == "seed" && !seedFound && test.ops.empty()) {
            test.seed = readNumber(value);
            seedFound = true;
        } else if (type == "tree" && !treeFound && test.ops.empty()) {
            test.tree = value;
            treeFound = true;
        } else if (type == "workload" && !workloadFound && test.ops.empty()) {
            if (value != "mixed" && value != "adversarial")
                throw runtime_error("unknown workload: " + value);
            test.workload = value;
            workloadFound = true;
        } else if (type == "size" && !sizeFound && test.ops.empty()) {
            unsigned long long size = readNumber(value);
            if (size < 2 || size > static_cast<unsigned long long>(kKeyUniverse))
                throw runtime_error("invalid size in replay header");
            test.size = static_cast<size_t>(size);
            sizeFound = true;
        } else if (type == "insert" || type == "erase" || type == "contains") {
            if (!seedFound || !treeFound) throw runtime_error("seed and tree headers required first");
            size_t used = 0;
            long long key = stoll(value, &used);
            if (used != value.size() || key < INT_MIN || key > INT_MAX)
                throw runtime_error("invalid key at line " + to_string(lineNumber));
            test.ops.push_back({type, static_cast<Key>(key)});
        } else {
            throw runtime_error("invalid line " + to_string(lineNumber));
        }
    }
    if (!file.eof() || !seedFound || !treeFound)
        throw runtime_error("incomplete replay file");
    return test;
}

int main(int argc, char** argv) {
    try {
        TestCase test;
        size_t count = 1000000;
        string input, output = "oracle_failure", saved;
        bool generationOptions = false;
        bool sizeSet = false;
        for (int i = 1; i < argc; ++i) {
            string option = argv[i];
            if (option == "--help") {
                cout << "differential [--tree all|NAME] [--seed N] [--operations N]\n"
                             "             [--workload mixed|adversarial] [--size N] [--save FILE]\n"
                             "differential --replay FILE\n"
                             "Use --output PATH to choose a failure-file prefix.\n";
                return 0;
            }
            if (i + 1 == argc) throw runtime_error("missing value for " + option);
            string value = argv[++i];
            if (option == "--tree") { test.tree = value; generationOptions = true; }
            else if (option == "--seed") { test.seed = readNumber(value); generationOptions = true; }
            else if (option == "--operations") {
                unsigned long long n = readNumber(value);
                if (n == 0 || n > numeric_limits<size_t>::max())
                    throw runtime_error("operation count must be positive and fit size_t");
                count = static_cast<size_t>(n);
                generationOptions = true;
            } else if (option == "--replay") input = value;
            else if (option == "--output") output = value;
            else if (option == "--save") saved = value;
            else if (option == "--workload") {
                test.workload = value;
                generationOptions = true;
            } else if (option == "--size") {
                unsigned long long size = readNumber(value);
                if (size < 2 || size > static_cast<unsigned long long>(kKeyUniverse))
                    throw runtime_error("size must be between 2 and the key universe");
                test.size = static_cast<size_t>(size);
                sizeSet = true;
                generationOptions = true;
            }
            else throw runtime_error("unknown option: " + option);
        }
        if (!input.empty()) {
            if (generationOptions) throw runtime_error("replay already contains the tree and seed");
            test = loadCase(input);
        }
        if (test.workload != "mixed" && test.workload != "adversarial")
            throw runtime_error("unknown workload: " + test.workload);
        if (sizeSet && test.workload != "adversarial")
            throw runtime_error("--size is only used with --workload adversarial");
        vector<string> names;
        if (test.tree == "all")
            names = {"hibbard_succ", "hibbard_rand", "scapegoat_array", "scapegoat_inplace"};
        else names.push_back(test.tree);
        for (const string& name : names) {
            Rng rng(test.seed);
            if (!makeTestTree(name, rng))
                throw runtime_error("tree is not connected: " + name + "; see oracle/trees.hpp");
        }
        if (input.empty()) {
            if (test.workload == "adversarial")
                test.ops = makeAdversarialOperations(test.seed, count, test.size);
            else test.ops = makeOperations(test.seed, count);
        }
        if (!saved.empty()) saveCase(saved, test);
        for (const string& name : names) {
            cout << "Testing " << name << ", seed=" << test.seed
                      << ", operations=" << test.ops.size()
                      << ", workload=" << test.workload << endl;
            TestResult result = runTest(name, test.ops, test.seed, makeTestTree);
            if (!result.error.empty()) {
                cerr << "FAIL at step " << result.step << ": " << result.error << '\n';
                if (result.step > 0) {
                    const Operation& op = test.ops[result.step - 1];
                    cerr << op.type << ' ' << op.key << '\n';
                }
                test.tree = name;
                test.ops.resize(result.step);
                saveCase(output + ".full.txt", test);
                cerr << "Saved failing sequence. Shrinking...\n";
                test.ops = shrinkFailure(name, test.ops, test.seed, makeTestTree, result.error);
                saveCase(output + ".small.txt", test);
                cerr << "Saved " << output << ".small.txt with " << test.ops.size()
                          << " operations\nReplay: differential --replay \""
                          << output << ".small.txt\"\n";
                return 1;
            }
            cout << "PASS " << name << '\n';
        }
        cout << "ALL SELECTED TREES PASSED\n";
        return 0;
    } catch (const exception& e) {
        cerr << "ERROR: " << e.what() << '\n';
        return 2;
    }
}
