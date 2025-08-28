# Contributing

Thanks for your interest in improving the meta reflection benchmark. A few guidelines:

- Keep the scope minimal and focused on measuring RTTR vs EnTT::meta hot and cold paths.
- Prefer small, well-documented changes; update README/docs when behavior or usage changes.
- For CMake: follow modern CMake patterns and avoid global state where possible.
- For PRs: include a short summary of the change, rationale, and any measurement notes.

## Dev setup

- Windows + VS 2022 with C++ workload
- vcpkg installed (set VCPKG_ROOT)
- Python 3 (use the `py` launcher on Windows)

Use the VS Code tasks or run scripts under `scripts/` to configure, build, and run.