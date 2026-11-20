# Project 3 — Search Trees Under Long-Running Churn

CS 253 / MA 253 Data Structures and Algorithms Laboratory

The simulator compares two Hibbard deletion policies and two scapegoat rebuild methods under the same fixed-size workloads. It measures average node depth and rebuild frequency at 1,000, 2,000, and 4,000 keys. In the recorded uniform runs, always-successor deletion produced the greatest final average depth at all three sizes.

## Team and roles

| Member | Roll number | Role | Module | Derivation |
|---|---|---|---|---|
| Mayank Khoria | 250001048 | Structure A | `src/structA/` | [D1](docs/derivations/D1_250001048.pdf) |
| Parth Pawar | 2500041030 | Structure B | `src/structB/` | [D2](docs/derivations/D2.pdf) |
| Eklavya | 250001028 | Architect | `app/`, `include/` | [D3](docs/derivations/D3_250001028.pdf) |
| Tanush Bansal | 250001071 | Oracle | `oracle/`, `tests/` | [D4](docs/derivations/D4_250001071.pdf) |
| Chetan Yogbharti | 250001023 | Adversary | `adversary/` | [D5](docs/derivations/D5_250001023.pdf) |
| Parth Kalia | 250001054 | Analyst | `bench/`, `results/` | [D6](docs/derivations/D6_250001054.pdf) |

## Build

Use CMake 3.16 or newer and a C++17 compiler with integer `std::from_chars` support. The project was checked with Windows MinGW GCC 14.2. Bash is required for the benchmark script.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The examples below use paths from a single-configuration build. With a multi-configuration build, executables may be under `build/Release/`. Windows executables use the `.exe` extension.

## Simulator quick start

```sh
./build/simulator --strategy hibbard_succ --n 4 --seed 42 --pairs 4 --sample 2
```

Sample stdout from GCC 14.2 on Windows:

```csv
strategy,n,seed,pair_index,avg_depth,rebuild_count
hibbard_succ,4,42,0,1.250000,0
hibbard_succ,4,42,2,1.500000,0
hibbard_succ,4,42,4,1.500000,0
```

Use `hibbard_succ`, `hibbard_rand`, `scapegoat_array`, or `scapegoat_inplace` for comparisons. The default schedule is `uniform`. Other schedules are `min_delete_high_insert`, `max_delete_low_insert`, and `scapegoat_stress`. The defaults are n² pairs and one sample every n pairs. Use `--out FILE` for CSV output to a file. Diagnostics and elapsed time go to stderr. For matching workloads, use the same build, seed, size, schedule, and pair count across strategies.

## Tests and replay

Oracle work provides the sorted-array reference, a seeded differential harness connected to all four tree strategies, failure replay and shrinking, and regression tests for the invariant checkers. The harness compares operation results and dictionary state after every operation, with one million operations per strategy in the mixed and adversarial test configurations.My [Oracle README](oracle/README.md) explains the files, testing procedure, Windows commands, recorded results and current limitations.

```sh
ctest --test-dir build -C Release --output-on-failure
./build/differential --tree all --operations 1000000 --seed 20260928
./build/differential --replay tests/oracle_cases/deletions.txt
```

The replay format starts with `seed <integer>` and `tree <strategy-or-all>`. Each following line is `insert <key>`, `erase <key>`, or `contains <key>`. See the [oracle instructions](oracle/README.md) for saving, replaying, and shrinking a failing sequence.

## Benchmarks and figures

```sh
bash scripts/run_all.sh --smoke
bash scripts/run_all.sh --figures-only
```

Use `--full` for fresh measurements. The script requires unused benchmark output directories for a full run. See the [benchmark instructions](bench/README.md) for the existing raw data and pair budgets. At 4,000 keys after 16 million uniform pairs, the five-seed mean final depths were **23.93** edges for always-successor, **11.94** for randomized replacement, and **12.87** for both scapegoat modes. Hardware cache-counter measurements were unavailable. [Report](REPORT.md)

## Repository map

| Path | Contents |
|---|---|
| `app/`, `include/` | Simulator, schedule interface, dictionary contract, and RNG helpers |
| `src/structA/`, `src/structB/` | Hibbard and scapegoat tree implementations |
| `oracle/`, `tests/` | Sorted-array oracle, differential tests, and replay fixtures |
| `adversary/` | Adversarial workloads and workload runner |
| `bench/`, `scripts/` | Benchmark runner and experiment commands |
| `results/` | Raw measurements, figures, and the figure generator |
| `docs/` | Derivations, prediction model, and presentation materials |

See [INTERFACE.md](INTERFACE.md) for invariants and [TOOLS.md](TOOLS.md) for tool use.
