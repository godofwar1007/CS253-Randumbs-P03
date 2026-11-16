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

## Python

Python 3 with matplotlib and pandas, used only to read results/*.csv and
produce plots. No Python is used inside the experiment loop.

## Repository rules

- Seeds are explicit; no component creates its own random generator.
- Results files are named strategy_n_seed.csv and written to results/.
- Do not commit build directories. A .gitignore is provided.
