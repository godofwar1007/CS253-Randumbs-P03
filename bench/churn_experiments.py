"""Run fixed-size churn experiments and plot their recorded samples."""

import argparse
import csv
import platform
import statistics
import subprocess
from collections import defaultdict
from pathlib import Path

TREES = ("hibbard_succ", "hibbard_rand", "scapegoat_array", "scapegoat_inplace")
EXPERIMENTS = {
    "uniform_churn": [(tree, "uniform") for tree in TREES],
    "hibbard_adversary": [
        (tree, schedule)
        for tree in TREES[:2]
        for schedule in ("uniform", "min_delete_high_insert", "max_delete_low_insert")
    ],
    "scapegoat_adversary": [
        (tree, schedule)
        for tree in TREES[2:]
        for schedule in ("uniform", "scapegoat_stress")
    ],
}
SEEDS = (20260928, 20260929, 20260930, 20261001, 20261002)


def collect(executable, name, cases, sizes, seeds, raw_dir):
    path = raw_dir / f"{name}.csv"
    temporary = path.with_suffix(".tmp")
    with temporary.open("w", encoding="ascii", newline="") as output:
        for index, (tree, schedule, size, seed) in enumerate(
            (tree, schedule, size, seed)
            for size in sizes
            for tree, schedule in cases
            for seed in seeds
        ):
            print(f"{name}: {tree} {schedule} n={size} seed={seed}", flush=True)
            data = subprocess.check_output(
                [executable, tree, schedule, str(size), str(seed), str(size**2), str(size**2 // 100)],
                text=True,
            ).splitlines()
            output.writelines(f"{line}\n" for line in data[min(index, 1):])
    temporary.replace(path)


def plot(name, raw_dir, figures_dir):
    import math

    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    grouped = defaultdict(lambda: defaultdict(lambda: defaultdict(list)))
    rebuild_baseline = {}
    metric = "rebuild_count" if name == "scapegoat_adversary" else "avg_depth"
    path = raw_dir / f"{name}.csv"
    with path.open(encoding="ascii", newline="") as source:
        for row in csv.DictReader(source):
            n = int(row["n"])
            pair = int(row["pair_index"])
            label = f'{row["strategy"]} / {row["schedule"]}'
            value = float(row[metric])
            if metric == "rebuild_count":
                run = (n, label, row["seed"])
                if pair == 0:
                    rebuild_baseline[run] = value
                value -= rebuild_baseline[run]
            grouped[n][label][pair].append(value)

    for n, series in grouped.items():
        fig, ax = plt.subplots(figsize=(10, 5.5), layout="constrained")
        for label, samples in series.items():
            pairs = sorted(samples)
            means = [statistics.mean(samples[pair]) for pair in pairs]
            line, = ax.plot(pairs, means, label=label)
            ax.fill_between(
                pairs, [min(samples[pair]) for pair in pairs],
                [max(samples[pair]) for pair in pairs], color=line.get_color(), alpha=0.12,
            )
        if name == "uniform_churn":
            ax.axhline(math.log(n), linestyle="--", color="gray", label="ln(n)")
            ax.axhline(math.sqrt(n), linestyle=":", color="black", label="sqrt(n)")
        ax.set(
            title=f"{name.replace('_', ' ').capitalize()} (n = {n})",
            xlabel="Churn (insert-delete pairs)",
            ylabel="Rebuilds since initial build (count)" if metric == "rebuild_count" else "Average node depth (edges)",
        )
        ax.legend(fontsize="small")
        fig.supxlabel(
            f"Mean and min-max across seeds. Source: {path.name}; script: bench/churn_experiments.py.",
            fontsize="small",
        )
        fig.savefig(figures_dir / f"{name}_n{n}.png", dpi=160)
        plt.close(fig)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("runner", nargs="?", type=Path)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--smoke", action="store_true")
    mode.add_argument("--full", action="store_true")
    mode.add_argument("--figures-only", action="store_true")
    args = parser.parse_args()

    output = Path("build-run-all/smoke-results" if args.smoke else "results")
    raw, figures = output / "raw", output / "figures"
    raw.mkdir(parents=True, exist_ok=True)
    figures.mkdir(parents=True, exist_ok=True)

    if not args.figures_only:
        if args.runner is None:
            parser.error("runner is required to collect measurements")
        (raw / "environment.txt").write_text(
            f"Platform: {platform.platform()}\nProcessor: {platform.processor()}\n"
            f"Python: {platform.python_version()}\n", encoding="utf-8",
        )
        for name, cases in EXPERIMENTS.items():
            collect(args.runner.resolve(), name, cases, (64,) if args.smoke else (1000, 2000, 4000),
                    SEEDS[:2] if args.smoke else SEEDS, raw)

    for name in EXPERIMENTS:
        plot(name, raw, figures)


if __name__ == "__main__":
    main()
