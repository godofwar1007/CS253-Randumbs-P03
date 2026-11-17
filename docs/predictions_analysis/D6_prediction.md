# D6 prediction: scapegoat rebuilds under random churn

The original model and prediction range are in commit `f994b56`. This revision
evaluates that model at the implementation's α = 2/3. The root is at depth zero.
See `docs/derivations/D6_250001054.pdf` for the amortised bound.

## Setup

- Start with n random distinct keys. Each churn pair deletes a uniformly
  selected live key and inserts a fresh uniform key, returning the size to n.
- A newly attached leaf triggers a rebuild if its depth exceeds
  h(n) = floor(log base 3/2 of n). Deletion triggers a shrink rebuild when
  the remaining size is below two thirds of the maximum recorded size.
- Use n = 1000, 2000, and 4000 and at least n² pairs per run.

## Predictions

1. **Zero deletion-triggered rebuilds during random churn.** The maximum
   recorded size is n and the minimum live size is n − 1. At α = 2/3 the
   shrink condition n − 1 < (2/3)n requires n < 3. Insertion-triggered
   rebuilds can still happen at the root.
2. **Rate per pair.** Each pair makes one new attachment. Approximate the
   attachment depth D using a random BST on n − 1 keys. Its probability
   generating function is the product of (k − 1 + 2z)/(k + 1) for k from
   1 to n − 1. The model rate r is P(D > h(n)). Rebuilding reduces depth
   locally, so the pre-measurement prediction for the observed rate is
   0.1r to r. This range is a heuristic.
3. **Dependence on n.** The model rates stay of similar magnitude at the
   three specified sizes, with steps where h(n) increases. The expected
   totals grow substantially as the number of pairs increases.
4. **The two rebuild variants agree.** With the same seed and workload,
   the array and in-place modes should have identical sampled average
   depths and cumulative rebuild counts. Compare elapsed times separately.

| α | n | h(n) | Model rebuilds per pair | Expected rebuilds in n² pairs |
|---|---:|---:|---:|---:|
| 2/3 | 1000 | 17 | 0.0848090710 | 84,809 |
| 2/3 | 2000 | 18 | 0.1161886291 | 464,755 |
| 2/3 | 4000 | 20 | 0.0985909340 | 1,577,455 |

Reproduce the table from the repo root with
`g++ -std=c++17 -O2 docs/analysis/pred.cpp -o pred` and then `./pred`.

## Comparison method

For P pairs in a seed, compute (count(P) − count(0))/P. Subtracting
count(0) removes rebuilds from the initial tree construction. Report the
observed/model ratio for every (n, seed, strategy), compare same-seed
sample series across the rebuild variants, and show the mean and sample
standard deviation across seeds. The average-depth plots use depth in
edges and show ln(n) and sqrt(n) as reference levels.

## Adversarial prediction for D5 to review

The existing adversarial schedule deletes the minimum live key and
inserts keys in descending order from the upper end of the key universe.
Consecutive large keys can build a deep leftward path in a high-key region,
increasing the chance of exceeding h(n). A rebuild can shorten that path.
The prediction is a higher rebuild rate than uniform churn at the same
size and operation budget. Measure both schedules with the same seeds and
ask D5 to confirm which generator and baseline to use for each target.
