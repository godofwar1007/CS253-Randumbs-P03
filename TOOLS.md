# Tools and build

## AI assistance

| Tool and date | Used by | Used for | Files affected | Changes and verification |
|---|---|---|---|---|
| OpenAI Codex, 2026-11-14 | Tanush Bansal | Sorted-array oracle | oracle/sorted_array.hpp | Generated against the shared interface; edge cases and 100,000 seeded operations checked against a test-only std::set |
| OpenAI Codex, 2026-11-17 | Tanush Bansal | Differential runner, seeded workloads, replay, shrinking, tests and documentation | oracle/harness.hpp, oracle/differential.cpp, oracle/trees.hpp, tests/harness_test.cpp, oracle/README.md, CMakeLists.txt, README.md, TOOLS.md | Used plain structs and functions without code comments; compiled and checked one million operations against a test-only reference, reproducibility, injected faults and shrinking; real-tree testing still awaits integration |

### Representative prompts

- 2026-11-14: "implement the sorted array oracle"
- 2026-11-17: "explain how we'll build the differential harness"
- 2026-11-17: "write the code and write it human like, not too high-fi stuff, also dont write comments in the code"

## Language and build

C++17, built with CMake 3.16 or later. The shared build file is
CMakeLists.txt at the repository root. Shared headers are in include/.

    mkdir build && cd build
    cmake ..
    cmake --build .

## Compiler

Sanitizer test builds are made by a member with WSL or Linux available, with
g++ and -DCHURN_SANITIZE=ON. Everyone else builds with the Windows MinGW g++,
which has no sanitizer runtime.

    cmake -DCHURN_SANITIZE=ON ..

Timed experiments use one agreed compiler and the Release flags in
CMakeLists.txt, so that timings are comparable.

## Benchmarks and figures

The build, benchmark runner, model calculator, and SVG figure generator
use C++17. Bash orchestrates compilation, tests and experiments.

## Repository rules

- Seeds are explicit; no component creates its own random generator.
- Results files are named strategy_n_seed.csv and written to results/.
- Do not commit build directories. A .gitignore is provided.

## Structure B (D2) tooling

- OpenAI Codex reviewed the assignment, the shared dictionary interface, and
  the Galperin–Rivest and Stout–Warren papers. It assisted with the scapegoat
  implementation, both rebuild routines, the focused tests, and simulator
  and CMake integration.
- During verification, the shrink-test fixture was changed to insert
  medians in level order so setup does not trigger a rebuild. MinGW g++
  was used to run the focused tests in release and debug builds and to
  check both modes on seeded churn at all three assignment sizes.
- CMake and Ninja built the integrated simulator and test target. CTest
  ran the focused suite, and Python compared the simulator CSV samples
  from both modes on the same 100,000-pair workload.
- OpenAI Codex drafted the D2 LaTeX proof and checked it against the
  implementation and Galperin and Rivest's lemmas. The draft distinguishes
  the live-size insertion trigger from the high-water validator bound and
  includes a five-node example. Tectonic compiles the source, and PyMuPDF
  renders the pages for inspection.

## D6 tooling

- Parth Kalia used OpenAI Codex on 2026-11-17 to assist with the prediction
  calculator, C++ benchmark and figure tools, integration of the existing
  D5 schedules, the revised derivation PDF, the report, and the D6 slide.
- Verification compared the full raw records, model table and figure
  inputs, and exercised the CMake/CTest workflow. Zig with Clang ran the
  measured Release experiments. The presentation authoring runtime was
  used for the slide and its preview.
