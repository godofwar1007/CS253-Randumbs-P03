<h1>Search Trees Under Long-Running Churn</h1>

<p><strong>CS 253 / MA 253 — Data Structures and Algorithms Laboratory, Project </strong></p>

<h2>Project Overview Scenario</h2>

<p>
An index is subjected to days of random churn, with insertions and deletions in equal measure. The question is whether an ordinary binary search tree slowly degrades under this load, and what it costs to prevent that.
</p>

<h2>Team Roles</h2>

<table>
  <thead>
    <tr>
      <th>Member</th>
      <th>Role</th>
      <th>Responsibility</th>
      <th>Derivation</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td>Mayank Khoria</td>
      <td>Structure A</td>
      <td>Implement the iterative BST and both Hibbard deletion policies.</td>
      <td>D1</td>
    </tr>
    <tr>
      <td>Parth Pawar</td>
      <td>Structure B</td>
      <td>Implement the scapegoat tree and both rebuild methods.</td>
      <td>D2</td>
    </tr>
    <tr>
      <td>Eklavya</td>
      <td>Architect</td>
      <td>Define the common interface and build the churn simulator.</td>
      <td>D3</td>
    </tr>
    <tr>
      <td>Tanush Bansal</td>
      <td>Oracle</td>
      <td>Build the sorted-array oracle, invariant checkers, and seeded differential test with failure shrinking.</td>
      <td>D4</td>
    </tr>
    <tr>
      <td>Chetan Yogbharti</td>
      <td>Adversary</td>
      <td>Generate workloads that degrade the always-successor BST and trigger frequent scapegoat rebuilds.</td>
      <td>D5</td>
    </tr>
    <tr>
      <td>Parth Kalia</td>
      <td>Analyst</td>
      <td>Model predicted costs, run benchmarks, and plot measured results.</td>
      <td>D6</td>
    </tr>
  </tbody>
</table>

## Oracle tests

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The differential runner uses 1,000,000 operations and seed 20260928 by default.
All four real tree variants are connected in `oracle/trees.hpp`. The full test
compares each with the sorted-array oracle and checks invariants after every
operation. The separate `harness_test` checks the harness against a test-only
reference and deliberately broken code; `oracle_checks` tests deletion cases,
rebuilds and checker detection of corrupted metadata.
See [oracle/README.md](oracle/README.md) for commands, expected output, tree
integration and failure replay.

The teammate's adversarial schedule can also be checked against all four trees:

```sh
./build/differential --tree all --workload adversarial --size 128 --operations 1000000 --seed 20260928
```

The operation count includes initial insertions followed by complete
delete/insert pairs. Use `--save build/adversarial.txt` to export the identical
input used for every tree, then `--replay build/adversarial.txt` to rerun it.
