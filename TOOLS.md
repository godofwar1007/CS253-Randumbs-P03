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