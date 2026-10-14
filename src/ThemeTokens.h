#pragma once

// Read-only snapshot of CTheme's resolved tokens, compilable without MFC.
// The first slice of issue #60 swaps the four color-caching holders
// (scrollbar, caption, options nav) from scattered CTheme getters to a single
// Snapshot() read, so a theme switch repaints them from one resolved source.
//
// Carries the v4 semantic tokens plus the legacy fields the first-slice holders
// still paint with (scrollbar, three caption variants, nav uses SurfaceBase)
// and the two legacy row colours the selection-separation invariant needs.
// Pure v4-only consumers come later; these legacy fields drop once their holders
// move to v4 tokens.
//
// No MFC/CString: only Win32 COLORREF and std::string, so tests/test_theme_tokens.cpp
// can compile this header (and ThemeColorMath.h) outside the main project.

#include "ThemeColorMath.h"
#include <string>

struct ThemeTokenSnapshot {
	// 20 v4 semantic colours (mirror CTheme members, Theme.h:57-76)
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

	// Legacy surfaces the first-slice holders still paint with.
	// Scrollbar (CQPasteWnd::RefreshScrollBarColors).
	COLORREF scrollBarTrack, scrollBarThumb, scrollBarThumbHover;
	// Caption normal / top-most / not-connected variants (CWndEx::SetCaptionColorActive
	// picks one of three; carrying all three lets it migrate with no behaviour change).
	COLORREF captionLeft, captionRight, border, captionTextColor;
	COLORREF captionLeftTopMost, captionRightTopMost, borderTopMost;
	COLORREF captionLeftNotConnected, captionRightNotConnected, borderNotConnected;
	int captionSize, captionFontSize;
	// Legacy row colours the selection-pill separation invariant checks against
	// (RefreshAccentDerived walks State.SelectedBG away from these, not SurfaceRowAlt).
	COLORREF listBoxOddRowsBG, listBoxEvenRowsBG;
};

// MFC-free invariant checks for tests/test_theme_tokens.cpp. Each mirrors a
// constraint CTheme's derivation enforces (FinalizeTokens / RefreshAccentDerived);
// tests feed hand-constructed snapshots and assert these hold, with expected
// ratios from WCAG reference figures, never from the implementation.

// isDarkTheme must agree with the surface-base luminance polarity.
inline bool SnapshotDarkFlagMatchesSurface(const ThemeTokenSnapshot& s)
{
	return s.isDarkTheme == (RelLuminance(s.surfaceBase) < 0.5);
}

// Accent.Text must clear WCAG AA (4.5:1) on Surface.Base.
inline bool SnapshotAccentTextClearsSurface(const ThemeTokenSnapshot& s)
{
	return ContrastRatio(s.accentText, s.surfaceBase) >= 4.5;
}

// State.SelectedBG must separate >= 1.25:1 from the surface and both legacy row
// colours (kMinSeparation in RefreshAccentDerived, Theme.cpp:704).
inline bool SnapshotSelectedBGSeparates(const ThemeTokenSnapshot& s)
{
	const double kMinSeparation = 1.25;
	return ContrastRatio(s.stateSelectedBG, s.surfaceBase) >= kMinSeparation &&
		ContrastRatio(s.stateSelectedBG, s.listBoxOddRowsBG) >= kMinSeparation &&
		ContrastRatio(s.stateSelectedBG, s.listBoxEvenRowsBG) >= kMinSeparation;
}
