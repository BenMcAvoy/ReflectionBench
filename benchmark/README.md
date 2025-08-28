# RTTR vs EnTT::meta micro-benchmark

This project builds a focused benchmark for field and method access using RTTR and EnTT::meta on MSVC with vcpkg.

## Prereqs
- Windows + Visual Studio 2022 with C++ workload
- vcpkg installed (classic or manifest mode)

To open a PowerShell with MSVC and CMake in PATH, use:

```powershell
powershell.exe -NoExit -Command "&{Import-Module \"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\Common7\\Tools\\Microsoft.VisualStudio.DevShell.dll\"; Enter-VsDevShell 63a93980 -SkipAutomaticLocation -DevCmdArguments \"-arch=x64 -host_arch=x64\"}"
```

## Configure & Build (CMake + vcpkg)

Make sure the environment variable `VCPKG_ROOT` is set or pass the toolchain path explicitly.

```powershell
# From the repository root or the benchmark folder
$root = "$PSScriptRoot"; if (-not $root) { $root = (Get-Location).Path }
$src = Join-Path $root 'benchmark'
$build = Join-Path $src 'build'
New-Item -ItemType Directory -Force -Path $build | Out-Null

# Use vcpkg toolchain (classic layout)
$toolchain = "$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake"

cmake -S $src -B $build -G "Ninja" -DCMAKE_TOOLCHAIN_FILE=$toolchain -DCMAKE_BUILD_TYPE=Release
cmake --build $build --config Release -v
```

If using the Visual Studio generator instead of Ninja, omit `-DCMAKE_BUILD_TYPE` and choose the configuration when building.

## Run
```powershell
$exe = Join-Path $build 'Release/meta_bench.exe'
if (-Not (Test-Path $exe)) { $exe = Join-Path $build 'meta_bench.exe' }
& $exe | py .\tools\render_table.py
```

## Notes
- The benchmark resolves meta members once and then measures get/set/invoke hot paths.
- A trivial checksum is accumulated to avoid dead code elimination.
- Times are approximate microbenchmarks; compare relative values.
