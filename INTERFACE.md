# Interface and conventions

This document is the single source of truth for how components fit together.
Any change requires agreement from all six members, because every structure
is compared on the same footing. The code form of the interface is
include/dictionary.hpp and include/rng.hpp.

## Interface

Every strategy implements `churn::Dictionary`. The simulator and the
differential tests depend only on this class.

- `bool insert(Key)`: returns true if inserted, false if the key was already
  present (structure unchanged).
- `bool erase(Key)`: returns true if erased, false if the key was absent
  (structure unchanged).
- `bool contains(Key) const`.
- `size_t size() const`.
- `uint64_t totalDepth() const`: sum of node depths over all keys, maintained
  incrementally. The oracle returns 0.
- `uint64_t rebuildCount() const`: number of rebuild events so far. A scapegoat
  subtree rebuild and a full-tree rebuild each count as one. Structures that
  never rebuild return 0.
- `void inorder(std::vector<Key>&) const`: appends all keys in ascending
  order. Used by the tests only.
- `bool validate() const`: checks the structure's own invariants (BST order,
  size fields, depth bookkeeping, weight balance where applicable). Tests only.
- `std::string name() const`: label used in CSV output. Fixed values:
  `oracle`, `hibbard_succ`, `hibbard_rand`, `scapegoat_array`,
  `scapegoat_inplace`.

## Keys

Keys are `int`. Duplicates are rejected.

## Depth convention

The root is at depth 0. The depth of a node is the number of edges from the
root. Average depth is `totalDepth() / size()`. The expected cost of a
successful search is the average depth plus one, so any comparison with
published search-cost figures must add one. All plots use average depth with
the root at 0.

## Randomness

One `churn::Rng` (std::mt19937_64) is created per run from an explicit seed
and passed by reference to every component that needs randomness. No
component creates its own generator. The seed is recorded in every CSV row.
The random Hibbard policy draws its predecessor or successor choice from this
shared generator.

## One churn pair

A tree of fixed size n is maintained. One pair consists of:

1. Delete a key chosen uniformly at random from the n keys currently stored.
2. Insert a fresh key drawn uniformly at random from [0, 2^30). If the draw
   is already present, draw again until it is not.

The size is n before the pair, n - 1 between the two steps, and n after. The
initial tree of n keys is built by n random insertions from the same
universe. The experiment runs at least n^2 pairs for n = 1000, 2000 and 4000.

## Output format

CSV, one row per sample, with the header
`strategy,n,seed,pair_index,avg_depth,rebuild_count`. Average depth is
sampled every n pairs. Files are written to results/.

## Obligations by member

- Structure A (D1): implement `Dictionary` twice, as `hibbard_succ` and
  `hibbard_rand`. Iterative code only. Maintain totalDepth incrementally.
- Structure B (D2): implement `Dictionary` as `scapegoat_array` and
  `scapegoat_inplace` (Stout and Warren). Count every rebuild.
- Oracle (D4): implement `Dictionary` as a sorted array, and write the
  differential harness against the interface only.
- Adversary (D5): write schedule generators that plug into the simulator in
  place of the uniform schedule, without changing the interface.
- Architect (D3): simulator, CSV output, shared headers, build setup.
- Analyst (D6): plots and prediction comparison, reading the CSV files only.