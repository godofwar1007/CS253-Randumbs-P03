# Tools and build

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
