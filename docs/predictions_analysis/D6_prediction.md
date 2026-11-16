# D6 prediction: scapegoat rebuild frequency under random churn

Committed before any full experimental run. Depth convention: root = 0.
Full derivation: `docs/derivations/D6.pdf` (Section 4). Reproduce numbers: `g++ -std=c++17 -O2 analysis/pred.cpp -o pred && ./pred <alpha>`.

## Setup assumed
- alpha = **TODO: fill in from D2**, trigger: new node depth > floor(log_{1/alpha}(size)), full rebuild when size < alpha * maxSize
- churn: n in {1000, 2000, 4000}, one pair = delete uniform key + insert fresh uniform key

## Predictions
1. **No full-tree rebuilds.** Size never exceeds n, so maxSize = n and size < alpha*maxSize needs n < 1/(1-alpha) (n < 5 for alpha <= 0.8). All counted rebuilds are subtree rebuilds.
2. **Rebuilds per pair = P(new-leaf depth > floor(log_{1/alpha} n))**. Model (random-BST insertion depth, PGF prod_{k=1}^{n-1} (k-1+2z)/(k+1)) gives the rate in the table below. This is an upper estimate; committed range for the observed rate: 0.1x to 1x of the model value.
3. **Roughly flat in n**: no systematic growth with n; small steps where floor(log_{1/alpha} n) jumps. Total rebuilds over n^2 pairs grow like n^2.
4. **scapegoat_array and scapegoat_inplace have exactly identical rebuild_count and avg_depth** for the same seed.
5. Comparison method: observed rate = (count(P) - count(0)) / P per seed (count(0) includes the initial build); report observed/model per (n, seed).

## Model table (run pred; paste the rows for the alpha in use)
| alpha | n | h | rate per pair | expected rebuilds in n^2 pairs |
|---|---|---|---|---|
| 0.6 | 1000 | 13 | 4.186e-01 | 418,630 |
| 0.6 | 2000 | 14 | 4.671e-01 | 1,868,260 |
| 0.6 | 4000 | 16 | 4.027e-01 | 6,443,188 |
| 0.65 | 1000 | 16 | 1.377e-01 | 137,682 |
| 0.65 | 2000 | 17 | 1.775e-01 | 710,173 |
| 0.65 | 4000 | 19 | 1.501e-01 | 2,402,041 |
| 0.7 | 1000 | 19 | 2.726e-02 | 27,258 |
| 0.7 | 2000 | 21 | 2.417e-02 | 96,681 |
| 0.7 | 4000 | 23 | 2.136e-02 | 341,682 |
| 0.75 | 1000 | 24 | 6.459e-04 | 646 |
| 0.75 | 2000 | 26 | 6.969e-04 | 2,788 |
| 0.75 | 4000 | 28 | 7.206e-04 | 11,530 |
| 0.8 | 1000 | 30 | 1.606e-06 | 2 |
| 0.8 | 2000 | 34 | 2.937e-07 | 1 |
| 0.8 | 4000 | 37 | 1.622e-07 | 3 |

## Adversarial-schedule argument
TODO: joint paragraph with D5 (why the max-rebuild schedule should beat random churn).
