# Simple Injector — C++20 / Qt 6

A C++20 + Qt 6 Widgets desktop application ported from the supplied Python/PyQt6 project. The repository includes a reusable data utility module, automated unit tests, and a Windows GitHub Actions workflow.

## Features

- Add flag entries from pasted JSON or import a `.json` file.
- Search saved entries by name or value.
- Remove selected rows, copy selected entries as JSON, and export all entries.
- Persist flags and settings using atomic JSON writes.
- Detect whether `RobloxPlayerBeta.exe` is running.
- Fetch an offsets header asynchronously and cache parsed offsets locally.
- Optional Auto Apply when the Roblox process is detected.
- Report successful writes, missing offsets, unsupported values, and write failures separately.

## Build and test with GitHub Actions

The workflow is located at `.github/workflows/ci.yml`. It runs on pushes to `main` or `master`, pull requests, and manual dispatches. It:

1. Checks out the repository.
2. Installs Qt 6.8.3 and uses the Visual Studio 2022 x64 toolchain.
3. Configures and builds the app and unit tests with CMake.
4. Runs CTest and fails the job if any test fails.
5. Deploys the required Qt runtime DLLs with `windeployqt`.
6. Uploads `Simple-Injector-windows-x64` as a GitHub Actions artifact.

### Publish the project to GitHub

Upload the *contents* of this folder to the root of a GitHub repository (so that `CMakeLists.txt` and `.github/` are at the repository root), then push to `main` or `master`. Open the repository's **Actions** tab, select **Simple Injector - Windows Build and Tests**, and download the artifact from a successful run.

The portable ZIP does not include the Microsoft Visual C++ runtime. Install the official Microsoft Visual C++ 2015–2022 Redistributable (x64) if Windows reports missing MSVC runtime DLLs.

## Build locally (Windows)

Requirements:

- Visual Studio 2022 with the Desktop development with C++ workload.
- CMake 3.21 or newer.
- Qt 6.7 or newer with `Core`, `Widgets`, `Network`, and `Test` modules.

From a Developer PowerShell / terminal with Qt available to CMake:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/Qt/6.x.x/msvc2022_64" -DBUILD_TESTING=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

The executable is produced at `build/Release/SimpleInjector.exe`. Use a 64-bit Qt kit and a 64-bit compiler.

## Tests

The unit tests run without Roblox and do not access another process. They cover:

- Prefix normalization (`FFlag`, `DFInt`, `FString`, and related forms).
- JSON value conversion and unsupported JSON types.
- Boolean, decimal, hexadecimal, and signed 32-bit integer parsing.
- Parsing offsets from a C++ header-like text file.
- Atomic JSON write/read behavior and invalid-file fallbacks.

Process-memory behavior is not exercised by CI.

## Data files

The application stores its data under `%LOCALAPPDATA%/SimpleInjector/`:

- `ffs.json` — saved entries.
- `settings.json` — application settings.
- `offsets.json` — cached offsets.

On first launch, the app copies any matching data files from the earlier `%LOCALAPPDATA%/ez/` folder if the new files do not already exist.

## Important limitations

- The Windows process-memory feature uses the documented `WriteProcessMemory` API and supplied offsets. It does not bypass process permissions, Roblox protections, or anti-cheat controls.
- The code assumes an offset identifies a writable 32-bit value relative to the process's main module base. A successful operating-system write does not prove that the offset is current or that the value has the intended meaning.
- Only Boolean and signed 32-bit integer values are eligible for Apply. String-valued entries can be stored and exported, but their in-memory representation is not inferred.
- Roblox can change its memory layout. Stale or incorrect offsets can cause instability or unexpected behavior. Use only offsets you are authorized to use and have independently validated.
