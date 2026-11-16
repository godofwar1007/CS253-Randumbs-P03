# Differential testing

The runner gives the same operations to the sorted-array oracle and each tree.
After every operation it checks the return value, membership, size, sorted
contents, traversal append behaviour and the tree's `validate()` result.
It also checks empty-tree depth and rebuild-count bookkeeping.

The default is 1,000,000 operations with seed 20260928. It starts with empty,
duplicate, missing-key, integer-boundary and sorted-insertion cases, then mixes
insert, erase and contains operations over 257 possible keys. This bounded
working set makes full checks after every operation practical. It is separate
from full-scale churn benchmarks; replay files can contain larger workloads.

## Build

With CMake 3.16+ and a C++17 compiler:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

For multi-configuration generators, use `--config Release` for the build and
`-C Release` for CTest. Executables may be under `build/Release/`.

Or create `build/` and compile the two programs directly:

```sh
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -I include oracle/differential.cpp src/structA/hibbard.cpp src/structB/scapegoat.cpp -o build/differential
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -I include -I oracle tests/harness_test.cpp -o build/harness_test
```

On Windows use `.exe` for the output filenames.

## Connected trees

`oracle/trees.hpp` connects `hibbard_succ` and `hibbard_rand` to `HibbardBst`,
and `scapegoat_array` and `scapegoat_inplace` to `ScapegoatTree`. CMake links
both implementations. The randomized Hibbard policy receives the supplied RNG.
Operations are generated before any tree runs, so a tree's random choices
cannot change the input sequence. Unknown names are reported as errors.

```sh
./build/harness_test
./build/differential --tree all --operations 1000000 --seed 20260928
./build/differential --tree hibbard_succ --operations 100000 --seed 42
```

`harness_test` checks the oracle against an independent test-only `std::set`,
seed reproducibility, deliberately broken methods and failure shrinking.
It prints `HARNESS TEST PASSED`. This does not certify the real trees.
A passing differential run prints `PASS` for each selected tree, then
`ALL SELECTED TREES PASSED`. Exit codes are 0 for success, 1 for a detected
failure, and 2 for an input/configuration/file error.

`oracle_checks` exercises root, leaf, one-child and two-child deletion,
duplicates, missing keys, integer boundaries, sorted insertion and deletion
shrinkage. Test-only access deliberately corrupts subtree sizes, node counts,
depth sums, ordering and scapegoat high-water metadata to verify `validate()`
rejects them. `differential_replay` checks the saved deletion fixture against
all four trees. The separate `scapegoat_tests` target checks the rebuilders.

For a memory-error check, run these commands inside WSL Ubuntu or Linux with
a full C++17 compiler (verified toolchain: GCC 13.3.0, CMake 3.28.3):

```sh
cmake -S . -B build-oracle-sanitize -DCHURN_SANITIZE=ON
cmake --build build-oracle-sanitize -j 4
ctest --test-dir build-oracle-sanitize --output-on-failure
```

MinGW GCC 6.3.0 does not support all C++17 features used by the complete
project. Use the WSL commands above for the full suite.

The verified sanitizer run passed all five CTests, including one million
operations per real tree with seed 20260928. The original output is saved in
`results/raw/oracle_sanitizer_tests.log`, with a summary in
`results/raw/oracle_validation.csv`. These record correctness, not performance.

## Failures and replay

The runner prints the tree, seed, failing step and mismatch. Step 0 means the
initial state failed. It saves the failing prefix to `oracle_failure.full.txt`
before shrinking and the reduced sequence to `oracle_failure.small.txt`.
Use `--output PATH` to choose another basename; the parent folder must exist.
Existing files with that basename are overwritten.

```sh
./build/differential --replay oracle_failure.small.txt
```

Replay files contain their own tree and seed, so do not combine `--replay`
with `--tree`, `--seed` or `--operations`. The format is:

```text
seed 42
tree hibbard_succ
insert 20
insert 10
erase 20
contains 10
```

Each shrinking attempt starts with a fresh tree and the same seed. Chunks are
removed while the same error still occurs. The final case is deletion-minimal:
removing any one operation stops that error. It is not guaranteed globally
shortest, and key values are not reduced. Large cases can take time to shrink.

Each real tree's `validate()` must independently recompute its node counts,
stored depth sum and applicable invariants. The runner cannot inspect private
nodes through `Dictionary`. Tree depth is not compared with the array oracle's
zero depth. Scapegoat checkers must not assume every node always remains
weight-balanced.

Standard C++ exceptions are recorded. Crashes, sanitizer aborts and hangs need
a debugger or sanitizer; this runner does not isolate each replay in a separate
process. Replaying randomized behaviour also requires compatible tree code and
random-number implementation.
