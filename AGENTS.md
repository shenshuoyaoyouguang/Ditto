# AGENTS.md

Ditto — Windows clipboard manager (MFC C++ desktop app). Saves every clipboard entry to a local SQLite database, shows a searchable paste list (default hotkey Ctrl+`), supports sending clips over TCP to other machines ("friends"), scripting (ChaiScript), and add-ins.

## Build

- Solution: `CP_Main_10.sln` (VS 2026, v145 toolset, MFC dynamic, Unicode, C++17).
- CLI build (what CI does, see `.github/workflows/pull_request.yml`):
  ```
  msbuild CP_Main_10.sln /p:Configuration=Release /p:Platform=x64
  ```
  (vcpkg manifest mode auto-restores `vcpkg.json` deps on build; no `nuget restore` needed.)
- Configurations: Debug|Release × Win32|x64|ARM64. x64 output lands in `Release64\` / `Debug64\` at repo root (`Ditto.exe`, `ICU_Loader.dll`, `Addins\DittoUtil.dll`).
- Debug build: `msbuild CP_Main_10.sln /p:Configuration=Debug /p:Platform=x64` → `Debug64\`.
- Native deps via vcpkg manifest: zlib 1.3.x + tinyxml2 11.x (`vcpkg.json`, `VcpkgEnableManifest=true`).

## Tests

- CI (`.github/workflows/pull_request.yml`) has two jobs: `tests` (compiles and runs unit tests) and `build` (Release x64).
- Unit tests live in `tests/` and are NOT part of any `.vcxproj`; they compile only MFC-free headers (`src/ThemeColorMath.h`, `src/ListQueryBuilder.h`, `src/ClipRowFlags.h`, `src/ListLayoutMath.h`, `src/XmlFileLoad.h`):
  ```
  cl /nologo /std:c++17 /W4 /EHsc /I src tests\test_theme_colors.cpp /Fe:test_theme_colors.exe
  .\test_theme_colors.exe
  cl /nologo /std:c++17 /W4 /EHsc /I src tests\test_list_query_builder.cpp /Fe:test_list_query_builder.exe
  .\test_list_query_builder.exe
  cl /nologo /std:c++17 /W4 /EHsc /I src tests\test_clip_row_flags.cpp /Fe:test_clip_row_flags.exe
  .\test_clip_row_flags.exe
  cl /nologo /std:c++17 /W4 /EHsc /I src tests\test_list_layout_math.cpp /Fe:test_list_layout_math.exe
  .\test_list_layout_math.exe
  cl /nologo /std:c++17 /W4 /EHsc /I src tests\test_xml_file_load.cpp /Fe:test_xml_file_load.exe
  .\test_xml_file_load.exe
  ```
  `test_xml_parity` additionally needs the vcpkg restore first (`vcpkg install --triplet x64-windows`):
  ```
  cl /nologo /std:c++17 /W4 /EHsc /I src /I vcpkg_installed\x64-windows\include tests\test_xml_parity.cpp /Fe:test_xml_parity.exe /link vcpkg_installed\x64-windows\lib\tinyxml2.lib
  copy vcpkg_installed\x64-windows\bin\tinyxml2.dll .
  .\test_xml_parity.exe Debug\Language Debug\Themes <outdir>
  ```
  MinGW alternatives: `g++ -std=c++17 -Wall -Wextra -I src tests/test_theme_colors.cpp -o test_theme_colors.exe` (same pattern for the other three `tests/test_*.cpp`).
  Run each (`.\test_*.exe`), then delete the exes to keep the repo root clean (`*.exe` is gitignored).
- Keep new unit-testable logic in an MFC/CString-free header (see `src/ThemeColorMath.h`) first, then add a matching `tests\test_*.cpp`. Expected values must come from an independent source of truth (e.g. WCAG reference figures), never from the implementation.
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
- Network paste between peers: `src/Server.cpp`, `src/Client.cpp` (`ServerDefines.h`), friend UI in `Friend*`/`OptionFriends*` files. Transport is plaintext LAN: use on trusted networks only. File-transfer integrity is dual MD5 (V1 wire, `CSendInfoV1`) + SHA-256 (V2 `CSendInfo.m_sha256`, SHA-256 preferred with MD5 fallback for V1 peers).
- Scripting hooks: `DittoChaiScript.cpp`, `ChaiScriptOnCopy.cpp`, `ChaiScriptXml.cpp`.
- Main list UI: `QListCtrl*` files; settings dialogs: `Adv*`, `Option*` files.

## Settings

- All application options persist through `CGetSetOptions` (`src/Options.h/.cpp`): static `Get*`/`Set*` pairs layered on `GetProfileLong/GetProfileString/SetProfile*`, which pick an ini file or the registry at startup (`m_bFromIni`). Exception: `CProgressWnd::SaveCurrentSettings` persists its window position directly via `AfxGetApp()->WriteProfileInt` (`src/ProgressWnd.cpp:443-444`).
- Portable, Windows Store and Chocolatey builds always use the ini file (`Ditto.Settings`, not `Ditto.ini`), see `CGetSetOptions::GetIniFileName` (`src/Options.cpp:379-395`): Store/Chocolatey always resolve to the AppData directory (`GetIniFileName(false)`), portable resolves next to the exe (`GetIniFileName(true)`); see `LoadSettings` (`src/Options.cpp:108-154`) for the full fallback chain.
- Gotcha: `GetProfileData`/`SetProfileData` assert in ini mode — do not add new binary settings.
- Adding a setting = new Get/Set pair + static cache member in `Options.h/.cpp` + a control in the matching `Option*.cpp`/`Adv*.cpp` dialog.

## Theming & custom-drawn UI

- `src/Theme.h/.cpp` (`CTheme`) loads themes from `Debug/Themes/`; colour-token derivation and the owner-drawn controls live in `src/ThemeColorMath.h` and the `src/Fluent*` files.
- `ThemeColorMath.h` is deliberately MFC/CString-free so `tests/` can compile it — it is the seam to use when changing colour or contrast logic.
- Odd/even row backgrounds are plain theme values (`ListBoxOddRowsBG`/`ListBoxEvenRowsBG`), not accent-derived. The selection background starts from the accent-subtle mix and is walked toward the accent until it reaches 1.25:1 separation (`kMinSeparation`, `src/Theme.cpp:692`) from the base surface and both row colours; text-on-accent colours use WCAG contrast picks. Do not hard-code colours.

## Gotchas

- MSBuild does not glob: any new `.cpp/.h` must be added to `CP_Main.vcxproj` (and ideally `.vcxproj.filters`).
- `resource.h` + `CP_Main.rc` share one ID space — use the next free IDs, don't reorder existing ones.
- User-facing strings are translated via `Debug/Language/*.xml`, not only resource-table strings; check how `MultiLanguage.cpp` resolves a string before adding UI text.
- Keep changes Windows-only-compatible: this is a pure Win32/MFC codebase, no CRT-abstraction layers.
- `tests/` vs CI drift check: the six `cl` commands above must match the `tests` job in `.github/workflows/pull_request.yml` — update both together.
- `CSendInfo` wire compat: keep `CSendInfoV1` byte-identical to the old layout and V2 a pure tail grow; `RecieveSocket.cpp` derives V1/V2 encrypted sizes instead of hard-coding them.
