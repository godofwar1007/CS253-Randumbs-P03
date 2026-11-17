#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

build_dir="${BUILD_DIR:-build-run-all}"
case "${1:---smoke}" in
  --smoke)
    mkdir -p "$build_dir/smoke-results"
    output="$(mktemp -d "$build_dir/smoke-results/run.XXXXXX")"
    benchmark_flags=(--smoke)
    ;;
  --full) output="results"; benchmark_flags=() ;;
  --figures-only) output="results"; benchmark_flags=() ;;
  *) printf 'usage: %s [--smoke|--full|--figures-only]\n' "$0" >&2; exit 2 ;;
esac

cmake -S . -B "$build_dir" -DCMAKE_BUILD_TYPE=Release
cmake --build "$build_dir" --config Release
if [[ "${1:-}" != "--figures-only" ]]; then
  ctest --test-dir "$build_dir" -C Release --output-on-failure
fi

executable() {
  local candidate
  for candidate in "$build_dir/$1.exe" "$build_dir/Release/$1.exe" "$build_dir/$1"; do
    if [[ -x "$candidate" ]]; then printf '%s\n' "$candidate"; return; fi
  done
  printf 'Missing executable: %s\n' "$1" >&2
  return 1
}

simulator="$(executable simulator)"
prediction="$(executable d6_pred)"
runner="$(executable d6_run)"
figures="$(executable d6_figures)"
raw="$output/raw"
mkdir -p "$raw"
schedules=(uniform scapegoat_stress min_delete_high_insert max_delete_low_insert)
if [[ "${1:-}" != "--figures-only" ]]; then
  for schedule in "${schedules[@]}"; do
    if [[ -e "$raw/d6_$schedule" ]]; then
      printf 'Move existing benchmark data before rerunning: %s\n' "$raw/d6_$schedule" >&2
      exit 1
    fi
  done
  "$simulator" --strategy hibbard_succ --n 32 --seed 42 --pairs 64 --sample 32
  for schedule in "${schedules[@]}"; do
    "$runner" "$simulator" "$raw/d6_$schedule" --schedule "$schedule" "${benchmark_flags[@]}"
  done
fi
"$prediction" > "$raw/d6_prediction.csv"
for schedule in "${schedules[@]}"; do
  "$figures" "$raw/d6_$schedule" "$raw/d6_prediction.csv" \
    "$output/figures/d6_$schedule" "$raw/d6_uniform"
done
