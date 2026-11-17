# Churn experiments

Run from the repository root. The workflow needs Bash, CMake, Ninja, a
C++17 g++-compatible compiler, Python 3, and matplotlib. On Windows, use
Git Bash with the matching native CMake, Ninja, compiler, and Python on PATH.

- `bash scripts/run_all.sh --smoke` builds, runs CTest and the schedule
  checks, runs the simulator, then exercises every benchmark case with
  64 keys and two seeds. Smoke figures and CSVs go under
  `build-run-all/smoke-results/`.
- `bash scripts/run_all.sh --full` uses 1,000, 2,000, and 4,000 keys,
  n² pairs per run, and five recorded seeds. It writes one CSV per
  experiment under `results/raw/` and figures under `results/figures/`.
- `python3 bench/churn_experiments.py --figures-only` recreates the
  figures from the full raw CSV files.

The standalone runner uses the existing dictionary implementations.
It assigns separate seeded RNG streams to churn and deletion policy,
so corresponding runs start with the same keys and see the same requests.
It samples average depth in edges and cumulative rebuilds, and verifies
the final contents and invariants. Elapsed seconds include workload
generation and sampled CSV output.

The extremal-delete schedules rotate through a key interval above or
below the initial live keys. Over time, oppositely directed deletion and
insertion leave a chain. Once that happens, both deletion policies can
degrade because deleting an extreme never chooses a two-child replacement.
The monotone insertion schedule tests whether repeatedly attaching new
extreme keys stresses scapegoat subtree rebuilding. Use the net rebuild
count since pair zero when comparing it to uniform churn.
