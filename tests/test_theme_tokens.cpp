// Unit tests for src/ThemeTokens.h -- snapshot invariants, no MFC.
//
// The snapshot's invariant checks mirror constraints CTheme's derivation
// enforces (FinalizeTokens / RefreshAccentDerived). Tests feed hand-constructed
// snapshots and assert the checks hold or fail, with expected polarities and
// ratios from WCAG reference figures, never from the implementation.
//
// BuildSnapshot itself lives in Theme.cpp (MFC + tinyxml2) and cannot compile
// in this MFC-free harness; these invariant tests are the maximum reachable
// coverage under this seam. Four-path builder coverage (defaults / file
// overrides / legacy fallback / accent refresh) defers to integration tests.
//
// Build & run (local, no MFC needed):
//   cl /nologo /std:c++17 /W4 /EHsc /I src tests\test_theme_tokens.cpp
//   .\test_theme_tokens.exe
// Delete the exe afterwards to keep the repo root clean.

#include "../src/ThemeTokens.h"

#include <cstdio>

static int g_failures = 0;

#define CHECK_TRUE(cond) do { \
	if (!(cond)) { \
		g_failures++; \
		std::printf("FAIL %s:%ld  %s\n", __FILE__, (long)__LINE__, #cond); \
	} \
} while (0)

static ThemeTokenSnapshot MakeBlankSnapshot()
{
	ThemeTokenSnapshot s{};
	s.surfaceBase = RGB(255, 255, 255);
	s.isDarkTheme = false;
	return s;
}

static void test_dark_flag_matches_surface()
{
	// WCAG: L(#FFFFFF) = 1.0 (light), L(#000000) = 0.0 (dark). The flag must
	// agree with the surface polarity.
	ThemeTokenSnapshot s = MakeBlankSnapshot();
	s.surfaceBase = RGB(255, 255, 255);
	s.isDarkTheme = false;
	CHECK_TRUE(SnapshotDarkFlagMatchesSurface(s));

	s.surfaceBase = RGB(0, 0, 0);
	s.isDarkTheme = true;
	CHECK_TRUE(SnapshotDarkFlagMatchesSurface(s));

	// A mid-grey surface (#808080, L ~= 0.216) is dark-side; flag must say so.
	s.surfaceBase = RGB(0x80, 0x80, 0x80);
	s.isDarkTheme = true;
	CHECK_TRUE(SnapshotDarkFlagMatchesSurface(s));

	// Mismatch must be detected (the check is not vacuously true).
	s.surfaceBase = RGB(255, 255, 255);
	s.isDarkTheme = true;
	CHECK_TRUE(!SnapshotDarkFlagMatchesSurface(s));
}

static void test_accent_text_clears_surface()
{
	// Black on white is 21:1, well past WCAG AA (4.5:1).
	ThemeTokenSnapshot s = MakeBlankSnapshot();
	s.accentText = RGB(0, 0, 0);
	s.surfaceBase = RGB(255, 255, 255);
	CHECK_TRUE(SnapshotAccentTextClearsSurface(s));

	// The Fluent Light accent #005FB8 on white is ~6.3:1 (from WCAG figures),
	// also past AA.
	s.accentText = RGB(0x00, 0x5F, 0xB8);
	s.surfaceBase = RGB(255, 255, 255);
	CHECK_TRUE(SnapshotAccentTextClearsSurface(s));

	// White on white is 1:1, far below AA -- the check must fail.
	s.accentText = RGB(255, 255, 255);
	s.surfaceBase = RGB(255, 255, 255);
	CHECK_TRUE(!SnapshotAccentTextClearsSurface(s));
}

static void test_selected_bg_separates()
{
	// kMinSeparation = 1.25:1 (RefreshAccentDerived, Theme.cpp:704). Black on
	// white/near-white surfaces separates by >= 15:1, well past the threshold.
	ThemeTokenSnapshot s = MakeBlankSnapshot();
	s.stateSelectedBG = RGB(0, 0, 0);
	s.surfaceBase = RGB(255, 255, 255);
	s.listBoxOddRowsBG = RGB(240, 240, 240);
	s.listBoxEvenRowsBG = RGB(245, 245, 245);
	CHECK_TRUE(SnapshotSelectedBGSeparates(s));

	// A selected background identical to the surface fails 1.25:1 (1:1).
	s.stateSelectedBG = RGB(255, 255, 255);
	s.surfaceBase = RGB(255, 255, 255);
	s.listBoxOddRowsBG = RGB(255, 255, 255);
	s.listBoxEvenRowsBG = RGB(255, 255, 255);
	CHECK_TRUE(!SnapshotSelectedBGSeparates(s));

	// Separates from the surface but not from one row: must still fail.
	s.stateSelectedBG = RGB(244, 244, 244);
	s.surfaceBase = RGB(255, 255, 255);
	s.listBoxOddRowsBG = RGB(244, 244, 244);	// ~1:1 against the pill
	s.listBoxEvenRowsBG = RGB(230, 230, 230);
	CHECK_TRUE(!SnapshotSelectedBGSeparates(s));
}

int main()
{
	test_dark_flag_matches_surface();
	test_accent_text_clears_surface();
	test_selected_bg_separates();

	if (g_failures > 0)
	{
		std::printf("%d failure(s)\n", g_failures);
		return 1;
	}
	std::printf("all theme token snapshot tests passed\n");
	return 0;
}
