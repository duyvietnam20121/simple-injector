# Simple Injector v1.1 — C++20 / Qt 6

A C++20 + Qt 6 Widgets desktop app (Windows) that keeps a list of Roblox FastFlags and writes
their boolean / 32-bit integer values into the running `RobloxPlayerBeta.exe`, then tells you
which flags were applied and which were not.

## Features

- Add flags from pasted JSON or from a `.json` file (the app reports how many entries were skipped).
- A table built on `QAbstractTableModel` + `QSortFilterProxyModel`: sort by any column, filter by
  name / value / status, **edit values in place** (double-click or `F2`), delete with `Delete`,
  copy with `Ctrl+C`, export to a file.
- **Per-flag STATUS column** with three easy states:
  `— Pending` (not applied yet, or the value was edited since), `✓ Applied`, `✗ Failed`.
  There is also an "Only show failed flags" filter, and sorting by STATUS puts failures first.
  Editing a flag resets its status to pending; losing the Roblox connection or attaching to a new
  process clears all statuses.
- **Read-back verification**: after every write the value is read back with `ReadProcessMemory`.
  A flag counts as applied only if the read-back matches; otherwise it is reported as failed
  (a typical sign of a stale offset or of the game overwriting the value).
- **Version check**: the running Roblox version (`version-<hex>`, taken from the executable's path) is
  compared with the version the offsets table was generated for (the `ClientVersion` line of the
  header). On a mismatch the app warns, asks for confirmation before a manual Apply, and postpones
  Auto Apply until newer offsets have been downloaded.
- **Offsets provenance**: the offsets line shows the entry count, the version, and whether the data
  came from the network (with the time) or from the on-disk cache; a failed download is shown
  persistently. A **Refresh** button downloads the table again; it also refreshes every 15 minutes
  and right after attaching to Roblox when the versions differ.
- Cheap Roblox detection (a process handle check while attached, a process-list scan otherwise) and
  Auto Apply exactly once per newly attached process.
- Flags and settings are saved atomically (`QSaveFile`).
- The window size follows the screen (it never overflows small or highly scaled displays) and can be
  resized with the grip in the bottom-right corner.

## Source layout

```
src/
  main.cpp
  core/                  Logic with no dependency on Widgets or Windows; unit-tested
    io/                    json_io (parse/read/write JSON atomically), app_storage (data folder)
    flags/                 flag_name (prefix handling), flag_value (value parsing),
                           flag_store, flag_table_model, flag_filter_proxy_model
    offsets/               offset_map, offset_lookup, offsets_header_parser, offsets_cache,
                           roblox_version
    apply/                 process_memory (IProcessMemory), flag_status, flag_applier
  platform/              The Roblox process (Windows APIs)
    process_probe            IProcessProbe + ProcessStatus (so tests can use a fake probe)
    roblox_process           finds/opens the process, reads and writes memory
    process_monitor          polls a probe and emits status changes
  services/
    offsets_service          downloads (manual / periodic) + cache + version of the offsets
  ui/                    Widgets: main_window, add_flag_dialog, title_bar, style, window_sizing
resources/               icon (app.ico, app.png), app.qrc, app.rc.in (icon + version info for the .exe)
tests/                   One test executable per module; none needs Roblox
```

Dependency rule: `ui` → `services` / `platform` / `core`; `core` knows nothing about the UI or
Windows. Process memory goes through `IProcessMemory` and the process through `IProcessProbe`, so the
tests use fakes. `OffsetsService` is tested against a local `QTcpServer` acting as the download
source (200, 404, empty body, oversized body, hung server, concurrent refreshes, periodic refresh).

## Building locally (Windows)

Requirements: Visual Studio 2022 (Desktop development with C++), CMake ≥ 3.21, Qt ≥ 6.7 with the
`Core`, `Gui`, `Widgets`, `Network` and `Test` modules.

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/Qt/6.x.x/msvc2022_64" -DBUILD_TESTING=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

The executable is `build/Release/SimpleInjector.exe`. For a trial build you can lower the Qt
requirement with `-DSI_QT_MIN_VERSION=6.4` (the code has been built and tested with Qt 6.4 on
Linux; the Windows-only parts compile on Windows only).

CMake lists every missing source file at once, and refuses to configure if a key file still has
outdated content (for example a file pasted under the wrong name).

## CI

`.github/workflows/ci.yml` runs on push / pull request: it installs Qt 6.8.3, builds, runs CTest,
runs `windeployqt`, and uploads the `Simple-Injector-windows-x64` folder as an artifact. The portable
folder does not include the Microsoft Visual C++ Redistributable (x64); install it if Windows
complains about missing MSVC DLLs.

## Version and icon

- The version is declared in one place: `project(SimpleInjector VERSION 1.1.0 ...)` in
  `CMakeLists.txt`. From it CMake generates the `v1.1` shown in the title bar and the window title,
  and the version info embedded in the `.exe` (Properties > Details). To release a new version, edit
  that line.
- The icon lives in `resources/icons/`: `app.ico` (sizes 16–256 px, attached to the `.exe` through
  `resources/app.rc.in`) and `app.png` (embedded with `resources/app.qrc`, used by the title bar and the
  window icon). To change the icon, replace those two files (keep the names) and rebuild.

## Data

Stored in `%LOCALAPPDATA%/SimpleInjector/`: `ffs.json` (flags), `settings.json`, and `offsets.json`
(the offsets cache, with the Roblox version under a `__meta` key). On first run, files with the same
names in `%LOCALAPPDATA%/ez/` are copied over if they do not exist yet.

## Important limits

- Only documented `ReadProcessMemory` / `WriteProcessMemory` calls are used, with the current user's
  own rights; the app does not bypass any process protection or anti-cheat of Roblox.
- An offset is interpreted as a writable 32-bit value relative to the base of the main module. A
  matching read-back only proves that the memory cell took the value, not that it is the flag you
  meant; conversely, a mismatch can also be the game overwriting the value right after the write.
- The version check relies on the `version-<hex>` folder name of the executable and on the
  `ClientVersion` line of the offsets header. If either is missing the result is "cannot compare
  yet" and nothing is blocked.
- Only booleans and signed 32-bit integers can be applied; string flags are stored and exported only.
- A wrong or stale offset can destabilize Roblox. Use only offsets you are allowed to use and have
  verified yourself.
