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
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -I include oracle/differential.cpp -o build/differential
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -I include -I oracle tests/harness_test.cpp -o build/harness_test
```

On Windows use `.exe` for the output filenames.

## Connect the trees

`oracle/trees.hpp` contains `makeTestTree(name, rng)`. Include the real tree
headers there and return a new tree for each name: `hibbard_succ`,
`hibbard_rand`, `scapegoat_array` and `scapegoat_inplace`. Pass the supplied RNG
to any tree needing randomness. Add their `.cpp` files to the `differential`
CMake target if required. The operations are generated before any tree runs,
so a tree's random choices cannot change the input sequence.

The real implementations are not present in this checkout. Until connected,
the runner and `differential_all` CTest fail with `tree is not connected`.
No missing tree is silently skipped or replaced by the oracle.

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
