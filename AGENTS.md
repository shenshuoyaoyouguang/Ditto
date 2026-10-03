# AGENTS.md

Ditto — Windows clipboard manager (MFC C++ desktop app). Saves every clipboard entry to a local SQLite database, shows a searchable paste list (default hotkey Ctrl+`), supports sending clips over TCP to other machines ("friends"), scripting (ChaiScript), and add-ins.

## Build

- Solution: `CP_Main_10.sln` (VS 2022, v143 toolset, MFC dynamic, Unicode, C++17).
- CLI build (what CI does, see `.github/workflows/pull_request.yml`):
  ```
  nuget restore CP_Main_10.sln
  msbuild CP_Main_10.sln /p:Configuration=Release /p:Platform=x64
  ```
- Configurations: Debug|Release × Win32|x64|ARM64. x64 output lands in `Release64\` / `Debug64\` at repo root (`Ditto.exe`, `ICU_Loader.dll`, `Addins\DittoUtil.dll`).
- NuGet native packages: zlib + libpng (`packages.config`).
- No test suite in the repo; CI only builds Release x64. Verify changes by building and running `Release64\Ditto.exe`.

## Layout

- `src/` — all main-app sources, flat. `src/sqlite/` holds the sqlite3mc amalgamation (SQLite with encryption) plus the CppSQLite3 wrapper.
- `Shared/` — code compiled into multiple projects (`IClip.h`, `DittoDefines.h`, `TextConvert`, `Tokenizer`). Keep it dependency-light.
- `EncryptDecrypt/` — DLL that encrypts clip data (DB + network transfer). Link path depends on platform (`$(Configuration)64\EncryptDecrypt.lib` for x64).
- `Addins/DittoUtil/` — add-in DLL with exported utilities (`DittoUtil.def`); dialog-resizer helpers shared with the main app.
- `focusdll/`, `FocusHighlight/`, `ICU_Loader/` — helper DLLs (focus tracking, paste highlight, ICU unicode loader).
- `U3Stop/` — legacy, not in the solution.
- `DittoSetup/` — Inno Setup installer (`DittoSetup_10.iss`), portable ZIP script, choco/appx packaging.
- `Debug/Language/*.xml`, `Debug/Themes/` — runtime resources: UI translations (loaded via `src/MultiLanguage.cpp`) and themes.

## Architecture notes

- App entry is `src/CP_Main.cpp`. The clipboard viewer chain lives in `ClipboardViewer.cpp`/`CopyThread.cpp`; clip data model in `Clip.cpp`; clip formats are aggregated by `CF_*Aggregator.cpp`.
- History DB is `Ditto.db` (SQLite); schema creation/migration in `src/DatabaseUtilities.cpp`.
- Network paste between peers: `src/Server.cpp`, `src/Client.cpp` (`ServerDefines.h`), friend UI in `Friend*`/`OptionFriends*` files.
- Scripting hooks: `DittoChaiScript.cpp`, `ChaiScriptOnCopy.cpp`, `ChaiScriptXml.cpp`.
- Main list UI: `QListCtrl*` files; settings dialogs: `Adv*`, `Option*` files.

## Gotchas

- MSBuild does not glob: any new `.cpp/.h` must be added to `CP_Main.vcxproj` (and ideally `.vcxproj.filters`).
- `resource.h` + `CP_Main.rc` share one ID space — use the next free IDs, don't reorder existing ones.
- User-facing strings are translated via `Debug/Language/*.xml`, not only resource-table strings; check how `MultiLanguage.cpp` resolves a string before adding UI text.
- Keep changes Windows-only-compatible: this is a pure Win32/MFC codebase, no CRT-abstraction layers.
