(function () {
  "use strict";

  const strategies = {
    hibbard_succ: { name: "Hibbard · successor", short: "Successor", color: "#6642c7", description: "Two-child deletion promotes the next larger key." },
    hibbard_rand: { name: "Hibbard · random replacement", short: "Random replacement", color: "#bc3769", description: "Two-child deletion flips a coin for predecessor or successor." },
    scapegoat_array: { name: "Scapegoat · array rebuild", short: "Scapegoat array", color: "#147d65", description: "Deep insertions trigger a subtree rebuild via a sorted array." },
    scapegoat_inplace: { name: "Scapegoat · in-place rebuild", short: "Scapegoat in-place", color: "#a96615", description: "Deep insertions trigger Stout–Warren rotations." }
  };

  function randomFrom(seed) {
    let state = seed >>> 0;
    return () => {
      state = (Math.imul(state, 1664525) + 1013904223) >>> 0;
      return state / 4294967296;
    };
  }

  function node(key) { return { key, left: null, right: null, size: 1 }; }

  function measure(root, depth = 0) {
    if (!root) return { size: 0, sum: 0, height: -1 };
    const left = measure(root.left, depth + 1);
    const right = measure(root.right, depth + 1);
    root.size = 1 + left.size + right.size;
    return { size: root.size, sum: depth + left.sum + right.sum, height: 1 + Math.max(left.height, right.height) };
  }

  function inorder(root, out = []) {
    if (root) {
      inorder(root.left, out);
      out.push(root);
      inorder(root.right, out);
    }
    return out;
  }

  function shapeOf(count) {
    let slots = 1;
    while (2 * slots <= count + 1) slots *= 2;
    return { slots, holes: 2 * slots - 1 - count };
  }

  function arrayRebuild(root, count, observe) {
    const nodes = inorder(root);
    observe?.("array", root, nodes.map(item => item.key));
    const { slots, holes } = shapeOf(count);
    const holesThrough = position => position === slots ? holes : Math.floor(((position + 1) * holes - 1) / slots);
    function build(first, size, offset, width) {
      if (!size) return null;
      const half = width / 2;
      const leftSize = half >= 1 ? 2 * half - 1 - (holesThrough(offset + half) - holesThrough(offset)) : 0;
      const pivot = nodes[first + leftSize];
      if (observe) {
        const middle = first + leftSize;
        observe("split", root, {
          left: nodes.slice(first, middle).map(item => item.key),
          pivot: pivot.key,
          right: nodes.slice(middle + 1, first + size).map(item => item.key)
        });
      }
      pivot.left = build(first, leftSize, offset, half);
      pivot.right = build(first + leftSize + 1, size - leftSize - 1, offset + half, half);
      return pivot;
    }
    return build(0, count, 0, slots);
  }

  function inPlaceRebuild(root, count, observe) {
    const head = { right: root };
    let tail = head;
    while (tail.right) {
      const current = tail.right;
      if (!current.left) {
        tail = current;
      } else {
        const pivot = current.left;
        current.left = pivot.right;
        pivot.right = current;
        tail.right = pivot;
      }
    }

    observe?.("vine", head.right);

    function rotateLeft(parent) {
      const current = parent.right;
      const pivot = current.right;
      current.right = pivot.left;
      pivot.left = current;
      parent.right = pivot;
    }

    const { slots, holes } = shapeOf(count);
    let nextHole = 1;
    tail = head;
    for (let slot = 1; slot < slots; slot += 1) {
      if (slot === Math.floor(nextHole * slots / holes)) {
        nextHole += 1;
      } else {
        const leaf = tail.right.key;
        rotateLeft(tail);
        observe?.("leaf-rotation", head.right, { leaf, parent: tail.right.key });
      }
      tail = tail.right;
    }
    for (let spine = slots - 1; spine > 1; spine = Math.floor(spine / 2)) {
      tail = head;
      for (let i = 0; i < Math.floor(spine / 2); i += 1) {
        rotateLeft(tail);
        tail = tail.right;
      }
    }
    return head.right;
  }

  class Tree {
    constructor(strategy, policyRandom) {
      this.strategy = strategy;
      this.policyRandom = policyRandom;
      this.root = null;
      this.mark = 0;
      this.rebuilds = 0;
      this.stats = measure(null);
    }

    lookup(key) {
      let current = this.root;
      const path = [];
      while (current) {
        path.push(current);
        if (current.key === key) return { found: current, path };
        current = key < current.key ? current.left : current.right;
      }
      return { found: null, path };
    }

    get isScapegoat() { return this.strategy.startsWith("scapegoat"); }

    rebuild(target, parent) {
      const count = target.size;
      const before = this.onRebuild ? copyTree(this.root) : null;
      const observe = this.onRebuild ? (phase, subtree, details) => {
        // In the array trace, display the original tree while its pointer ranges are split.
        const root = phase === "split" ? copyTree(before) : copyTree(this.root, target, subtree);
        this.onRebuild(phase, root, target.key, details);
      } : undefined;
      observe?.("selected", target);
      const updated = this.strategy === "scapegoat_inplace"
        ? inPlaceRebuild(target, count, observe) : arrayRebuild(target, count, observe);
      if (!parent) { this.root = updated; this.mark = count; }
      else if (parent.left === target) parent.left = updated;
      else parent.right = updated;
      this.stats = measure(this.root);
      this.rebuilds += 1;
      return { key: target.key, newRoot: updated.key, count };
    }

    insert(key) {
      const { found, path } = this.lookup(key);
      if (found) return { changed: false, path: path.map(item => item.key) };
      const fresh = node(key);
      const parent = path.at(-1);
      if (!parent) this.root = fresh;
      else if (key < parent.key) parent.left = fresh;
      else parent.right = fresh;
      this.stats = measure(this.root);
      this.mark = Math.max(this.mark, this.stats.size);
      const result = { changed: true, path: [...path, fresh].map(item => item.key) };
      if (this.isScapegoat && path.length > Math.floor(Math.log(this.stats.size) / Math.log(1.5))) {
        let child = fresh;
        for (let index = path.length - 1; index >= 0; index -= 1) {
          const candidate = path[index];
          if (3 * child.size > 2 * candidate.size) {
            result.rebuilt = this.rebuild(candidate, path[index - 1]);
            return result;
          }
          child = candidate;
        }
        throw new Error("Deep insertion did not yield a scapegoat.");
      }
      return result;
    }

    erase(key) {
      const { found, path } = this.lookup(key);
      if (!found) return { changed: false, path: path.map(item => item.key) };
      const result = { changed: true, path: path.map(item => item.key) };
      let parent = path.at(-2);
      let removed = found;
      if (found.left && found.right) {
        const predecessor = this.strategy === "hibbard_rand" && this.policyRandom() < .5;
        const nearer = predecessor ? "left" : "right";
        const further = predecessor ? "right" : "left";
        parent = found;
        removed = found[nearer];
        while (removed[further]) {
          parent = removed;
          removed = removed[further];
        }
        result.replacement = removed.key;
        result.policy = predecessor ? "predecessor" : "successor";
        found.key = removed.key;
      }
      const promoted = removed.left || removed.right;
      if (!parent) this.root = promoted;
      else if (parent.left === removed) parent.left = promoted;
      else parent.right = promoted;
      this.stats = measure(this.root);
      if (!this.root) this.mark = 0;
      else if (this.isScapegoat && 3 * this.stats.size < 2 * this.mark) result.rebuilt = this.rebuild(this.root, null);
      return result;
    }

    values() { return inorder(this.root).map(item => item.key); }
  }

  class Lab {
    constructor(strategy, workload, size, seed) {
      this.strategy = strategy;
      this.workload = workload;
      this.rng = randomFrom(seed);
      this.tree = new Tree(strategy, randomFrom(seed ^ 0x9e3779b9));
      this.keys = [];
      this.pairs = 0;
      this.high = 100;
      this.low = 0;
      this.history = [];
      this.path = [];
      this.rebuilt = null;
      const pool = [48,26,72,14,35,61,86,8,20,30,41,54,66,78,94];
      while (pool.length < size) {
        const key = 1 + Math.floor(this.rng() * 99);
        if (!pool.includes(key)) pool.push(key);
      }
      if (seed !== 42 || size !== 15) {
        for (let i = pool.length - 1; i > 0; i -= 1) {
          const j = Math.floor(this.rng() * (i + 1));
          [pool[i], pool[j]] = [pool[j], pool[i]];
        }
      }
      for (const key of pool.slice(0, size)) this.apply("insert", key);
      this.initialRebuilds = this.tree.rebuilds;
      this.path = [];
      this.rebuilt = null;
      this.record("Start", "Inserted " + size + " keys. Root depth is zero.");
    }

    record(kind, description) {
      this.history.unshift({ kind, description });
      if (this.history.length > 5) this.history.pop();
    }

    apply(action, key) {
      const result = action === "insert" ? this.tree.insert(key) : this.tree.erase(key);
      const index = this.keys.findIndex(value => value >= key);
      const existed = index !== -1 && this.keys[index] === key;
      if (action === "insert" && !existed) this.keys.splice(index === -1 ? this.keys.length : index, 0, key);
      if (action === "erase" && existed) this.keys.splice(index, 1);
      if (result.changed !== (action === "insert" ? !existed : existed)) this.operationMismatch = true;
      return result;
    }

    manual(action, key) {
      if (action === "find") {
        const { found, path } = this.tree.lookup(key);
        this.operationMismatch ||= Boolean(found) !== this.keys.includes(key);
        this.path = path.map(item => item.key);
        this.rebuilt = null;
        this.record("Find", found ? "Found " + key + " at depth " + (path.length - 1) + "." : key + " is absent.");
        return;
      }
      if (action === "insert" && !this.keys.includes(key) && this.keys.length >= 63) {
        this.record("Limit", "The visual lab holds up to 63 keys. Delete a key or reset.");
        return;
      }
      const result = this.apply(action, key);
      this.path = result.path;
      this.rebuilt = result.rebuilt || null;
      const changed = action === "insert" ? "Inserted " : "Deleted ";
      const description = result.changed ? changed + key + "." + describe(result) :
        key + (action === "insert" ? " is already present." : " is absent.");
      this.record(action === "insert" ? "Insert" : "Delete", description);
      return result;
    }

    step() {
      if (!this.keys.length) {
        this.record("Paused", "Insert a key or reset to run fixed-size churn.");
        return false;
      }
      let index = 0;
      if (this.workload === "max_delete_low_insert") index = this.keys.length - 1;
      else if (this.workload !== "min_delete_high_insert") index = Math.floor(this.rng() * this.keys.length);
      const victim = this.keys[index];
      const erased = this.apply("erase", victim);
      let fresh;
      if (this.workload === "min_delete_high_insert") {
        this.high = Math.max(this.high + 1, (this.keys.at(-1) ?? 0) + 1);
        fresh = this.high;
      } else if (this.workload !== "uniform") {
        this.low = Math.min(this.low - 1, (this.keys[0] ?? 0) - 1);
        fresh = this.low;
      } else {
        do { fresh = Math.floor(this.rng() * 100); } while (this.keys.includes(fresh));
      }
      const inserted = this.apply("insert", fresh);
      this.pairs += 1;
      this.path = inserted.path;
      this.rebuilt = inserted.rebuilt || erased.rebuilt || null;
      this.record("Pair " + this.pairs, "Deleted " + victim + " → inserted " + fresh + "." + describe(erased) + describe(inserted));
      return true;
    }

    agrees() {
      if (this.operationMismatch) return false;
      const actual = this.tree.values();
      return actual.length === this.keys.length && actual.every((key, i) => key === this.keys[i]);
    }
  }

  function describe(event) {
    const replacement = event.policy ? " Used " + event.policy + " " + event.replacement + "." : "";
    const rebuilt = event.rebuilt ? " Rebuilt " + event.rebuilt.count + " nodes at " + event.rebuilt.key + "." : "";
    return replacement + rebuilt;
  }

  function copyTree(root, target, replacement) {
    if (target && root === target) return copyTree(replacement);
    if (!root) return null;
    return { key: root.key, left: copyTree(root.left, target, replacement),
      right: copyTree(root.right, target, replacement) };
  }

  function makeDemo(id) {
    const valid = ["hibbard_succ", "hibbard_rand", "scapegoat_insert",
      "scapegoat_array", "scapegoat_inplace", "scapegoat_shrink"];
    if (!valid.includes(id)) throw new Error("Unknown demo: " + id);
    const strategy = id === "scapegoat_insert" || id === "scapegoat_shrink" ? "scapegoat_array" : id;
    const lab = new Lab(strategy, "uniform", 0, 42);
    const frames = [];
    function frame(label, detail, options = {}) {
      const root = options.root ?? copyTree(lab.tree.root);
      const keys = options.keys ?? [...lab.keys];
      const actual = inorder(root).map(item => item.key);
      const agrees = !lab.operationMismatch && actual.length === keys.length &&
        actual.every((key, index) => key === keys[index]);
      frames.push({ label, detail, array: options.array, lab: {
        strategy, pairs: 0, keys, agrees: () => agrees,
        tree: { root, stats: measure(root), rebuilds: lab.tree.rebuilds },
        path: options.path ?? [],
        rebuilt: options.highlight === undefined ? lab.rebuilt : { newRoot: options.highlight },
        initialRebuilds: 0, history: [...lab.history]
      }});
    }
    function setup(keys) {
      lab.history = [];
      for (const key of keys) lab.apply("insert", key);
    }

    if (strategy.startsWith("hibbard")) {
      setup([40, 20, 60, 50, 70]);
      frame("Delete a node with two children", "Key 40 has both a left and a right child.", { path: [40] });
      const event = lab.manual("erase", 40);
      frame("Promote the " + event.policy,
        "Key " + event.replacement + " replaces 40. Its old position is spliced out.", { highlight: event.replacement });
      lab.manual("erase", event.policy === "successor" ? 20 : 50);
      frame("Remove a leaf", "A leaf has no children. Its parent simply drops that link.");
      lab.manual("erase", 60);
      frame("Promote the only child", "Key 60 has one remaining child, 70. Its parent now points to 70.", { highlight: 70 });
    } else if (id === "scapegoat_insert") {
      setup([6, 5, 4, 3]);
      frame("Insert a new leaf", "Key 2 will go below 3 in this descending BST.");
      lab.tree.onRebuild = (phase, root, target) => {
        if (phase === "selected") frame("Depth 4 exceeds the limit of 3",
          "Walk up from 2. At node 5, its child contains 3 of the 4 nodes, above α = 2/3.",
          { root, keys: [2, 3, 4, 5, 6], path: [6, 5, 4, 3, 2], highlight: target });
      };
      lab.manual("insert", 2);
      frame("Reconnect the rebuilt subtree", "Key 6 stays in place. Its left pointer now points to key 3.", { highlight: 3 });
    } else if (id === "scapegoat_shrink") {
      setup([40, 20, 60, 10, 30, 50, 70]);
      frame("Start with a high-water mark of 7", "A whole-tree rebuild is triggered when the live count drops below 2/3 of 7.");
      lab.manual("erase", 70);
      lab.manual("erase", 60);
      frame("Five keys remain", "The live count is still above the threshold. No rebuild is needed.");
      lab.manual("erase", 50);
      frame("Four keys trigger a whole-tree rebuild", "The rebuilt root is shown in pink. The high-water mark resets to 4.",
        { highlight: lab.tree.root.key });
    } else {
      // Seed the slide’s selected subtree as a plain BST fixture.
      // Then run the same rebuild routine the normal scapegoat updates call.
      lab.tree.strategy = "hibbard_succ";
      setup([70, 10, 20, 30, 40, 50, 60]);
      lab.tree.strategy = strategy;
      frame("Selected subtree under 70", "The six-node chain starts at 10. We will rebuild that subtree.", { highlight: 10 });
      lab.tree.onRebuild = (phase, root, target, details) => {
        if (phase === "array") frame("Collect the sorted node pointers",
          "The inorder traversal gives this sorted array. Next, split the array into subtree ranges.",
          { root, array: details });
        if (phase === "split") {
          const { pivot, left, right } = details;
          const singleton = !left.length && !right.length;
          frame(singleton ? "Key " + pivot + " becomes a leaf" : "Choose " + pivot + " and split the range",
            singleton ? "Only one key remains in this range." :
              "The shared leaf layout selects middle key " + pivot + ". " +
              left.length + " keys go left and " + right.length + " go right.",
            { root, array: details, highlight: pivot });
        }
        if (phase === "vine") frame("Flatten to a right-going vine",
          "Every link already points right in this example, so the right-rotation pass leaves it as it is.", { root });
        if (phase === "leaf-rotation") frame("Left rotation at " + details.leaf,
          "Lift " + details.parent + " on the vine. Key " + details.leaf +
          " becomes its left child.", { root, path: [details.leaf], highlight: details.parent });
      };
      lab.rebuilt = lab.tree.rebuild(lab.tree.root.left, lab.tree.root);
      lab.record("Rebuild", describe({ rebuilt: lab.rebuilt }).trim());
      frame("Root 40 replaces root 10",
        id === "scapegoat_inplace"
          ? "A left rotation at 20 lifts 40. The parent 70 now points to 40."
          : "The parent 70 now points to 40. The left and right ranges are linked into place.",
        { highlight: 40 });
    }
    return frames;
  }

  const api = { Lab, Tree, strategies, randomFrom, makeDemo };
  if (typeof module !== "undefined") module.exports = api;
  else window.TreeLab = api;
}());
