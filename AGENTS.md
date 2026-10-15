# AGENTS.md

Ditto — Windows clipboard manager (MFC C++ desktop app, Unicode C++17). Every clipboard entry lands in local SQLite (`Ditto.db`), searchable paste list on Ctrl+`, TCP paste to peers ("friends"), ChaiScript hooks, add-ins.

## Build & verify

- Build (what CI runs): `msbuild CP_Main_10.sln /p:Configuration=Release /p:Platform=x64` → `Release64\Ditto.exe`. Debug → `Debug64\`. Deps restore via vcpkg manifest (`vcpkg.json` is the version source).
- Tests (source of truth: `tests` job in `.github/workflows/pull_request.yml`): `cl /nologo /std:c++17 /W4 /EHsc /I src tests\test_<name>.cpp` per file in `tests/`, run each exe, then delete exes (`*.exe` is gitignored). `test_xml_parity` additionally links `tinyxml2` after `vcpkg install --triplet x64-windows` and runs against `Debug\Language Debug\Themes`.
- No other automated coverage: verify app behaviour by running `Release64\Ditto.exe`.

## Where new code goes

- Testable logic first in an MFC/CString-free header (seam: `src/ThemeColorMath.h`) + `tests\test_*.cpp` with expected values from an independent reference (e.g. WCAG figures), never from the implementation.
- MSBuild does not glob: register every new `.cpp/.h` in `CP_Main.vcxproj` (+ `.vcxproj.filters`), then run `scripts\check-vcxproj-consistency.ps1`.
- `Shared\` compiles into multiple projects: keep it dependency-light.
- User-facing text resolves via `src/MultiLanguage.cpp` + `Debug/Language/*.xml`: add the translation entry alongside the resource string. Resource IDs share one space (`resource.h` + `CP_Main.rc`): take the next free ID, verified by `scripts\check-resource-ids.ps1`.
- Target Win32/MFC only, no CRT-abstraction layers.

## Subsystem map

- Clipboard chain: `src/CP_Main.cpp` entry, `ClipboardViewer.cpp`/`CopyThread.cpp` capture, `Clip.cpp` model, `CF_*Aggregator.cpp` per-format aggregation.
- History DB: schema creation/migration lives in `src/DatabaseUtilities.cpp`.
- Network paste (plaintext LAN, trusted networks only): `src/Server.cpp`, `src/Client.cpp` (`ServerDefines.h`), friend UI in `Friend*`/`OptionFriends*`. Wire rule: grow `CSendInfo` by tail only, keep `CSendInfoV1` byte-identical; sizes derive in `RecieveSocket.cpp`, never hard-coded. Integrity is MD5 (V1 `CSendInfoV1`) + SHA-256 (V2 `CSendInfo.m_sha256`), SHA-256 preferred with MD5 fallback.
- Scripting: `DittoChaiScript.cpp`, `ChaiScriptOnCopy.cpp`, `ChaiScriptXml.cpp`.
- List UI: `QListCtrl*` files; settings dialogs: `Adv*`, `Option*` files.
- Settings: extend `CGetSetOptions` (`src/Options.h/.cpp`) with a `Get*`/`Set*` pair + static cache + control in the matching `Option*.cpp`/`Adv*.cpp` dialog. Persist via `GetProfileLong/String`/`SetProfile*` only (`GetProfileData` asserts in ini mode). Ini filename/fallback chain: `GetIniFileName` / `LoadSettings` in `src/Options.cpp`.
- Theming: `src/Theme.h/.cpp` loads `Debug/Themes/`; derive tokens via the `ThemeColorMath.h` seam. Odd/even rows stay plain theme values; selection walks from the accent-subtle mix toward accent until 1.25:1 separation (`kMinSeparation`, `src/Theme.cpp:692`) from surface and both row colours; text-on-accent picks by WCAG contrast.

## Agent skills

### Issue tracker

GitHub Issues in the `shenshuoyaoyouguang/Ditto` fork. See `docs/agents/issue-tracker.md`.

### Triage labels

Five canonical defaults. See `docs/agents/triage-labels.md`.

### Domain docs

Single-context. See `docs/agents/domain.md`.
