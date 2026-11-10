# Tools and build

Items marked CONFIRM must be settled by the team before code is merged.

## Language and build

C++17, built with CMake 3.16 or later. The shared build file is
CMakeLists.txt at the repository root. Shared headers are in include/.

    mkdir build && cd build
    cmake ..
    cmake --build .

## Compiler (CONFIRM)

Test builds use g++ or clang++ with sanitizers, through WSL or any Linux
machine, because MSVC provides AddressSanitizer only and MinGW on Windows
cannot run the sanitizers. Every member runs the tests through the same
route.

    cmake -DCHURN_SANITIZE=ON ..

Full experiment builds use the default Release configuration without
sanitizers, with one compiler and one set of flags for all runs, so that
timings are comparable.

## Python

Python 3 with matplotlib and pandas, used only to read results/*.csv and
produce plots. No Python is used inside the experiment loop.

## Repository rules

- Seeds are explicit; no component creates its own random generator.
- Results files are named strategy_n_seed.csv and written to results/.
- Do not commit build directories. A .gitignore is provided.