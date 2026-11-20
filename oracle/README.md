# Oracle, differential testing and D4

**Owner:** Tanush Bansal, 250001071 | Oracle owner | CS 253 / MA 253, Project 3

This module checks whether the project's tree implementations behave correctly. It uses a sorted array as a simple reference dictionary, gives the same operations to that reference and each tree, and checks their results after every operation. If a difference is found, the runner records the failing input and tries to reduce it to a small replayable example.

The Oracle contribution includes the sorted-array implementation, seeded differential harness, adversarial-workload integration, replay and failure shrinking, and tests that exercise the tree invariant checkers. D4 is the accompanying derivation of the expected average depth of an initially random BST.

## Files and responsibilities

Paths in this table are relative to the repository root.

| File | Purpose |
|---|---|
| [oracle/sorted_array.hpp](sorted_array.hpp) | `SortedArrayOracle`, the reference dictionary backed by a sorted vector |
| [oracle/harness.hpp](harness.hpp) | Operation generation, state comparison, test execution and failure shrinking |
| [oracle/differential.cpp](differential.cpp) | Command-line runner, replay-file handling, output and failure reporting |
| [oracle/trees.hpp](trees.hpp) | Factory connecting strategy names to the four real trees |
| [oracle/adversarial.hpp](adversarial.hpp) | Converts the supplied adversarial schedule into recorded insert/erase operations |
| [tests/harness_test.cpp](../tests/harness_test.cpp) | Checks the reference and harness, including deliberately broken dictionaries |
| [tests/oracle_checks.cpp](../tests/oracle_checks.cpp) | Checks real-tree updates and verifies that corrupted metadata is rejected |
| [tests/adversarial_test.cpp](../tests/adversarial_test.cpp) | Checks adversarial generation, reproducibility and argument validation |
| [tests/oracle_cases/deletions.txt](../tests/oracle_cases/deletions.txt) | Saved regression sequence for all four trees |
| [CMakeLists.txt](../CMakeLists.txt) | Builds the programs and registers the CTest tests |
| [docs/derivations/D4_250001071.pdf](../docs/derivations/D4_250001071.pdf) | Individual D4 derivation |

The common contract is [include/dictionary.hpp](../include/dictionary.hpp), with the agreed behaviour described in [INTERFACE.md](../INTERFACE.md). The tree owners implement their structures and their `validate()` methods. The harness calls those validators, and the regression tests check that they detect selected faults.

## 1. The sorted-array oracle

The oracle stores distinct integer keys in increasing order in `vector<Key>`. It implements the same `Dictionary` interface as the trees, so the harness can apply identical operations to both.

| Method | Behaviour | Cost for n keys |
|---|---|---|
| `insert(key)` | Uses `lower_bound` to find the position; rejects a duplicate or inserts at that position | O(n), including element shifts |
| `erase(key)` | Uses `lower_bound`; rejects a missing key or removes the matching element | O(n), including element shifts |
| `contains(key)` | Uses the standard library's `binary_search` | O(log n) |
| `size()` | Returns the number of stored keys | O(1) |
| `inorder(out)` | Appends all keys to the supplied vector in increasing order | O(n) |
| `validate()` | Checks that every adjacent pair is strictly increasing | O(n) |
| `totalDepth()` | Returns zero because the array has no tree shape | O(1) |
| `rebuildCount()` | Returns zero because the array does not perform tree rebuilds | O(1) |
| `name()` | Returns `oracle` | O(1) |

The array is a suitable reference because its correctness is easy to follow. It starts empty. Inserting an absent key at its lower-bound position preserves sorted order and uniqueness; erasing an existing key preserves both properties. Binary search therefore answers membership correctly. Duplicate insertions and missing-key erasures return false without changing the stored keys.

Tree depth is not compared with the oracle's zero depth. The tree validators independently check the actual tree metadata. The vector is a reference implementation; it does not replace either tree under study.

## 2. How one differential run proceeds

1. `differential.cpp` reads the command-line options or loads a replay file.
2. The runner checks that every selected strategy has a factory connection in `trees.hpp`.
3. For a generated workload, it creates the complete operation vector before executing the tests.
4. `runTest()` starts a fresh tree, an empty sorted-array oracle and a seeded tree RNG for each strategy.
5. It checks the initial empty state, strategy name and initial rebuild count.
6. It applies each operation to the oracle and the tree and checks the result and resulting state.
7. A passing strategy prints `PASS`. On the first failure, the runner saves the failing prefix, shrinks it and saves the reduced sequence.

An operation is one dictionary call: `insert key`, `erase key` or `contains key`. A churn pair contains two updates: one erase followed by one insert.

The default selection runs these four strategies separately:

| Strategy | Implementation |
|---|---|
| `hibbard_succ` | Hibbard BST using successor replacement |
| `hibbard_rand` | Hibbard BST choosing predecessor or successor randomly |
| `scapegoat_array` | Scapegoat tree using array rebuilding |
| `scapegoat_inplace` | Scapegoat tree using in-place Stout-Warren rebuilding |

`TreeMaker` in `harness.hpp` is a function-pointer type. It lets `runTest()` request a dictionary by name without depending on its node representation. `makeTestTree()` in `trees.hpp` supplies the actual implementation.

### Seeded mixed operations

`makeOperations(seed, count)` first adds fixed edge cases: empty lookups and erasures, duplicate insertions, repeated erasures, integer boundaries, increasing insertion and decreasing insertion, with complete drains between them. Boundary keys include `INT_MIN`, `INT_MAX`, zero, minus one and one. If the requested count is shorter than this prefix, the prefix is truncated.

It then fills the remaining operation slots from a seeded RNG. The choices target roughly 40% insertions, 40% erasures and 20% membership checks. Random keys range from -128 through 128. This small working set repeatedly exercises duplicates and missing keys while making full state checks after every operation practical.

The defaults are **1,000,000 operations per selected tree**, seed **20260928**. With `--tree all`, the same million-operation vector is replayed on each of the four strategies; the total is not divided among them.

The workload has its own RNG during generation. Each tested tree receives a fresh RNG initialized from the recorded seed. A random predecessor/successor choice can therefore consume RNG values without changing later workload operations. Reproduction requires compatible source and random-number behaviour as well as the recorded seed.

### Checks after each operation

The harness compares or checks:

- The boolean result of the requested operation.
- Membership of the key just used.
- Strict ordering of the reference array and the tree's `validate()` result.
- The number of keys and the complete sorted contents.
- The append behaviour of `inorder(out)`: the comparison vectors already contain a sentinel, which traversal must preserve.
- Zero total depth for an empty tree.
- A rebuild count that never decreases, with zero rebuilds for either Hibbard strategy.

The Hibbard validator checks strict BST bounds, stored subtree sizes, total node count and total depth. The scapegoat validator checks strict BST bounds, subtree sizes, total depth, the high-water size relation and the applicable height limit. A valid scapegoat tree does not need every node to remain weight-balanced after every operation.

## 3. Build and run on Windows

Run the commands from the **repository root**, not from inside `oracle/`. Use CMake 3.16 or newer and a C++17 compiler with the library support required by the whole project, including integer `std::from_chars`. MinGW GCC 6.3 is insufficient for the complete build.

In PowerShell:

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

For a single-configuration build, run the differential program with:

```powershell
.\build\differential.exe --tree all --operations 1000000 --seed 20260928
```

For a multi-configuration generator, executables may be under `build/Release/`; use `.\build\Release\differential.exe` instead. The same adjustment applies to the other executable commands below.

To test one strategy or inspect the supported options:

```powershell
.\build\differential.exe --tree hibbard_succ --operations 100000 --seed 42
.\build\differential.exe --help
```

A successful all-tree mixed run prints:

```text
Testing hibbard_succ, seed=20260928, operations=1000000, workload=mixed
PASS hibbard_succ
Testing hibbard_rand, seed=20260928, operations=1000000, workload=mixed
PASS hibbard_rand
Testing scapegoat_array, seed=20260928, operations=1000000, workload=mixed
PASS scapegoat_array
Testing scapegoat_inplace, seed=20260928, operations=1000000, workload=mixed
PASS scapegoat_inplace
ALL SELECTED TREES PASSED
```

This is the expected passing output, not a claim that every current platform build passes. See the verification status below.

## 4. Adversarial differential testing

`makeAdversarialOperations()` uses the teammate's [AdversarialWorkload](../adversary/adversarial_workload.hpp). It records distinct random initial insertions, then repeatedly deletes the current minimum and inserts a fresh key from a decreasing high-end sequence. It tracks the live keys with the oracle while constructing the operation vector.

```powershell
.\build\differential.exe --tree all --workload adversarial --size 128 --operations 1000000 --seed 20260928
```

Here `--operations` includes the initial 128 insertions. The remaining 999,872 operations form 499,936 complete erase/insert pairs. The live size is 128 between pairs and 127 between their two updates.

For this workload, the size must be between 2 and the key-universe size. The operation count must equal the initial size plus a positive even number. `--size` is only accepted for adversarial generation. The default mixed workload has no fixed live size.

To save a generated sequence, including a passing one, and replay it:

```powershell
.\build\differential.exe --tree all --workload adversarial --size 16 --operations 1024 --seed 42 --save build/adversarial.txt
.\build\differential.exe --replay build/adversarial.txt
```

The replay stores actual operations, so tree-specific RNG choices cannot change the input stream. The `workload` and `size` headers describe its origin. A shrunk replay is allowed to contain incomplete pairs because replay executes the stored operations directly.

This integration currently covers minimum deletion with high-end insertion. The simulator's newer `max_delete_low_insert` and `scapegoat_stress` schedules have separate schedule checks and benchmarks, but are not selectable through this differential generator.

## 5. Failure files, replay and shrinking

On a mismatch, the runner reports the strategy, seed, failing step, error and offending operation when one exists. Steps are numbered from one; step zero means the initial state failed.

It saves the prefix up to that failure as `<prefix>.full.txt` before trying to shrink it. After shrinking, it saves `<prefix>.small.txt`. The default prefix is `oracle_failure` in the working directory. Use an existing ignored build folder to keep generated inputs out of the repository:

```powershell
.\build\differential.exe --tree all --operations 1000000 --seed 20260928 --output build/oracle_failure
```

If that run finds a failure and creates the files, replay the reduced case with:

```powershell
.\build\differential.exe --replay build/oracle_failure.small.txt
```

The parent folder must exist. Existing save or failure files at the chosen paths are overwritten. A passing run does not create failure files.

### Replay format

```text
seed 42
tree hibbard_succ
insert 20
insert 10
erase 20
contains 10
```

The `seed` and `tree` headers must appear before operations. `tree all` selects all four strategies. Optional `workload` and `size` headers also precede operations. Each nonempty line contains exactly two tokens; operation keys must fit the `Key` integer type. Blank lines are allowed.

Replay files supply their own settings. Do not combine `--replay` with generation options: `--tree`, `--seed`, `--operations`, `--workload` or `--size`.

### How shrinking works

`shrinkFailure()` first confirms that the same error can be reproduced. It removes a chunk of operations, constructs a fresh tree with the same seed, and runs the shorter sequence. If the same error text still occurs, it keeps that reduction. Otherwise, it tries another chunk. It progressively reduces chunk size until it checks individual operations.

The result is deletion-minimal for that error: removing any single remaining operation stops that error from occurring. It is not guaranteed to be the globally shortest sequence, and it does not simplify key values. A randomized tree may make different internal choices after operations are removed; each candidate is replayed to check whether it still fails.

The runner records standard C++ exceptions as failures. Crashes, sanitizer aborts and hangs are not isolated in a subprocess and may stop the runner before it can save or shrink a case. Large failing sequences can also take time to shrink.

| Exit code | Meaning |
|---|---|
| `0` | Every selected tree passed |
| `1` | A differential failure was detected and the failure-handling path completed |
| `2` | Input, configuration or file handling failed, including an exception escaping failure handling |

## 6. What the tests check

CTest currently registers ten tests. These serve different purposes: some check the real trees, while others check the oracle, workload generator or failure-detection machinery.

| CTest name | Purpose |
|---|---|
| `adversarial_schedules` | Checks the shared schedule implementations and their selection |
| `harness_test` | Compares the oracle with a test-only `std::set` over one million operations; checks reproducibility, deliberately broken methods, append handling and shrinking |
| `differential_all` | Runs one million mixed operations on each real tree, seed 20260928 |
| `scapegoat_tests` | Focused tests of the scapegoat implementation and both rebuild methods |
| `oracle_checks` | Checks real-tree update cases, metadata and validator fault detection |
| `differential_replay` | Replays `tests/oracle_cases/deletions.txt` against all four trees |
| `adversarial_generator` | Checks distinct initialization, minimum deletion, decreasing fresh insertions, fixed-size pairs, seed reproduction and invalid-input rejection |
| `differential_adversarial` | Runs one million adversarial operations per real tree with initial size 128 and seed 20260928 |
| `adversarial_export` | Runs and saves a 1,024-operation adversarial case with size 16 and seed 42 |
| `adversarial_replay` | Replays the exported file; a CTest fixture makes the export run first |

`oracle_checks` covers root, leaf, one-child and two-child deletion, empty and single-node states, duplicates, missing keys, integer boundaries, sorted insertion, draining and deletion-triggered rebuilding. Test-only access temporarily corrupts subtree sizes, node counts, depth sums, ordering and scapegoat high-water metadata. The test checks that `validate()` rejects each corruption and accepts the restored fixture.

The test-only `std::set` is an independent check of the reference and harness. It is not used as the implementation of either project tree. Passing `harness_test` establishes that the testing machinery passed its checks; the real-tree differential tests are separate.

Run the full registered suite with the CTest command in the build section. Individual executable checks can also be run directly:

```powershell
.\build\harness_test.exe
.\build\oracle_checks.exe
.\build\adversarial_test.exe
.\build\differential.exe --replay tests/oracle_cases/deletions.txt
```

## 7. Sanitizers and verification status

For a Linux or WSL toolchain with AddressSanitizer and UndefinedBehaviorSanitizer support, the intended workflow is:

```sh
cmake -S . -B build-oracle-sanitize -DCHURN_SANITIZE=ON
cmake --build build-oracle-sanitize -j 4
ctest --test-dir build-oracle-sanitize --output-on-failure
```

Linux executable paths use `./build-oracle-sanitize/differential` rather than the Windows `.exe` paths. Sanitizers check for memory errors and undefined behaviour; they complement dictionary and invariant checks.

**Current build limitation, checked on 20 November 2026:** the fresh WSL GCC 13.3 build fails because `SortedArrayOracle::totalDepth()` and `rebuildCount()` return `unsigned long long`, while the shared interface declares `std::uint64_t`. Those are different types on this Linux toolchain. The overrides must match the interface exactly before the full Linux suite can run. The documentation update does not change these source files.

The Windows harness self-test passed one million operations with seed 20260928, including fault detection and shrinking. The Windows adversarial-generator test also passed. These checks do not establish that the complete current tree suite passes. A modern Windows compiler was not available for a fresh full-project build during that audit.

Earlier verification records are preserved here:

| Record | Contents |
|---|---|
| [oracle_sanitizer_tests.log](../results/raw/oracle_sanitizer_tests.log) | Earlier five-test sanitizer-suite output |
| [oracle_validation.csv](../results/raw/oracle_validation.csv) | Summary of that verification stage |
| [oracle_adversarial_tests.log](../results/raw/oracle_adversarial_tests.log) | Earlier nine-test suite including adversarial integration |
| [oracle_adversarial_validation.csv](../results/raw/oracle_adversarial_validation.csv) | Summary of adversarial verification |

These logs describe earlier source states and smaller registered suites. They are correctness records, not performance measurements or a substitute for rebuilding and running all ten current tests.

## 8. D4 and its application

[D4](../docs/derivations/D4_250001071.pdf) proves that the expected average depth of a BST built by random insertion is about `2 ln n`, with root depth zero.

Let `P(n)` be the expected sum of node depths. A uniformly random root leaves `k` keys on one side and `n - 1 - k` on the other. Both subtrees contribute their own expected path lengths, and their `n - 1` nodes gain one level. Averaging over the root choices gives:

```text
P(0) = P(1) = 0
P(n) = (2/n) * sum(P(k), k = 0..n-1) + (n - 1)
P(n) = 2(n + 1)H(n) - 4n
Expected average depth = P(n)/n = 2(n + 1)H(n)/n - 4
```

Here `H(n)` is the nth harmonic number. Since `H(n) = ln n + O(1)`, the expected average depth is `2 ln n + O(1)`.

In [app/simulator.cpp](../app/simulator.cpp), `main()` constructs the initial tree from distinct random insertions. Its `writeRow(0)` call records the initial average depth using `averageDepth()` from [include/dictionary.hpp](../include/dictionary.hpp). That ordinary-BST measurement can be compared with D4's expected value.

D4 describes initial random construction. It does not prove the steady-state depth after churn or the depth of a scapegoat tree after balancing.

## 9. Scope and related work

The million-operation differential runs check correctness on bounded working sets. The full-scale application experiments separately use n = 1,000, 2,000 and 4,000 with at least n squared churn pairs for the uniform workload. Their timing and depth results belong to [the benchmark workflow](../bench/README.md) and [REPORT.md](../REPORT.md).

The adversarial differential run checks dictionary correctness under its supplied schedule. It does not prove a maximum rebuild frequency or isolate successor-deletion bias: deleting a minimum never uses the two-child replacement choice. Performance claims need the application measurements and their workload arguments.

For the shared contract, see [INTERFACE.md](../INTERFACE.md). For tool assistance, see [TOOLS.md](../TOOLS.md). The course project description assigns the sorted-array oracle, one million seeded differential operations, failure shrinking, invariant checkers and D4 to the Oracle role; the submission guidelines define the required documentation and verification records.
