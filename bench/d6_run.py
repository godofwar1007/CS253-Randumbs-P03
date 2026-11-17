import argparse
import csv
import re
import subprocess
from pathlib import Path


STRATEGIES = (
    "hibbard_succ", "hibbard_rand", "scapegoat_array", "scapegoat_inplace",
)
SIZES = (1000, 2000, 4000)
SEEDS = (20261117, 20261118, 20261119, 20261120, 20261121)


def main():
    parser = argparse.ArgumentParser(description="Run the D6 churn benchmark.")
    parser.add_argument("simulator", type=Path)
    parser.add_argument("output", type=Path, help="New directory for one workload")
    parser.add_argument("--schedule", default="uniform")
    parser.add_argument("--smoke", action="store_true",
                        help="Use 2n pairs per run to check the pipeline")
    args = parser.parse_args()
    simulator = args.simulator.resolve(strict=True)
    args.output.mkdir(parents=True)

    with (args.output / "runs.csv").open("w", newline="") as summary:
        writer = csv.writer(summary)
        writer.writerow(("strategy", "n", "seed", "schedule", "pairs",
                         "seconds", "samples"))
        for n in SIZES:
            pairs = 2 * n if args.smoke else n * n
            for seed in SEEDS:
                for strategy in STRATEGIES:
                    filename = f"{strategy}_{n}_{seed}.csv"
                    sample_path = args.output / filename
                    result = subprocess.run(
                        [str(simulator), "--strategy", strategy, "--n", str(n),
                         "--seed", str(seed), "--schedule", args.schedule,
                         "--pairs", str(pairs), "--sample", str(n),
                         "--out", str(sample_path)],
                        text=True, capture_output=True,
                    )
                    if result.returncode:
                        raise SystemExit(result.stderr.strip())
                    timing = re.search(r"\btime=([0-9.eE+-]+)s\b", result.stderr)
                    if timing is None:
                        raise SystemExit(f"Missing simulator timing for {filename}")
                    writer.writerow((strategy, n, seed, args.schedule, pairs,
                                     timing[1], filename))
                    summary.flush()
                    print(result.stderr.strip(), flush=True)


if __name__ == "__main__":
    main()
