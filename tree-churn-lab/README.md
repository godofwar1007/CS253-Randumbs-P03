# Tree Churn Lab

Open index.html in a browser, or serve this folder with Python.

```sh
python -m http.server 8787 --bind 127.0.0.1
```

The browser implements both Hibbard deletion policies and both scapegoat rebuild
methods. Presentation presets include every array split and each Stout–Warren
rotation for the six-node subtree under parent 70. Free lab supports seeded
churn and manual insert, delete, and search operations with a sorted-array oracle.

Benchmark charts and the final-depth table show recorded C++ results. Run the
repository benchmark harness to collect new full-scale measurements.

For Vercel, select this folder as the Root Directory, choose Other as the framework,
and leave the build command empty. The site uses static HTML, CSS, and JavaScript.
