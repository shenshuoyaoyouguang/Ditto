// Unit tests for src/ThemeTokens.h -- read-only theme-token snapshot producer.
// Pure C++17, no MFC/CString: compiles standalone like test_theme_colors.cpp.
//
// Expected values come from independent sources of truth (WCAG 2.1 reference
// figures and hand-worked MulDiv blends), never from the implementation.
//
// Build & run (local, no MFC needed):
//   cl /nologo /std:c++17 /W4 /EHsc /I src tests\test_theme_tokens.cpp /Fe:test_theme_tokens.exe
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

#define CHECK_COLOR(actual, r, g, b) do { \
	COLORREF a_ = (actual); COLORREF e_ = RGB((r), (g), (b)); \
	if (a_ != e_) { \
		g_failures++; \
		std::printf("FAIL %s:%ld  %s = RGB(%lu,%lu,%lu), expected RGB(%d,%d,%d)\n", \
			__FILE__, (long)__LINE__, #actual, \
			(unsigned long)GetRValue(a_), (unsigned long)GetGValue(a_), (unsigned long)GetBValue(a_), \
			(r), (g), (b)); \
	} \
} while (0)

// Slice A: default palettes. Empty request (no xml, no overrides) resolves to
// the OS-polarity default palette with the built-in accent.

static void test_light_defaults_carry_finished_values()
{
	ditto::theme::ThemeSnapshotRequest req;
	req.osDark = false;
	req.parsedXml = false;
	// Hand-picked built-in accent so the subtle blend below is hand-workable.
	req.builtInAccent = RGB(0x00, 0x5F, 0xB8);

	ditto::theme::ThemeTokenSnapshot snap = ditto::theme::BuildThemeSnapshot(req);

	CHECK_COLOR(snap.surfaceBase, 0xF3, 0xF3, 0xF3);
	CHECK_COLOR(snap.surfaceElevated, 0xFF, 0xFF, 0xFF);
	CHECK_COLOR(snap.surfaceRowAlt, 0xF9, 0xF9, 0xF9);
	CHECK_COLOR(snap.textPrimary, 0x1A, 0x1A, 0x1A);
	CHECK_COLOR(snap.textSecondary, 0x61, 0x61, 0x61);
	CHECK_COLOR(snap.textDisabled, 0x9D, 0x9D, 0x9D);
	CHECK_COLOR(snap.accentDefault, 0x00, 0x5F, 0xB8);
	CHECK_COLOR(snap.strokeCard, 0xE5, 0xE5, 0xE5);
	CHECK_COLOR(snap.strokeDivider, 0xED, 0xED, 0xED);
	CHECK_COLOR(snap.controlFill, 0xFB, 0xFB, 0xFB);
	CHECK_COLOR(snap.controlHover, 0xF0, 0xF0, 0xF0);
	CHECK_COLOR(snap.controlPressed, 0xED, 0xED, 0xED);
	CHECK_COLOR(snap.controlDisabledBG, 0xF5, 0xF5, 0xF5);
	CHECK_COLOR(snap.indicatorBadge, 0x61, 0x61, 0x61);
	// Hover = BlendOver(#F9F9F9, black, 15): MulDiv(-249,15,255) = -15
	// (hand-worked), so 249 - 15 = 234 per channel.
	CHECK_COLOR(snap.stateHover, 234, 234, 234);
	// Accent.Subtle = BlendOver(#F3F3F3, #005FB8, 31), hand-worked per channel:
	// R: MulDiv(-243,31,255) = -30 -> 213; G: MulDiv(-148,31,255) = -18 -> 225;
	// B: MulDiv(-59,31,255) = -7 -> 236.
	CHECK_COLOR(snap.accentSubtle, 213, 225, 236);
	// #005FB8 on near-white clears WCAG AA already, so Accent.Text keeps the hue.
	CHECK_TRUE(snap.accentText == snap.accentDefault);
	// White contrasts ~6.3:1 against #005FB8, black only ~3.3:1.
	CHECK_COLOR(snap.textOnAccent, 0xFF, 0xFF, 0xFF);
	CHECK_TRUE(snap.radiusControl == 4);
	CHECK_TRUE(snap.rowHeightCompact == 30);
	CHECK_TRUE(snap.rowHeightComfortable == 44);
	CHECK_TRUE(snap.fontFamily == L"auto");
	CHECK_TRUE(snap.isDarkTheme == false);
}

static void test_dark_defaults_carry_finished_values()
{
	ditto::theme::ThemeSnapshotRequest req;
	req.osDark = true;
	req.parsedXml = false;
	req.builtInAccent = RGB(0x4C, 0xC2, 0xFF);

	ditto::theme::ThemeTokenSnapshot snap = ditto::theme::BuildThemeSnapshot(req);

	CHECK_COLOR(snap.surfaceBase, 0x20, 0x20, 0x20);
	CHECK_COLOR(snap.surfaceElevated, 0x2B, 0x2B, 0x2B);
	CHECK_COLOR(snap.surfaceRowAlt, 0x26, 0x26, 0x26);
	CHECK_COLOR(snap.textPrimary, 0xFF, 0xFF, 0xFF);
	CHECK_COLOR(snap.textSecondary, 0xC8, 0xC8, 0xC8);
	CHECK_COLOR(snap.textDisabled, 0x71, 0x71, 0x71);
	CHECK_COLOR(snap.accentDefault, 0x4C, 0xC2, 0xFF);
	CHECK_COLOR(snap.strokeCard, 0x3D, 0x3D, 0x3D);
	CHECK_COLOR(snap.strokeDivider, 0x33, 0x33, 0x33);
	CHECK_COLOR(snap.controlFill, 0x2D, 0x2D, 0x2D);
	CHECK_COLOR(snap.controlHover, 0x38, 0x38, 0x38);
	CHECK_COLOR(snap.controlPressed, 0x3A, 0x3A, 0x3A);
	CHECK_COLOR(snap.controlDisabledBG, 0x29, 0x29, 0x29);
	CHECK_COLOR(snap.indicatorBadge, 0xC8, 0xC8, 0xC8);
	// Hover = BlendOver(#262626, white, 15): MulDiv(217,15,255) = +13
	// (217*15 = 3255, 3255/255 = 12.76 -> 13), so 38 + 13 = 51.
	CHECK_COLOR(snap.stateHover, 51, 51, 51);
	CHECK_TRUE(snap.isDarkTheme == true);
	CHECK_TRUE(snap.fontFamily == L"auto");
}

// Slice B: explicit per-token overrides win; the dark flag re-resolves from
// the overridden base (mirrors FinalizeTokens recomputing m_bDarkTheme).

static void test_explicit_overrides_win()
{
	ditto::theme::ThemeSnapshotRequest req;
	req.osDark = false;
	req.parsedXml = false;
	req.builtInAccent = RGB(0x00, 0x5F, 0xB8);
	req.stateHover.has = true;
	req.stateHover.value = RGB(0x12, 0x34, 0x56);
	req.surfaceBase.has = true;
	req.surfaceBase.value = RGB(0x00, 0x00, 0x00);

	ditto::theme::ThemeTokenSnapshot snap = ditto::theme::BuildThemeSnapshot(req);

	CHECK_TRUE(snap.stateHover == RGB(0x12, 0x34, 0x56));
	CHECK_TRUE(snap.isDarkTheme == true);
	// Accent refresh still runs against the overridden base:
	// Subtle = BlendOver(black, #005FB8, 31): R stays 0,
	// G: MulDiv(95,31,255) = 2945/255 = 11.55 -> 12,
	// B: MulDiv(184,31,255) = 5704/255 = 22.37 -> 22.
	CHECK_COLOR(snap.accentSubtle, 0, 12, 22);
}

// Slice B: a dark legacy MainWindowBG on a light OS flips the whole
// derivation to the dark branch (mirrors DeriveTokensFromLegacy judging
// polarity from the theme's own base, not the OS setting).

static void test_legacy_dark_base_drives_derivation()
{
	ditto::theme::ThemeSnapshotRequest req;
	req.osDark = false;
	req.parsedXml = true;
	req.hasLegacyMainWindowBG = true;
	req.legacyMainWindowBG = RGB(0x20, 0x20, 0x20);

	ditto::theme::ThemeTokenSnapshot snap = ditto::theme::BuildThemeSnapshot(req);

	CHECK_COLOR(snap.surfaceBase, 0x20, 0x20, 0x20);
	// Elevated = ShiftToward(#202020, black, 0.04): (32 - 1.28 + 0.5) = 31.22 -> 31.
	CHECK_COLOR(snap.surfaceElevated, 31, 31, 31);
	// RowAlt = ShiftToward(#202020, black, 0.01): (32 - 0.32 + 0.5) = 32.18 -> 32.
	CHECK_COLOR(snap.surfaceRowAlt, 32, 32, 32);
	CHECK_COLOR(snap.textPrimary, 0xFF, 0xFF, 0xFF);
	// Secondary = BlendOver(#202020, white, 158):
	// 32 + MulDiv(223,158,255) = 32 + (35234/255 = 138.17 -> 138) = 170.
	CHECK_COLOR(snap.textSecondary, 170, 170, 170);
	// Disabled = BlendOver(#202020, #AAAAAA, 153):
	// 32 + MulDiv(138,153,255) = 32 + (21114/255 = 82.80 -> 83) = 115.
	CHECK_COLOR(snap.textDisabled, 115, 115, 115);
	// Card = per-channel int average of base and elevated: (32+31)/2 = 31.
	CHECK_COLOR(snap.strokeCard, 31, 31, 31);
	// Divider = BlendOver(#1F1F1F, #202020, 64): 31 + MulDiv(1,64,255) = 31 + 0.
	CHECK_COLOR(snap.strokeDivider, 31, 31, 31);
	CHECK_COLOR(snap.controlFill, 31, 31, 31);
	CHECK_COLOR(snap.controlHover, 31, 31, 31);
	// Pressed = ShiftToward(#1F1F1F, black, 0.02): (31 - 0.62 + 0.5) = 30.88 -> 30.
	CHECK_COLOR(snap.controlPressed, 30, 30, 30);
	// DisabledBG = ShiftToward(#202020, black, 0.015): (32 - 0.48 + 0.5) = 32.02 -> 32.
	CHECK_COLOR(snap.controlDisabledBG, 32, 32, 32);
	CHECK_COLOR(snap.indicatorBadge, 170, 170, 170);
	CHECK_COLOR(snap.accentDefault, 0x4C, 0xC2, 0xFF);
	// Hover = BlendOver(#202020, white, 15):
	// 32 + MulDiv(223,15,255) = 32 + (3345/255 = 13.12 -> 13) = 45.
	CHECK_COLOR(snap.stateHover, 45, 45, 45);
	// Subtle starts at BlendOver(#202020, #4CC2FF, 31) = (37,52,59), which
	// already separates ~1.27:1 from the base, so the walk correctly leaves
	// it alone while the finished pill still clears 1.25:1 everywhere.
	CHECK_COLOR(snap.stateSelectedBG, 37, 52, 59);
	CHECK_TRUE(ContrastRatio(snap.stateSelectedBG, snap.surfaceBase) >= 1.25);
	CHECK_TRUE(ContrastRatio(snap.stateSelectedBG, RGB(255, 255, 255)) >= 1.25);
	CHECK_TRUE(ContrastRatio(snap.stateSelectedBG, RGB(243, 243, 243)) >= 1.25);
	// #4CC2FF on near-black clears AA untouched, and black wins on the accent
	// (10.47:1 vs white 2.01:1, hand-worked from the WCAG formula).
	CHECK_TRUE(snap.accentText == snap.accentDefault);
	CHECK_COLOR(snap.textOnAccent, 0x00, 0x00, 0x00);
	CHECK_TRUE(snap.isDarkTheme == true);
	CHECK_TRUE(snap.fontFamily == L"auto");
}

// Slice B: legacy v3 nodes that are present win over synthesis (mirrors the
// hasLegacy* branches of DeriveTokensFromLegacy).

static void test_legacy_nodes_win_where_present()
{
	ditto::theme::ThemeSnapshotRequest req;
	req.osDark = false;
	req.parsedXml = true;
	req.hasLegacySearchFocusBG = true;
	req.legacySearchFocusBG = RGB(0xFF, 0xFF, 0xFF);
	req.hasLegacyEvenRowsBG = true;
	req.rowEvenBG = RGB(0xEE, 0xEE, 0xEE);
	req.hasLegacyOddRowsText = true;
	req.legacyOddRowsText = RGB(0x11, 0x11, 0x11);
	req.hasLegacyClipPasted = true;
	req.legacyClipPasted = RGB(0x00, 0x80, 0x00);
	req.hasLegacyQuickPasteIndex = true;
	req.legacyQuickPasteIndex = RGB(0x22, 0x22, 0x22);

	ditto::theme::ThemeTokenSnapshot snap = ditto::theme::BuildThemeSnapshot(req);

	CHECK_COLOR(snap.surfaceBase, 0xF3, 0xF3, 0xF3);
	CHECK_COLOR(snap.surfaceElevated, 0xFF, 0xFF, 0xFF);
	CHECK_COLOR(snap.surfaceRowAlt, 0xEE, 0xEE, 0xEE);
	CHECK_COLOR(snap.textPrimary, 0x11, 0x11, 0x11);
	// Secondary = BlendOver(#F3F3F3, #111111, 158):
	// 243 + MulDiv(-226,158,255) = 243 - (35708/255 = 140.03 -> 140) = 103.
	CHECK_COLOR(snap.textSecondary, 103, 103, 103);
	// Disabled = BlendOver(#F3F3F3, #676767, 153):
	// 243 + MulDiv(-140,153,255) = 243 - (21420/255 = 84.0 -> 84) = 159.
	CHECK_COLOR(snap.textDisabled, 159, 159, 159);
	CHECK_COLOR(snap.accentDefault, 0x00, 0x80, 0x00);
	CHECK_COLOR(snap.indicatorBadge, 0x22, 0x22, 0x22);
	// #008000 on #F3F3F3 is ~4.63:1 (hand-worked WCAG), so Accent.Text keeps it.
	CHECK_TRUE(snap.accentText == snap.accentDefault);
	CHECK_TRUE(snap.isDarkTheme == false);
}

// Slice B: a low-contrast accent keeps its hue family while clearing AA
// through the snapshot seam (same golden accent as the EnsureContrast unit).

static void test_golden_accent_keeps_hue_and_clears_aa()
{
	ditto::theme::ThemeSnapshotRequest req;
	req.osDark = false;
	req.parsedXml = false;
	req.builtInAccent = RGB(255, 200, 0);

	ditto::theme::ThemeTokenSnapshot snap = ditto::theme::BuildThemeSnapshot(req);

	CHECK_TRUE(ContrastRatio(RGB(255, 200, 0), snap.surfaceBase) < 4.5);
	CHECK_TRUE(ContrastRatio(snap.accentText, snap.surfaceBase) >= 4.5);
	CHECK_TRUE(GetBValue(snap.accentText) == 0);
	CHECK_TRUE(GetRValue(snap.accentText) >= GetGValue(snap.accentText));
}

// Slice B: mid-grey accent -- white is ~3.95:1, black ~5.32:1 (hand-worked),
// so text-on-accent resolves to black and the selection pill separates.

static void test_grey_accent_picks_black_on_accent()
{
	ditto::theme::ThemeSnapshotRequest req;
	req.osDark = false;
	req.parsedXml = false;
	req.builtInAccent = RGB(0x80, 0x80, 0x80);

	ditto::theme::ThemeTokenSnapshot snap = ditto::theme::BuildThemeSnapshot(req);

	CHECK_COLOR(snap.textOnAccent, 0x00, 0x00, 0x00);
	// Subtle starts at BlendOver(#F3F3F3, grey, 31) = (229,229,229), too close
	// to the base, so the walk must move it to >= 1.25:1 separation.
	CHECK_TRUE(snap.stateSelectedBG != RGB(229, 229, 229));
	CHECK_TRUE(ContrastRatio(snap.stateSelectedBG, snap.surfaceBase) >= 1.25);
}

// Slice B: sizes and font family overrides are carried verbatim.

static void test_size_and_font_overrides()
{
	ditto::theme::ThemeSnapshotRequest req;
	req.osDark = false;
	req.parsedXml = false;
	req.builtInAccent = RGB(0x00, 0x5F, 0xB8);
	req.radiusControl.has = true;
	req.radiusControl.value = 8;
	req.rowHeightCompact.has = true;
	req.rowHeightCompact.value = 26;
	req.rowHeightComfortable.has = true;
	req.rowHeightComfortable.value = 40;
	req.fontFamily.has = true;
	req.fontFamily.value = L"Segoe UI";

	ditto::theme::ThemeTokenSnapshot snap = ditto::theme::BuildThemeSnapshot(req);

	CHECK_TRUE(snap.radiusControl == 8);
	CHECK_TRUE(snap.rowHeightCompact == 26);
	CHECK_TRUE(snap.rowHeightComfortable == 40);
	CHECK_TRUE(snap.fontFamily == L"Segoe UI");
}

// Publish parity: these build the request the way CTheme::PublishTokenSnapshot
// does (member flag + member value per token, concrete legacy rows as the
// walk refs, ClipPastedColor as the built-in accent) and assert the producer
// output against independent hand-worked values. If the runtime mapping drops
// or miswires a field, these fail; they are the lock on the single producer.

static ditto::theme::ThemeSnapshotRequest PublishStyleRequest()
{
	// Simulated post-LoadDefaults member state, light OS, no xml parsed:
	// every has-flag false, ClipPastedColor at its RGB(0,255,0) default.
	ditto::theme::ThemeSnapshotRequest req;
	req.osDark = false;
	req.parsedXml = false;
	req.builtInAccent = RGB(0, 255, 0);
	req.rowOddBG = RGB(255, 255, 255);
	req.rowEvenBG = RGB(243, 243, 243);
	return req;
}

static void test_publish_builtin_palette_matches_producer()
{
	ditto::theme::ThemeTokenSnapshot snap =
		ditto::theme::BuildThemeSnapshot(PublishStyleRequest());

	CHECK_COLOR(snap.surfaceBase, 0xF3, 0xF3, 0xF3);
	CHECK_COLOR(snap.surfaceElevated, 0xFF, 0xFF, 0xFF);
	CHECK_COLOR(snap.surfaceRowAlt, 0xF9, 0xF9, 0xF9);
	CHECK_COLOR(snap.textPrimary, 0x1A, 0x1A, 0x1A);
	CHECK_COLOR(snap.accentDefault, 0x00, 0xFF, 0x00);
	// Accent.Subtle = BlendOver(#F3F3F3, lime, 31), hand-worked per channel:
	// R/B: MulDiv(-243,31,255) = -7533/255 = -29.54 -> -30 -> 213;
	// G: MulDiv(12,31,255) = 372/255 = 1.46 -> 1 -> 244.
	CHECK_COLOR(snap.accentSubtle, 213, 244, 213);
	// Lime on near-white is ~1.2:1; black (15.3:1) beats white (1.4:1).
	CHECK_COLOR(snap.textOnAccent, 0x00, 0x00, 0x00);
	// Lime shifted toward black keeps R=B=0 and clears WCAG AA.
	CHECK_TRUE(GetRValue(snap.accentText) == 0);
	CHECK_TRUE(GetBValue(snap.accentText) == 0);
	CHECK_TRUE(GetGValue(snap.accentText) > 0);
	CHECK_TRUE(ContrastRatio(snap.accentText, snap.surfaceBase) >= 4.5);
	CHECK_COLOR(snap.stateHover, 234, 234, 234);
	CHECK_TRUE(ContrastRatio(snap.stateSelectedBG, snap.surfaceBase) >= 1.25);
	CHECK_TRUE(ContrastRatio(snap.stateSelectedBG, RGB(255, 255, 255)) >= 1.25);
	CHECK_TRUE(ContrastRatio(snap.stateSelectedBG, RGB(243, 243, 243)) >= 1.25);
	CHECK_TRUE(snap.radiusControl == 4);
	CHECK_TRUE(snap.rowHeightCompact == 30);
	CHECK_TRUE(snap.rowHeightComfortable == 44);
	CHECK_TRUE(snap.fontFamily == L"auto");
	CHECK_TRUE(snap.isDarkTheme == false);
}

static void test_publish_legacy_dark_matches_producer()
{
	// Simulated legacy member state: dark MainWindowBG parsed from xml, rows
	// at their LoadDefaults pair, no explicit tokens, no other legacy nodes.
	ditto::theme::ThemeSnapshotRequest req = PublishStyleRequest();
	req.parsedXml = true;
	req.hasLegacyMainWindowBG = true;
	req.legacyMainWindowBG = RGB(0x20, 0x20, 0x20);

	ditto::theme::ThemeTokenSnapshot snap = ditto::theme::BuildThemeSnapshot(req);

	// Same hand-worked derivation as test_legacy_dark_base_drives_derivation.
	CHECK_COLOR(snap.surfaceBase, 0x20, 0x20, 0x20);
	CHECK_COLOR(snap.surfaceElevated, 31, 31, 31);
	CHECK_COLOR(snap.surfaceRowAlt, 32, 32, 32);
	CHECK_COLOR(snap.textPrimary, 0xFF, 0xFF, 0xFF);
	CHECK_COLOR(snap.textSecondary, 170, 170, 170);
	CHECK_COLOR(snap.textDisabled, 115, 115, 115);
	CHECK_COLOR(snap.accentDefault, 0x4C, 0xC2, 0xFF);
	CHECK_COLOR(snap.stateHover, 45, 45, 45);
	CHECK_COLOR(snap.stateSelectedBG, 37, 52, 59);
	CHECK_TRUE(ContrastRatio(snap.stateSelectedBG, snap.surfaceBase) >= 1.25);
	CHECK_TRUE(ContrastRatio(snap.stateSelectedBG, RGB(255, 255, 255)) >= 1.25);
	CHECK_TRUE(ContrastRatio(snap.stateSelectedBG, RGB(243, 243, 243)) >= 1.25);
	CHECK_COLOR(snap.textOnAccent, 0x00, 0x00, 0x00);
	CHECK_TRUE(snap.isDarkTheme == true);
}

static void test_publish_explicit_override_matches_producer()
{
	// Simulated member state with explicit tokens (has-flags true): black
	// Surface.Base plus a pinned hover, as CTheme holds after LoadTokensFromXml.
	ditto::theme::ThemeSnapshotRequest req = PublishStyleRequest();
	req.builtInAccent = RGB(0x00, 0x5F, 0xB8);
	req.stateHover.has = true;
	req.stateHover.value = RGB(0x12, 0x34, 0x56);
	req.surfaceBase.has = true;
	req.surfaceBase.value = RGB(0x00, 0x00, 0x00);
	req.fontFamily.has = true;
	req.fontFamily.value = L"Segoe UI";
	req.radiusControl.has = true;
	req.radiusControl.value = 8;

	ditto::theme::ThemeTokenSnapshot snap = ditto::theme::BuildThemeSnapshot(req);

	CHECK_TRUE(snap.stateHover == RGB(0x12, 0x34, 0x56));
	CHECK_TRUE(snap.isDarkTheme == true);
	// Same hand-worked subtle as test_explicit_overrides_win.
	CHECK_COLOR(snap.accentSubtle, 0, 12, 22);
	CHECK_TRUE(snap.fontFamily == L"Segoe UI");
	CHECK_TRUE(snap.radiusControl == 8);
}

// Copy fields: the nineteen pure-copy members (unfocused selection,
// scrollbar, active caption, TopMost/NotConnected caption variants, caption
// sizes, legacy row pair) carry request -> snapshot identity. Values are
// hand-picked distinct literals so a miswired field cannot pass by coincidence.

static void test_copy_fields_carry_identity()
{
	ditto::theme::ThemeSnapshotRequest req;
	req.osDark = false;
	req.parsedXml = false;
	req.builtInAccent = RGB(0x00, 0x5F, 0xB8);
	req.stateSelectedNoFocusBG = RGB(0xCC, 0xCC, 0xCC);
	req.stateSelectedNoFocusText = RGB(0x11, 0x22, 0x33);
	req.scrollBarTrack = RGB(0xF0, 0xF0, 0xF0);
	req.scrollBarThumb = RGB(0xB4, 0xB4, 0xB4);
	req.scrollBarThumbHover = RGB(0x8C, 0x8C, 0x8C);
	req.captionLeft = RGB(0xFF, 0xFF, 0xFF);
	req.captionRight = RGB(0xCC, 0xCC, 0xCC);
	req.captionTextColor = RGB(0xBF, 0xBF, 0xBF);
	req.captionBorder = RGB(0x99, 0x88, 0x77);
	req.captionLeftTopMost = RGB(0x10, 0x20, 0x30);
	req.captionRightTopMost = RGB(0x40, 0x50, 0x60);
	req.borderTopMost = RGB(0x70, 0x80, 0x90);
	req.captionLeftNotConnected = RGB(0x01, 0x02, 0x03);
	req.captionRightNotConnected = RGB(0xFF, 0xFF, 0x00);
	req.borderNotConnected = RGB(0x04, 0x05, 0x06);
	req.captionSize = 28;
	req.captionFontSize = 12;
	req.listBoxOddRowsBG = RGB(0xAA, 0xBB, 0xCC);
	req.listBoxEvenRowsBG = RGB(0xDD, 0xEE, 0xFF);

	ditto::theme::ThemeTokenSnapshot snap = ditto::theme::BuildThemeSnapshot(req);

	CHECK_TRUE(snap.stateSelectedNoFocusBG == RGB(0xCC, 0xCC, 0xCC));
	CHECK_TRUE(snap.stateSelectedNoFocusText == RGB(0x11, 0x22, 0x33));
	CHECK_TRUE(snap.scrollBarTrack == RGB(0xF0, 0xF0, 0xF0));
	CHECK_TRUE(snap.scrollBarThumb == RGB(0xB4, 0xB4, 0xB4));
	CHECK_TRUE(snap.scrollBarThumbHover == RGB(0x8C, 0x8C, 0x8C));
	CHECK_TRUE(snap.captionLeft == RGB(0xFF, 0xFF, 0xFF));
	CHECK_TRUE(snap.captionRight == RGB(0xCC, 0xCC, 0xCC));
	CHECK_TRUE(snap.captionTextColor == RGB(0xBF, 0xBF, 0xBF));
	CHECK_TRUE(snap.captionBorder == RGB(0x99, 0x88, 0x77));
	CHECK_TRUE(snap.captionLeftTopMost == RGB(0x10, 0x20, 0x30));
	CHECK_TRUE(snap.captionRightTopMost == RGB(0x40, 0x50, 0x60));
	CHECK_TRUE(snap.borderTopMost == RGB(0x70, 0x80, 0x90));
	CHECK_TRUE(snap.captionLeftNotConnected == RGB(0x01, 0x02, 0x03));
	CHECK_TRUE(snap.captionRightNotConnected == RGB(0xFF, 0xFF, 0x00));
	CHECK_TRUE(snap.borderNotConnected == RGB(0x04, 0x05, 0x06));
	CHECK_TRUE(snap.captionSize == 28);
	CHECK_TRUE(snap.captionFontSize == 12);
	CHECK_TRUE(snap.listBoxOddRowsBG == RGB(0xAA, 0xBB, 0xCC));
	CHECK_TRUE(snap.listBoxEvenRowsBG == RGB(0xDD, 0xEE, 0xFF));
}

// Focus distinction: the unfocused-selection tokens keep their own values even
// when the focused-selection tokens resolve differently, so the list can tell
// focused from unfocused selection apart (story-10 regression lock).

static void test_no_focus_selection_stays_distinct()
{
	ditto::theme::ThemeSnapshotRequest req;
	req.osDark = false;
	req.parsedXml = false;
	req.builtInAccent = RGB(0x00, 0x5F, 0xB8);
	req.stateSelectedBG.has = true;
	req.stateSelectedBG.value = RGB(0xD5, 0xE1, 0xEC);
	req.stateSelectedText.has = true;
	req.stateSelectedText.value = RGB(0x1A, 0x1A, 0x1A);
	req.stateSelectedNoFocusBG = RGB(0xCC, 0xCC, 0xCC);
	req.stateSelectedNoFocusText = RGB(0x00, 0x00, 0x00);

	ditto::theme::ThemeTokenSnapshot snap = ditto::theme::BuildThemeSnapshot(req);

	CHECK_TRUE(snap.stateSelectedBG == RGB(0xD5, 0xE1, 0xEC));
	CHECK_TRUE(snap.stateSelectedNoFocusBG == RGB(0xCC, 0xCC, 0xCC));
	CHECK_TRUE(snap.stateSelectedBG != snap.stateSelectedNoFocusBG);
	CHECK_TRUE(snap.stateSelectedNoFocusText == RGB(0x00, 0x00, 0x00));
}

// Snapshot invariants (ported from the first-slice seam): the producer's
// finished values obey the same three constraints the member-copy seam
// checked -- dark-flag-vs-surface polarity, accent-text 4.5:1 on the surface,
// selected-pill 1.25:1 separation from the surface and BOTH stored row refs.
// Expected values are hand-worked WCAG figures, never implementation output.

static void test_snapshot_dark_flag_follows_surface()
{
	// WCAG: L(#FFFFFF) = 1.0 (light), L(#000000) = 0.0 (dark). The flag must
	// agree with the overridden surface polarity.
	ditto::theme::ThemeSnapshotRequest req;
	req.osDark = false;
	req.parsedXml = false;
	req.builtInAccent = RGB(0x00, 0x5F, 0xB8);
	req.surfaceBase.has = true;

	req.surfaceBase.value = RGB(255, 255, 255);
	CHECK_TRUE(ditto::theme::BuildThemeSnapshot(req).isDarkTheme == false);

	req.surfaceBase.value = RGB(0, 0, 0);
	CHECK_TRUE(ditto::theme::BuildThemeSnapshot(req).isDarkTheme == true);

	// Mid-grey #808080 has L ~= 0.216 (dark side), so the flag must say dark
	// even though the OS setting above is light.
	req.surfaceBase.value = RGB(0x80, 0x80, 0x80);
	CHECK_TRUE(ditto::theme::BuildThemeSnapshot(req).isDarkTheme == true);
}

static void test_snapshot_accent_text_clears_surface()
{
	// The Fluent Light accent #005FB8 on near-white is ~6.3:1 (hand-worked
	// WCAG figure), past AA -- the producer keeps the hue.
	ditto::theme::ThemeSnapshotRequest req;
	req.osDark = false;
	req.parsedXml = false;
	req.builtInAccent = RGB(0x00, 0x5F, 0xB8);

	ditto::theme::ThemeTokenSnapshot snap = ditto::theme::BuildThemeSnapshot(req);
	CHECK_TRUE(snap.accentText == RGB(0x00, 0x5F, 0xB8));
	CHECK_TRUE(ContrastRatio(snap.accentText, snap.surfaceBase) >= 4.5);

	// White on white is exactly 1:1 ((1.0 + 0.05) / (1.0 + 0.05)), far below
	// AA -- feeding white as the accent must NOT survive: the guarantee
	// shifts it until it clears 4.5:1.
	CHECK_TRUE(ContrastRatio(RGB(255, 255, 255), RGB(255, 255, 255)) == 1.0);
	req.builtInAccent = RGB(255, 255, 255);
	snap = ditto::theme::BuildThemeSnapshot(req);
	CHECK_TRUE(snap.accentText != RGB(255, 255, 255));
	CHECK_TRUE(ContrastRatio(snap.accentText, snap.surfaceBase) >= 4.5);
}

static void test_snapshot_selected_bg_separates_from_rows()
{
	// kMinSeparation = 1.25:1. Pin one walk ref exactly onto the pill's
	// hand-worked start (light BlendOver(#F3F3F3, #005FB8, 31) = (213,225,236),
	// ~1:1 against that row): the walk must move the pill until it separates
	// from the surface AND both stored row refs.
	ditto::theme::ThemeSnapshotRequest req;
	req.osDark = false;
	req.parsedXml = false;
	req.builtInAccent = RGB(0x00, 0x5F, 0xB8);
	req.rowOddBG = RGB(255, 255, 255);
	req.rowEvenBG = RGB(213, 225, 236);
	// Production wiring (PublishTokenSnapshot) fills the walk refs and the
	// stored row pair from the same members, so mirror that here: the pill
	// must separate from the stored refs too.
	req.listBoxOddRowsBG = RGB(255, 255, 255);
	req.listBoxEvenRowsBG = RGB(213, 225, 236);

	ditto::theme::ThemeTokenSnapshot snap = ditto::theme::BuildThemeSnapshot(req);
	CHECK_TRUE(snap.stateSelectedBG != RGB(213, 225, 236));
	CHECK_TRUE(ContrastRatio(snap.stateSelectedBG, snap.surfaceBase) >= 1.25);
	CHECK_TRUE(ContrastRatio(snap.stateSelectedBG, snap.listBoxOddRowsBG) >= 1.25);
	CHECK_TRUE(ContrastRatio(snap.stateSelectedBG, snap.listBoxEvenRowsBG) >= 1.25);
}

int main()
{
	test_light_defaults_carry_finished_values();
	test_dark_defaults_carry_finished_values();
	test_explicit_overrides_win();
	test_legacy_dark_base_drives_derivation();
	test_legacy_nodes_win_where_present();
	test_golden_accent_keeps_hue_and_clears_aa();
	test_grey_accent_picks_black_on_accent();
	test_size_and_font_overrides();
	test_publish_builtin_palette_matches_producer();
	test_publish_legacy_dark_matches_producer();
	test_publish_explicit_override_matches_producer();
	test_copy_fields_carry_identity();
	test_no_focus_selection_stays_distinct();
	test_snapshot_dark_flag_follows_surface();
	test_snapshot_accent_text_clears_surface();
	test_snapshot_selected_bg_separates_from_rows();

	if (g_failures > 0)
	{
		std::printf("%d failure(s)\n", g_failures);
		return 1;
	}
	std::printf("all theme token tests passed\n");
	return 0;
}
