## Quick start (VS Code on Windows)

Prerequisites:
...
# Meta reflection benchmark (RTTR vs EnTT)

This repository contains a small, focused C++ benchmark that compares runtime costs of meta reflection operations between RTTR and EnTT::meta on Windows/MSVC using vcpkg.

Highlights:
- Clean CMake project with presets and vcpkg manifest
- One-click build/run tasks for VS Code
- JSON output + pretty renderer (Python)

## Quick start (VS Code on Windows)

Prerequisites:
- Visual Studio 2022 with C++ (MSVC, CMake)
- vcpkg installed and VCPKG_ROOT set (classic install)
- Python 3 available on PATH

Steps:
1) Configure and build (Release):
   - Run task: “CMake: Configure (VS 2022, vcpkg)”
   - Run task: “CMake: Build (Release)”
2) Run benchmark and render table:
      - Run task: “Run meta_bench and render json” (uses `py`)
   
   The run task will auto-install Python deps (rich/tabulate) into the user site-packages if missing.

The run task will auto-install Python deps (rich/tabulate) into the user site-packages if missing.

## Manual build (CMake Presets)

From a Developer PowerShell for VS 2022:

```powershell
cd benchmark
cmake --preset vs-2022-vcpkg
cmake --build --preset vs-2022-release

# Run
# Run
& .\build-vs\Release\meta_bench.exe | py .\tools\render_table.py

```

Alternative (Ninja):

```powershell
cd benchmark
cmake --preset ninja-vcpkg-release
cmake --build --preset ninja-vcpkg-release

# Run
# Run
& .\build\meta_bench.exe | py .\tools\render_table.py

```

## Repository layout
- benchmark/ … CMake project (sources, presets, tools)
- .vscode/ … VS Code tasks and launch config

For deeper details, see `benchmark/README.md`.

## Notes
- Results are micro-benchmarks; compare relative values on your machine.
- If vcpkg is not on PATH, set VCPKG_ROOT to your install (e.g., C:\src\vcpkg).
