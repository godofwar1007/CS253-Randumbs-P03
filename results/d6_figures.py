import argparse
import csv
import math
from collections import defaultdict
from pathlib import Path
from statistics import mean, stdev

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt


SCAPEGOAT = ("scapegoat_array", "scapegoat_inplace")


def read_rows(path):
    with path.open(newline="") as stream:
        return list(csv.DictReader(stream))


def load_runs(raw):
    groups = defaultdict(dict)
    for run in read_rows(raw / "runs.csv"):
        strategy, n, seed = run["strategy"], int(run["n"]), int(run["seed"])
        samples = read_rows(raw / run["samples"])
        if any((row["strategy"], int(row["n"]), int(row["seed"])) !=
               (strategy, n, seed) for row in samples):
            raise ValueError(f"Sample metadata disagrees with {run['samples']}")
        pairs = [int(row["pair_index"]) for row in samples]
        if (not pairs or pairs[0] != 0 or pairs[-1] != int(run["pairs"]) or
                any(a >= b for a, b in zip(pairs, pairs[1:]))):
            raise ValueError(f"Invalid sample times in {run['samples']}")
        if seed in groups[n, strategy]:
            raise ValueError(f"Duplicate run for {strategy}, n={n}, seed={seed}")
        groups[n, strategy][seed] = (
            pairs, [float(row["avg_depth"]) for row in samples],
            [int(row["rebuild_count"]) for row in samples], float(run["seconds"]),
        )
    if not groups or any(len(runs) < 5 for runs in groups.values()):
        raise ValueError("Every size and strategy must have at least five runs")
    return groups


def save(figure, output, stem, caption):
    figure.text(0.02, 0.015, caption, fontsize=8, wrap=True)
    figure.tight_layout(rect=(0, 0.085, 1, 1))
    figure.savefig(output / f"{stem}.png", dpi=180)
    plt.close(figure)


def plot_depth(groups, raw, output):
    for n in sorted({size for size, _ in groups}):
        figure, axes = plt.subplots(figsize=(8.5, 4.6))
        for (size, strategy), runs in sorted(groups.items()):
            if size != n:
                continue
            pairs = next(iter(runs.values()))[0]
            if any(run[0] != pairs for run in runs.values()):
                raise ValueError(f"Inconsistent sample times for {strategy}, n={n}")
            depths = list(zip(*(run[1] for run in runs.values())))
            averages = [mean(values) for values in depths]
            spread = [stdev(values) for values in depths]
            line, = axes.plot(pairs, averages, label=strategy)
            axes.fill_between(pairs, [a - s for a, s in zip(averages, spread)],
                              [a + s for a, s in zip(averages, spread)],
                              color=line.get_color(), alpha=0.16)
        axes.axhline(math.log(n), linestyle="--", color="gray", label="ln n")
        axes.axhline(math.sqrt(n), linestyle=":", color="gray", label="square root of n")
        axes.set(title=f"Average depth during churn (n = {n:,})",
                 xlabel="Churn pairs", ylabel="Average node depth (edges)")
        axes.legend()
        save(figure, output, f"depth_{n}",
             f"Compare depth over time; shading shows ±1 sample SD.\n"
             f"Source: {raw}/runs.csv and its sample CSVs. Script: results/d6_figures.py.")


def compare(groups, predictions, raw, model, output):
    sizes = sorted({n for n, _ in groups})
    rates = defaultdict(list)
    with (output / "rebuild_comparison.csv").open("w", newline="") as stream:
        writer = csv.writer(stream)
        writer.writerow(("n", "seed", "strategy", "pairs", "predicted_rate",
                         "observed_rate", "observed_over_predicted", "within_model_range",
                         "variants_match"))
        for n in sizes:
            array, inplace = (groups[n, strategy] for strategy in SCAPEGOAT)
            if array.keys() != inplace.keys():
                raise ValueError(f"Scapegoat runs have different seeds at n={n}")
            for seed in sorted(array):
                match = array[seed][:3] == inplace[seed][:3]
                for strategy, runs in zip(SCAPEGOAT, (array, inplace)):
                    pairs, _, counts, _ = runs[seed]
                    rate = (counts[-1] - counts[0]) / pairs[-1]
                    prediction = predictions[n]
                    ratio = rate / prediction
                    writer.writerow((n, seed, strategy, pairs[-1], prediction, rate,
                                     ratio, 0.1 <= ratio <= 1.0, match))
                    rates[n, strategy].append(rate)
    figure, axes = plt.subplots(figsize=(8.5, 4.6))
    predicted = [predictions[n] for n in sizes]
    axes.plot(sizes, predicted, "k--", label="Random BST model")
    axes.fill_between(sizes, [0.1 * value for value in predicted], predicted,
                      color="gray", alpha=0.18, label="Predicted range")
    for strategy, marker in zip(SCAPEGOAT, ("o", "x")):
        values = [rates[n, strategy] for n in sizes]
        axes.errorbar(sizes, [mean(group) for group in values],
                      yerr=[stdev(group) for group in values], marker=marker, capsize=4,
                      label=strategy)
    axes.set(title="Predicted and measured rebuild rates", xlabel="Tree size (keys)",
             ylabel="Rebuild events per churn pair", xticks=sizes)
    axes.legend()
    save(figure, output, "rebuild_rate",
         f"Compare measured rates with the predicted band. Error bars show ±1 sample SD.\n"
         f"Source: {raw}/runs.csv, its sample CSVs, and {model}. Script: results/d6_figures.py.")


def main():
    parser = argparse.ArgumentParser(description="Generate D6 figures from benchmark CSVs.")
    parser.add_argument("raw", type=Path)
    parser.add_argument("predictions", type=Path, help="CSV emitted by the D6 prediction program")
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    groups = load_runs(args.raw)
    args.output.mkdir(parents=True, exist_ok=True)
    plot_depth(groups, args.raw, args.output)
    if {run["schedule"] for run in read_rows(args.raw / "runs.csv")} == {"uniform"}:
        predictions = {int(row["n"]): float(row["predicted_rate"])
                       for row in read_rows(args.predictions)}
        compare(groups, predictions, args.raw, args.predictions, args.output)
    with (args.output / "timing_summary.csv").open("w", newline="") as stream:
        writer = csv.writer(stream)
        writer.writerow(("n", "strategy", "runs", "mean_seconds", "sd_seconds"))
        for (n, strategy), runs in sorted(groups.items()):
            seconds = [run[3] for run in runs.values()]
            writer.writerow((n, strategy, len(seconds), mean(seconds), stdev(seconds)))


if __name__ == "__main__":
    main()
