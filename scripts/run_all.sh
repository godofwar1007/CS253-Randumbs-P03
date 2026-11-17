#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
build_dir="${BUILD_DIR:-build-run-all}"
compiler="${CXX:-g++}"
python_cmd="${PYTHON:-python3}"
mode="${1:---smoke}"
case "$mode" in
  --smoke|--full) ;;
  *) printf 'usage: %s [--smoke|--full]\n' "$0" >&2; exit 2 ;;
esac

cmake -G Ninja -S . -B "$build_dir" -DCMAKE_BUILD_TYPE=Release "-DCMAKE_CXX_COMPILER=$compiler"
cmake --build "$build_dir" --config Release
ctest --test-dir "$build_dir" -C Release --output-on-failure
simulator="$build_dir/simulator"
if [[ ! -x "$simulator" ]]; then
  simulator="$simulator.exe"
fi
"$simulator" --strategy hibbard_succ --n 32 --seed 42 --pairs 64 --sample 32

"$compiler" -std=c++17 -O2 -Wall -Wextra -Wpedantic -Iinclude \
  adversary/schedule_test.cpp -o "$build_dir/schedule_test.exe"
"$build_dir/schedule_test.exe"
"$compiler" -std=c++17 -O2 -Wall -Wextra -Wpedantic \
  -Iinclude -Isrc/structA -Isrc/structB adversary/run_workload.cpp \
  src/structA/hibbard.cpp src/structB/scapegoat.cpp -o "$build_dir/run_workload.exe"
"$python_cmd" bench/churn_experiments.py "$build_dir/run_workload.exe" "$mode"
