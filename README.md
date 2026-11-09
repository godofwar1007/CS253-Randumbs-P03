<h1>Search Trees Under Long-Running Churn</h1>

<p><strong>CS 253 / MA 253 — Data Structures and Algorithms Laboratory, Project </strong></p>

<h2>Project Overview</h2>

<p>
  We study how repeated insertions and deletions affect the average node depth
  of a search tree. We compare an ordinary BST using always-successor Hibbard
  deletion, an ordinary BST that randomly chooses a predecessor or successor
  for two-child deletion, and a scapegoat tree with both array-based and
  in-place Stout–Warren rebuilds.
</p>

<p>
  The churn simulator keeps the number of keys fixed after each insert–delete
  pair. We run at least n<sup>2</sup> pairs for each of n = 1,000, 2,000,
  and 4,000. A sorted-array oracle checks the correctness of the tree
  implementations.
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
