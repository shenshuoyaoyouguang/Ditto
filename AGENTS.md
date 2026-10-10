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
- Debug build: `msbuild CP_Main_10.sln /p:Configuration=Debug /p:Platform=x64` → `Debug64\`.
- NuGet native packages: zlib + libpng (`packages.config`).

## Tests

- CI (`.github/workflows/pull_request.yml`) has two jobs: `tests` (compiles and runs unit tests) and `build` (Release x64).
- Unit tests live in `tests/` and are NOT part of any `.vcxproj`; they compile only MFC-free headers:
  ```
  cl /nologo /std:c++17 /W4 /EHsc /I src tests\test_theme_colors.cpp /Fe:test_theme_colors.exe
  .\test_theme_colors.exe
  ```
  MinGW alternative: `g++ -std=c++17 -Wall -I src tests/test_theme_colors.cpp -o dist/test_theme_colors.exe`
- Keep new unit-testable logic in a header that avoids MFC/CString (see `src/ThemeColorMath.h`) and add a matching `tests\test_*.cpp`. Expected values must come from an independent source of truth (e.g. WCAG reference figures), never from the implementation.
- No other automated coverage: verify app behavior by building and running `Release64\Ditto.exe`.

## Layout

- `src/` — all main-app sources, flat. `src/sqlite/` holds the sqlite3mc amalgamation (SQLite with encryption) plus the CppSQLite3 wrapper.
- `Shared/` — code compiled into multiple projects (`IClip.h`, `DittoDefines.h`, `TextConvert`, `Tokenizer`). Keep it dependency-light.
- `EncryptDecrypt/` — DLL that encrypts clip data (DB + network transfer). Link path depends on platform (`$(Configuration)64\EncryptDecrypt.lib` for x64).
- `Addins/DittoUtil/` — add-in DLL with exported utilities (`DittoUtil.def`); dialog-resizer helpers shared with the main app.
- `focusdll/`, `FocusHighlight/`, `ICU_Loader/` — helper DLLs (focus tracking, paste highlight, ICU unicode loader).
- `U3Stop/` — legacy, not in the solution.
- `tests/` — unit tests, intentionally excluded from the solution/MSBuild (run manually, see Tests).
- `DittoSetup/` — Inno Setup installer (`DittoSetup_10.iss`), portable ZIP script, choco/appx packaging.
- `Debug/Language/*.xml` (30 language files), `Debug/Themes/` — runtime resources: UI translations (loaded via `src/MultiLanguage.cpp`) and themes.

## Architecture notes

- App entry is `src/CP_Main.cpp`. The clipboard viewer chain lives in `ClipboardViewer.cpp`/`CopyThread.cpp`; clip data model in `Clip.cpp`; clip formats are aggregated by `CF_*Aggregator.cpp`.
- History DB is `Ditto.db` (SQLite); schema creation/migration in `src/DatabaseUtilities.cpp`.
- Network paste between peers: `src/Server.cpp`, `src/Client.cpp` (`ServerDefines.h`), friend UI in `Friend*`/`OptionFriends*` files.
- Scripting hooks: `DittoChaiScript.cpp`, `ChaiScriptOnCopy.cpp`, `ChaiScriptXml.cpp`.
- Main list UI: `QListCtrl*` files; settings dialogs: `Adv*`, `Option*` files.

## Settings

- All persistent settings go through `CGetSetOptions` (`src/Options.h/.cpp`): static `Get*`/`Set*` pairs layered on `GetProfileLong/GetProfileString/SetProfile*`, which pick an ini file or the registry at startup (`m_bFromIni`).
- Portable, Windows Store and Chocolatey builds always use the ini file (`Ditto.ini`), see `CGetSetOptions::GetIniFileName`.
- Gotcha: `GetProfileData`/`SetProfileData` assert in ini mode — do not add new binary settings.
- Adding a setting = new Get/Set pair + static cache member in `Options.h/.cpp` + a control in the matching `Option*.cpp`/`Adv*.cpp` dialog.

## Theming & custom-drawn UI

- `src/Theme.h/.cpp` (`CTheme`) loads themes from `Debug/Themes/`; colour-token derivation and the owner-drawn controls live in `src/ThemeColorMath.h` and the `src/Fluent*` files.
- `ThemeColorMath.h` is deliberately MFC/CString-free so `tests/` can compile it — it is the seam to use when changing colour or contrast logic.
- Accent, odd/even row backgrounds and selection highlights are derived from the accent colour via WCAG 4.5:1 contrast helpers; do not hard-code colours.

## Gotchas

- MSBuild does not glob: any new `.cpp/.h` must be added to `CP_Main.vcxproj` (and ideally `.vcxproj.filters`).
- `resource.h` + `CP_Main.rc` share one ID space — use the next free IDs, don't reorder existing ones.
- User-facing strings are translated via `Debug/Language/*.xml`, not only resource-table strings; check how `MultiLanguage.cpp` resolves a string before adding UI text.
- Keep changes Windows-only-compatible: this is a pure Win32/MFC codebase, no CRT-abstraction layers.
