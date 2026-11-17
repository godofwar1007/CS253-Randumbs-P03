# Churn experiments

Run from the repo root with Bash, CMake 3.16 or later, and a C++17 compiler.
On Windows, use Git Bash with native CMake and a C++17 toolchain.

| Command | Output |
|---|---|
| `bash scripts/run_all.sh --smoke` | Build, CTest, and short runs for every workload under `build-run-all/smoke-results/` |
| `bash scripts/run_all.sh --full` | Build, CTest, measured runs in `results/raw/`, and figures in `results/figures/` |
| `bash scripts/run_all.sh --figures-only` | Regenerate the figures from the checked-in CSVs |

The full workflow uses five matched seeds and n = 1000, 2000, 4000.
Uniform runs and `scapegoat_stress` run for n² pairs. Each Hibbard adversarial
direction runs for 4n pairs to capture chain formation and persistence.
Its control is the first 4n pairs of the same-size uniform run. Both
scapegoat modes are measured under stress and both Hibbard policies
under each directed workload. The same simulator records all runs.

Each output directory contains a manifest and one sample CSV per
(strategy, size, seed). A new run requires a fresh raw directory. Move
the old raw directories before using `--full` and record the new host,
compiler, flags and source revision in `results/raw/d6_environment.txt`.
The C++ figure generator creates SVGs and summary CSVs. See [D6 tool
usage](D6.md) for the individual commands.
