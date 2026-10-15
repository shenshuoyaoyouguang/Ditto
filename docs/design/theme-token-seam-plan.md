# Theme Token Seam — Implementation Plan (issue #60, candidate c2)

Introduce one read-only theme-token snapshot behind a single seam. First slice
migrates the four color-caching holders that go stale on theme switch; the
remaining ~17 direct readers migrate as follow-up tickets behind the same seam.

## 1. Problem (current state)

Every view reads the concrete global theme directly:

- Global instance: `CTheme CGetSetOptions::m_Theme;` (`src/Options.cpp:73`).
- Holders cache colors once at create time and are not all refreshed on switch:
  - **Scrollbar** — `CQPasteWnd::RefreshScrollBarColors()` (`QPasteWnd.cpp:8387`)
    pushes `m_Theme.ScrollBarTrack/Thumb/ThumbHover()` into `m_modernScrollBar`.
    Refreshed on in-dialog theme change (`OptionsGeneral.cpp:720` calls
    `CQPasteWnd::RefreshThemeColors` directly), but **stale on system switch**:
    fan-out's `IsKindOf(CWndEx)` matches first and `RefreshThemeColors` is
    non-virtual, so `CWndEx::RefreshThemeColors` (caption only) runs instead.
  - **Window caption** — `CWndEx::SetCaptionColorActive()` (`WndEx.cpp:128`)
    pushes caption tokens into `m_DittoWindow`; `WndEx.cpp:171` already notes
    "theme change left already open windows on the old palette".
    `CEditFrameWnd::RefreshThemeColors()` (`EditFrameWnd.cpp:176`) does the same.
  - **Options nav** — `COptionsSheet` creates `m_navBg` once
    (`OptionsSheet.cpp:166`, `m_Theme.SurfaceBase()`) and returns the cached
    brush in `OnCtlColor` (`:228`). **No `RefreshThemeColors` exists**, and
    neither switch path reaches it: system-switch fan-out has no `COptionsSheet`
    branch, and in-dialog change (`OptionsGeneral.cpp:720`) only refreshes the
    paste window. Open options dialog keeps the old nav background on both paths.
- Push mechanism that already fans out: `EnumThreadWindows` +
  `RefreshCaptionColorsProc` (`MainFrm.cpp:101/925/1587`) calls each window's
  `RefreshThemeColors()`. Reuse it; add no new subscription.
- Producer mount points: `CTheme::FinalizeTokens()` (`Theme.cpp:569`, called at
  end of every `Load`) and `CTheme::RefreshSystemAccent()` (`Theme.cpp:667`).
  `RefreshSystemAccent` also fires on a `Load` short-circuit path (`Theme.cpp:125`,
  file unchanged but `m_bFollowSystemAccent`) that bypasses `FinalizeTokens`, so
  it needs its own mount (covered below).
- Existing MFC-free math seam: `src/ThemeColorMath.h` (pure functions). Test
  prior art: `tests/test_theme_colors.cpp`.

## 2. Design

### 2.1 Snapshot (new, MFC-free)

`src/ThemeTokens.h` — a POD snapshot of the resolved tokens, compilable by the
test harness without MFC:

```cpp
#pragma once
#include <windows.h>
#include <string>

struct ThemeTokenSnapshot {
    // 20 semantic colors (mirror CTheme v4 members, Theme.h:57-76)
    COLORREF surfaceBase, surfaceElevated, surfaceRowAlt;
    COLORREF textPrimary, textSecondary, textDisabled, textOnAccent;
    COLORREF accentDefault, accentSubtle, accentText;
    COLORREF strokeCard, strokeDivider;
    COLORREF controlFill, controlHover, controlPressed, controlDisabledBG;
    COLORREF stateHover, stateSelectedBG, stateSelectedText, indicatorBadge;
    // control sizes + font + dark flag
    int radiusControl, rowHeightCompact, rowHeightComfortable;
    std::string fontFamily;
    bool isDarkTheme;
    // legacy surfaces the first-slice holders still paint with (scrollbar,
    // caption, nav). Carried so holders migrate without touching CTheme's
    // legacy accessors; dropped once those holders move to v4 tokens.
    COLORREF scrollBarTrack, scrollBarThumb, scrollBarThumbHover;
    COLORREF captionLeft, captionRight, border, captionTextColor;
    int captionSize, captionFontSize;
};
```

Rationale: carry the legacy fields the first slice actually reads, so migration
is a read-site swap with no behavior change. Pure v4-only consumers come later.

### 2.2 Producer (in CTheme)

Add to `CTheme` (`Theme.h`):

```cpp
public:
    const ThemeTokenSnapshot& Snapshot() const { return m_snapshot; }
private:
    void BuildSnapshot();
    ThemeTokenSnapshot m_snapshot{};
```

`BuildSnapshot()` in `Theme.cpp` copies the resolved members + converts
`m_csFontFamily` (CString) to `std::string`. Call it at the tail of
`FinalizeTokens()` and `RefreshSystemAccent()` (after `RefreshAccentDerived()`),
so the snapshot always reflects the last resolution.

### 2.3 Distribution (existing push, unchanged plumbing)

`RefreshCaptionColorsProc` fans out to every window; first-slice holders it
doesn't yet dispatch (scrollbar via `CQPasteWnd`, nav via `COptionsSheet`) get
new `IsKindOf` branches in steps 3/5. Each first-slice `RefreshThemeColors()`
reads `CGetSetOptions::m_Theme.Snapshot()` instead of the scattered token
getters. Per-paint readers stay per-paint (no behavior change).

## 3. Implementation steps (first slice)

| # | Change | Files | Verify |
|---|---|---|---|
| 1 | Add `src/ThemeTokens.h` (POD snapshot, MFC-free) | new | compiles standalone |
| 2 | Add `BuildSnapshot()` + `m_snapshot` + `Snapshot()` to CTheme; call from `FinalizeTokens` tail and `RefreshSystemAccent` tail | `Theme.h`, `Theme.cpp` | app builds; snapshot matches resolved values |
| 3 | Scrollbar holder reads snapshot: `RefreshScrollBarColors()` uses `Snapshot().scrollBarTrack/Thumb/ThumbHover`. **Also fix the system-switch trigger**: add a `CQPasteWnd` branch to `RefreshCaptionColorsProc` (`MainFrm.cpp:111`) **before** the `CWndEx` branch (IsKindOf matches both; non-virtual `RefreshThemeColors` otherwise runs the base) so fan-out calls `CQPasteWnd::RefreshThemeColors`→`RefreshScrollBarColors` | `QPasteWnd.cpp:8387`, `MainFrm.cpp:111` | existing tests pass; scrollbar follows system switch |
| 4 | Caption holders read snapshot: `CWndEx::SetCaptionColorActive` + `CEditFrameWnd::RefreshThemeColors` use `Snapshot().captionLeft/...` | `WndEx.cpp:128`, `EditFrameWnd.cpp:176` | open windows repaint on switch |
| 5 | Options nav holder: add `COptionsSheet::RefreshThemeColors()` that rebuilds `m_navBg` from `Snapshot().surfaceBase` and invalidates `m_nav`. **Two trigger wires** (both currently missing): (a) add a `COptionsSheet` branch to `RefreshCaptionColorsProc` (`MainFrm.cpp:111`) for the system-switch path; (b) in `COptionsGeneral::OnCbnSelchangeComboTheme` (`OptionsGeneral.cpp:720`) call the owning sheet's `RefreshThemeColors` for the in-dialog path. Confirm `COptionsSheet` has `IMPLEMENT_DYNAMIC` for `RUNTIME_CLASS` | `OptionsSheet.cpp/.h`, `MainFrm.cpp:111`, `OptionsGeneral.cpp:720` | open options dialog nav repaints on both switch paths |
| 6 | Register `ThemeTokens.h` in `CP_Main.vcxproj` + `.filters` (header only; no .cpp) | vcxproj, filters | `scripts\check-vcxproj-consistency.ps1` passes |
| 7 | Add `tests/test_theme_tokens.cpp`: assert snapshot *invariants* via MFC-free check functions in `ThemeTokens.h` (e.g. `AssertSnapshotContrast(const ThemeTokenSnapshot&)`): dark flag matches surface polarity, accentText clears 4.5:1 on surfaceBase, stateSelectedBG separates ≥1.25:1 from surfaceBase + both row colors. Tests feed **hand-constructed** snapshot values; expected ratios from WCAG reference figures. **Gap vs issue**: issue asks for builder-path coverage (defaults/file overrides/legacy fallback/accent refresh), but `BuildSnapshot` lives in `Theme.cpp` (MFC + tinyxml2) and cannot compile in the MFC-free test harness — invariant tests are the maximum reachable coverage under this seam; four-path coverage defers to integration tests or a future thinner builder seam | new test | compiles + passes standalone |
| 8 | Register the new test in `.github/workflows/pull_request.yml` (build + run stanza matching the existing six) | workflow | CI green |

Editor brush note: `CEditFrameWnd::RefreshThemeColors()` (step 4) covers the
editor frame caption. If "editor brush" in the issue also means the in-content
background brush of `DittoRulerRichEditCtrl`, locate and migrate it as a sub-step
of 4; otherwise follow-up ticket.

## 4. Out of scope (issue-stated)

- Migrating the remaining ~17 direct readers beyond the first slice.
- Deduplicating parallel color/path helpers, dialog-shell classes, settings
  facade, XML/script/net/db work.

## 5. Manual gate (before close)

Build `Release64\Ditto.exe`, then exercise a light/dark/system-accent switch
across: paste list, paste window, scrollbars, options dialog (open during
switch). Acceptance: no stale surfaces, selection/hover text readable.

## 6. Risk / rollback

- Additive only: CTheme and all legacy accessors retained; old call sites keep
  working. A bad snapshot field surfaces as one wrong color in one holder —
  revert the holder's read line, no flag-day cutover.
- Theme `Load` and snapshot reads/writes all run on the UI thread (OnTimer /
  control events); no cross-thread access.
