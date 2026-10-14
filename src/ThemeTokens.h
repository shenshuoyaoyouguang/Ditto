#pragma once

// Read-only theme-token snapshot producer (ticket T1, spec issue #60).
//
// The theme module resolves all defaults, file values, legacy derivation and
// contrast walks once, then hands every view the same finished values. This
// header is the MFC/CString-free seam for that producer: it compiles
// standalone (cl /nologo /std:c++17 /W4 /EHsc /I src) so that
// tests/test_theme_tokens.cpp can cover it without the application.
//
// steadfast rule: presence flags, legacy fallbacks and the minimum-separation
// walk resolve INSIDE BuildThemeSnapshot(); consumers only ever see the
// finished ThemeTokenSnapshot values. The audit stays diagnostic-only output
// in CTheme and is not part of the snapshot.

#include <windows.h>

#include <string>

#include "ThemeColorMath.h"

namespace ditto
{
namespace theme
{

// Finished values only: twenty semantic colors plus control sizes, the font
// family and the dark flag. No m_has* flags, no legacy colors, no walk state.
struct ThemeTokenSnapshot
{
	COLORREF surfaceBase = RGB(0, 0, 0);
	COLORREF surfaceElevated = RGB(0, 0, 0);
	COLORREF surfaceRowAlt = RGB(0, 0, 0);
	COLORREF textPrimary = RGB(0, 0, 0);
	COLORREF textSecondary = RGB(0, 0, 0);
	COLORREF textDisabled = RGB(0, 0, 0);
	COLORREF textOnAccent = RGB(0, 0, 0);
	COLORREF accentDefault = RGB(0, 0, 0);
	COLORREF accentSubtle = RGB(0, 0, 0);
	COLORREF accentText = RGB(0, 0, 0);
	COLORREF strokeCard = RGB(0, 0, 0);
	COLORREF strokeDivider = RGB(0, 0, 0);
	COLORREF controlFill = RGB(0, 0, 0);
	COLORREF controlHover = RGB(0, 0, 0);
	COLORREF controlPressed = RGB(0, 0, 0);
	COLORREF controlDisabledBG = RGB(0, 0, 0);
	COLORREF stateHover = RGB(0, 0, 0);
	COLORREF stateSelectedBG = RGB(0, 0, 0);
	COLORREF stateSelectedText = RGB(0, 0, 0);
	COLORREF indicatorBadge = RGB(0, 0, 0);
	int radiusControl = 0;
	int rowHeightCompact = 0;
	int rowHeightComfortable = 0;
	std::wstring fontFamily;
	bool isDarkTheme = false;
};

struct ColorOverride
{
	bool has = false;
	COLORREF value = RGB(0, 0, 0);
};

struct IntOverride
{
	bool has = false;
	int value = 0;
};

struct FontOverride
{
	bool has = false;
	std::wstring value;
};

// Everything the producer needs, in plain data: the OS polarity, whether a
// theme xml was parsed, the built-in accent, the row surfaces the selection
// pill separates from, explicit per-token overrides and legacy v3 fallbacks.
struct ThemeSnapshotRequest
{
	bool osDark = false;
	bool parsedXml = false;
	// ClipPastedColor equivalent: the built-in palette accent. Used when no
	// theme xml was parsed (mirrors FinalizeTokens falling back to the member
	// that LoadWindowsAccentColor may have pointed at the system accent).
	COLORREF builtInAccent = RGB(0x00, 0x5F, 0xB8);
	// Row surfaces the selection pill must separate from. Default to the
	// LoadDefaults legacy pair (odd white, even #F3F3F3); legacy themes pass
	// their loaded ListBoxOdd/EvenRowsBG here.
	COLORREF rowOddBG = RGB(255, 255, 255);
	COLORREF rowEvenBG = RGB(243, 243, 243);

	// Explicit per-token overrides (LoadTokensFromXml node wins). Resolved in
	// slice 2; present here so tests can name the full interface up front.
	ColorOverride surfaceBase;
	ColorOverride surfaceElevated;
	ColorOverride surfaceRowAlt;
	ColorOverride textPrimary;
	ColorOverride textSecondary;
	ColorOverride textDisabled;
	ColorOverride textOnAccent;
	ColorOverride accentDefault;
	ColorOverride accentSubtle;
	ColorOverride accentText;
	ColorOverride strokeCard;
	ColorOverride strokeDivider;
	ColorOverride controlFill;
	ColorOverride controlHover;
	ColorOverride controlPressed;
	ColorOverride controlDisabledBG;
	ColorOverride stateHover;
	ColorOverride stateSelectedBG;
	ColorOverride stateSelectedText;
	ColorOverride indicatorBadge;
	IntOverride radiusControl;
	IntOverride rowHeightCompact;
	IntOverride rowHeightComfortable;
	FontOverride fontFamily;

	// Legacy v3 fallbacks (DeriveTokensFromLegacy inputs). Resolved in slice 2.
	bool hasLegacyMainWindowBG = false;
	COLORREF legacyMainWindowBG = RGB(0, 0, 0);
	bool hasLegacySearchFocusBG = false;
	COLORREF legacySearchFocusBG = RGB(0, 0, 0);
	bool hasLegacyDescriptionBG = false;
	COLORREF legacyDescriptionBG = RGB(0, 0, 0);
	bool hasLegacyEvenRowsBG = false;
	bool hasLegacyOddRowsText = false;
	COLORREF legacyOddRowsText = RGB(0, 0, 0);
	bool hasLegacyClipPasted = false;
	COLORREF legacyClipPasted = RGB(0, 0, 0);
	bool hasLegacyQuickPasteIndex = false;
	COLORREF legacyQuickPasteIndex = RGB(0, 0, 0);
};

// Slice 1: default palettes plus the always-on accent refresh. Mirrors
// CTheme::LoadTokenDefaults (polarity palettes, hover/subtle blends, sizes,
// "auto" font) and CTheme::RefreshAccentDerived (hover/subtle recompute, the
// kMinSeparation walk of the selection pill, text-on-accent pick and the
// 4.5:1 accent-text guarantee).
// Slice 2: explicit per-token overrides win over the palette, and the legacy
// v3 fallbacks fill whatever is still missing when a theme xml was parsed
// (mirrors CTheme::LoadTokensFromXml precedence plus DeriveTokensFromLegacy).
inline ThemeTokenSnapshot BuildThemeSnapshot(const ThemeSnapshotRequest& req)
{
	ThemeTokenSnapshot snap;

	if (req.osDark)
	{
		snap.surfaceBase = RGB(0x20, 0x20, 0x20);
		snap.surfaceElevated = RGB(0x2B, 0x2B, 0x2B);
		snap.surfaceRowAlt = RGB(0x26, 0x26, 0x26);
		snap.textPrimary = RGB(0xFF, 0xFF, 0xFF);
		snap.textSecondary = RGB(0xC8, 0xC8, 0xC8);
		snap.textDisabled = RGB(0x71, 0x71, 0x71);
		snap.accentDefault = RGB(0x4C, 0xC2, 0xFF);
		snap.strokeCard = RGB(0x3D, 0x3D, 0x3D);
		snap.strokeDivider = RGB(0x33, 0x33, 0x33);
		snap.controlFill = RGB(0x2D, 0x2D, 0x2D);
		snap.controlHover = RGB(0x38, 0x38, 0x38);
		snap.controlPressed = RGB(0x3A, 0x3A, 0x3A);
		snap.controlDisabledBG = RGB(0x29, 0x29, 0x29);
		snap.stateSelectedText = RGB(0xFF, 0xFF, 0xFF);
		snap.indicatorBadge = RGB(0xC8, 0xC8, 0xC8);
	}
	else
	{
		snap.surfaceBase = RGB(0xF3, 0xF3, 0xF3);
		snap.surfaceElevated = RGB(0xFF, 0xFF, 0xFF);
		snap.surfaceRowAlt = RGB(0xF9, 0xF9, 0xF9);
		snap.textPrimary = RGB(0x1A, 0x1A, 0x1A);
		snap.textSecondary = RGB(0x61, 0x61, 0x61);
		snap.textDisabled = RGB(0x9D, 0x9D, 0x9D);
		snap.accentDefault = RGB(0x00, 0x5F, 0xB8);
		snap.strokeCard = RGB(0xE5, 0xE5, 0xE5);
		snap.strokeDivider = RGB(0xED, 0xED, 0xED);
		snap.controlFill = RGB(0xFB, 0xFB, 0xFB);
		snap.controlHover = RGB(0xF0, 0xF0, 0xF0);
		snap.controlPressed = RGB(0xED, 0xED, 0xED);
		snap.controlDisabledBG = RGB(0xF5, 0xF5, 0xF5);
		snap.stateSelectedText = RGB(0x1A, 0x1A, 0x1A);
		snap.indicatorBadge = RGB(0x61, 0x61, 0x61);
	}

	// Explicit file values win over the palette (mirrors LoadTokensFromXml).
	if (req.surfaceBase.has)
		snap.surfaceBase = req.surfaceBase.value;
	if (req.surfaceElevated.has)
		snap.surfaceElevated = req.surfaceElevated.value;
	if (req.surfaceRowAlt.has)
		snap.surfaceRowAlt = req.surfaceRowAlt.value;
	if (req.textPrimary.has)
		snap.textPrimary = req.textPrimary.value;
	if (req.textSecondary.has)
		snap.textSecondary = req.textSecondary.value;
	if (req.textDisabled.has)
		snap.textDisabled = req.textDisabled.value;
	if (req.strokeCard.has)
		snap.strokeCard = req.strokeCard.value;
	if (req.strokeDivider.has)
		snap.strokeDivider = req.strokeDivider.value;
	if (req.controlFill.has)
		snap.controlFill = req.controlFill.value;
	if (req.controlHover.has)
		snap.controlHover = req.controlHover.value;
	if (req.controlPressed.has)
		snap.controlPressed = req.controlPressed.value;
	if (req.controlDisabledBG.has)
		snap.controlDisabledBG = req.controlDisabledBG.value;
	if (req.stateSelectedText.has)
		snap.stateSelectedText = req.stateSelectedText.value;
	if (req.indicatorBadge.has)
		snap.indicatorBadge = req.indicatorBadge.value;
	if (req.accentDefault.has)
		snap.accentDefault = req.accentDefault.value;

	if (req.parsedXml)
	{
		// Legacy derivation fills whatever the file left out. Polarity is
		// judged from the theme's own base, not the OS setting, so a dark
		// theme on a light machine still derives the dark branch.
		if (!req.surfaceBase.has && req.hasLegacyMainWindowBG)
			snap.surfaceBase = req.legacyMainWindowBG;

		bool dark = RelLuminance(snap.surfaceBase) < 0.5;

		if (!req.surfaceElevated.has)
		{
			if (req.hasLegacySearchFocusBG)
				snap.surfaceElevated = req.legacySearchFocusBG;
			else if (req.hasLegacyDescriptionBG)
				snap.surfaceElevated = req.legacyDescriptionBG;
			else
				snap.surfaceElevated = ShiftToward(snap.surfaceBase, !dark, 0.04);
		}

		if (!req.surfaceRowAlt.has)
			snap.surfaceRowAlt = req.hasLegacyEvenRowsBG ? req.rowEvenBG
				: ShiftToward(snap.surfaceBase, !dark, 0.01);

		if (!req.textPrimary.has)
			snap.textPrimary = req.hasLegacyOddRowsText ? req.legacyOddRowsText
				: (dark ? RGB(255, 255, 255) : RGB(0, 0, 0));

		if (!req.textSecondary.has)
			snap.textSecondary = BlendOver(snap.surfaceBase, snap.textPrimary, 158);

		if (!req.textDisabled.has)
			snap.textDisabled = BlendOver(snap.surfaceBase, snap.textSecondary, 153);

		if (!req.strokeCard.has)
		{
			snap.strokeCard = RGB((GetRValue(snap.surfaceBase) + GetRValue(snap.surfaceElevated)) / 2,
				(GetGValue(snap.surfaceBase) + GetGValue(snap.surfaceElevated)) / 2,
				(GetBValue(snap.surfaceBase) + GetBValue(snap.surfaceElevated)) / 2);
		}

		if (!req.strokeDivider.has)
			snap.strokeDivider = BlendOver(snap.strokeCard, snap.surfaceBase, 64);

		if (!req.controlFill.has)
			snap.controlFill = snap.surfaceElevated;

		if (!req.controlHover.has)
			snap.controlHover = ShiftToward(snap.surfaceBase, !dark, 0.04);

		if (!req.controlPressed.has)
			snap.controlPressed = ShiftToward(snap.controlHover, !dark, 0.02);

		if (!req.controlDisabledBG.has)
			snap.controlDisabledBG = ShiftToward(snap.surfaceBase, !dark, 0.015);

		if (!req.indicatorBadge.has)
			snap.indicatorBadge = req.hasLegacyQuickPasteIndex ? req.legacyQuickPasteIndex : snap.textSecondary;

		if (!req.stateSelectedText.has)
			snap.stateSelectedText = snap.textPrimary;

		if (!req.accentDefault.has)
		{
			if (req.hasLegacyClipPasted)
				snap.accentDefault = req.legacyClipPasted;
			else
				snap.accentDefault = dark ? RGB(0x4C, 0xC2, 0xFF) : RGB(0x00, 0x5F, 0xB8);
		}
	}
	else if (!req.accentDefault.has)
	{
		// Built-in palette path: no xml parsed, so the accent comes from the
		// ClipPastedColor equivalent (system accent when following Windows).
		snap.accentDefault = req.builtInAccent;
	}

	snap.isDarkTheme = RelLuminance(snap.surfaceBase) < 0.5;

	// ---- accent refresh (always runs, mirrors RefreshAccentDerived) ----
	if (!req.stateHover.has)
		snap.stateHover = BlendOver(snap.surfaceRowAlt, snap.isDarkTheme ? RGB(255, 255, 255) : RGB(0, 0, 0), 15);
	else
		snap.stateHover = req.stateHover.value;

	if (!req.accentSubtle.has)
		snap.accentSubtle = BlendOver(snap.surfaceBase, snap.accentDefault, 31);
	else
		snap.accentSubtle = req.accentSubtle.value;

	if (!req.stateSelectedBG.has)
	{
		snap.stateSelectedBG = snap.accentSubtle;
		const double kMinSeparation = 1.25;
		for (int step = 0; step < 12; step++)
		{
			bool separated =
				ContrastRatio(snap.stateSelectedBG, snap.surfaceBase) >= kMinSeparation &&
				ContrastRatio(snap.stateSelectedBG, req.rowOddBG) >= kMinSeparation &&
				ContrastRatio(snap.stateSelectedBG, req.rowEvenBG) >= kMinSeparation;
			if (separated)
				break;

			COLORREF previous = snap.stateSelectedBG;
			snap.stateSelectedBG = BlendOver(snap.stateSelectedBG, snap.accentDefault, 24);
			if (snap.stateSelectedBG == previous)
				break;
		}
	}
	else
		snap.stateSelectedBG = req.stateSelectedBG.value;

	if (!req.textOnAccent.has)
		snap.textOnAccent = PickContrastOn(snap.accentDefault, RGB(255, 255, 255), RGB(0, 0, 0));
	else
		snap.textOnAccent = req.textOnAccent.value;

	if (!req.accentText.has)
		snap.accentText = EnsureContrast(snap.accentDefault, snap.surfaceBase);
	else
		snap.accentText = req.accentText.value;

	snap.radiusControl = req.radiusControl.has ? req.radiusControl.value : 4;
	snap.rowHeightCompact = req.rowHeightCompact.has ? req.rowHeightCompact.value : 30;
	snap.rowHeightComfortable = req.rowHeightComfortable.has ? req.rowHeightComfortable.value : 44;
	snap.fontFamily = req.fontFamily.has ? req.fontFamily.value : L"auto";

	return snap;
}

} // namespace theme
} // namespace ditto
