// Unit tests for src/ThemeColorMath.h -- pure colour maths, no MFC.
//
// Expected values come from independent sources of truth (WCAG 2.1 reference
// figures and hand-worked literals), never from the implementation.
//
// Build & run (local, no MFC needed):
//   g++ -std=c++17 -Wall -I src tests/test_theme_colors.cpp -o dist/test_theme_colors.exe
//   dist/test_theme_colors.exe

#include "../src/ThemeColorMath.h"

#include <cstdio>

static int g_failures = 0;

#define CHECK_EQ(actual, expected) do { \
	auto a_ = (actual); auto e_ = (expected); \
	if (a_ != e_) { \
		g_failures++; \
		std::printf("FAIL %s:%ld  %s = %ld, expected %ld\n", __FILE__, (long)__LINE__, #actual, (long)a_, (long)e_); \
	} \
} while (0)

#define CHECK_TRUE(cond) do { \
	if (!(cond)) { \
		g_failures++; \
		std::printf("FAIL %s:%ld  %s\n", __FILE__, (long)__LINE__, #cond); \
	} \
} while (0)

#define CHECK_NEAR(actual, expected, eps) do { \
	double a_ = (actual); double e_ = (expected); \
	if (a_ < e_ - (eps) || a_ > e_ + (eps)) { \
		g_failures++; \
		std::printf("FAIL %s:%ld  %s = %f, expected %f +/- %f\n", __FILE__, (long)__LINE__, #actual, a_, e_, (eps)); \
	} \
} while (0)

static void test_rel_luminance()
{
	// WCAG definitions: L(#FFFFFF) = 1.0, L(#000000) = 0.0
	CHECK_NEAR(RelLuminance(RGB(255, 255, 255)), 1.0, 1e-9);
	CHECK_NEAR(RelLuminance(RGB(0, 0, 0)), 0.0, 1e-9);
}

static void test_contrast_ratio()
{
	// WCAG definition: ratio between black and white is 21:1
	CHECK_NEAR(ContrastRatio(RGB(0, 0, 0), RGB(255, 255, 255)), 21.0, 1e-6);
	// #767676 is the well-known darkest grey passing 4.5:1 on white (~4.54:1)
	CHECK_NEAR(ContrastRatio(RGB(255, 255, 255), RGB(0x76, 0x76, 0x76)), 4.54, 0.01);
	// symmetric
	CHECK_NEAR(ContrastRatio(RGB(0x76, 0x76, 0x76), RGB(255, 255, 255)), 4.54, 0.01);
}

static void test_blend_over()
{
	COLORREF black = RGB(0, 0, 0);
	COLORREF white = RGB(255, 255, 255);

	// alpha 0 = under, alpha 255 = over
	CHECK_TRUE(BlendOver(black, white, 0) == black);
	CHECK_TRUE(BlendOver(black, white, 255) == white);
	// MulDiv(255, 128, 255) is exactly 128
	CHECK_TRUE(BlendOver(black, white, 128) == RGB(128, 128, 128));
	// per-channel: MulDiv(-243, 31, 255) rounds to -30, so 243 -> 213;
	// the blue channel blends up 243 + MulDiv(12, 31, 255) = 244
	CHECK_TRUE(BlendOver(RGB(243, 243, 243), RGB(0, 0, 255), 31) == RGB(213, 213, 244));
}

static void test_shift_toward()
{
	// percent 0 = identity, percent 1 = the target extreme
	COLORREF c = RGB(10, 20, 30);
	CHECK_TRUE(ShiftToward(c, true, 0.0) == c);
	CHECK_TRUE(ShiftToward(c, false, 0.0) == c);
	CHECK_TRUE(ShiftToward(c, true, 1.0) == RGB(255, 255, 255));
	CHECK_TRUE(ShiftToward(c, false, 1.0) == RGB(0, 0, 0));
	// hand-worked midpoint: 10 + 245*0.5 + 0.5 = 133 (etc. per channel)
	CHECK_TRUE(ShiftToward(c, true, 0.5) == RGB(133, 138, 143));
}

static void test_hsl_to_rgb()
{
	// the three primary hues at full saturation, l = 0.5
	CHECK_TRUE(HslToRgb(0.0f, 1.0f, 0.5f) == RGB(255, 0, 0));
	CHECK_TRUE(HslToRgb(120.0f, 1.0f, 0.5f) == RGB(0, 255, 0));
	CHECK_TRUE(HslToRgb(240.0f, 1.0f, 0.5f) == RGB(0, 0, 255));
	// s = 0 is achromatic: 0.5 * 255 + 0.5 = 128
	CHECK_TRUE(HslToRgb(200.0f, 0.0f, 0.5f) == RGB(128, 128, 128));
}

static void test_pick_contrast_on()
{
	CHECK_TRUE(PickContrastOn(RGB(255, 255, 255), RGB(0, 0, 0), RGB(255, 255, 255)) == RGB(0, 0, 0));
	CHECK_TRUE(PickContrastOn(RGB(0, 0, 0), RGB(0, 0, 0), RGB(255, 255, 255)) == RGB(255, 255, 255));
}

static void test_ensure_contrast_identity_when_compliant()
{
	// The Fluent Light accent #005FB8 against a white surface is ~6.3:1,
	// already past AA -- EnsureContrast must return it untouched, keeping the
	// accent hue instead of shifting it.
	COLORREF accent = RGB(0x00, 0x5F, 0xB8);
	COLORREF base = RGB(255, 255, 255);
	CHECK_TRUE(ContrastRatio(accent, base) >= 4.5);
	CHECK_TRUE(EnsureContrast(accent, base) == accent);
}

static void test_ensure_contrast_fixes_low_contrast()
{
	// A golden accent (#FFC800) on white is ~1.6:1 -- far below AA. The result
	// must clear 4.5:1 and keep the hue family (red/green dominant, no blue):
	// "step away from the surface" means darker gold, not a shift to grey.
	COLORREF accent = RGB(255, 200, 0);
	COLORREF base = RGB(255, 255, 255);
	CHECK_TRUE(ContrastRatio(accent, base) < 4.5);	// precondition: really fails

	COLORREF fixed = EnsureContrast(accent, base);
	CHECK_TRUE(ContrastRatio(fixed, base) >= 4.5);
	CHECK_TRUE(GetBValue(fixed) == 0);
	CHECK_TRUE(GetRValue(fixed) >= GetGValue(fixed));
}

static void test_ensure_contrast_on_dark_base()
{
	// The Fluent LIGHT accent #005FB8 dropped on a dark surface (#202020) is
	// ~2.6:1 -- the wrong-polarity accent a dark theme must survive.
	COLORREF accent = RGB(0x00, 0x5F, 0xB8);
	COLORREF base = RGB(0x20, 0x20, 0x20);
	CHECK_TRUE(ContrastRatio(accent, base) < 4.5);

	// ...so it must step toward white and clear AA while staying a blue
	// (blue channel stays the strongest).
	COLORREF fixed = EnsureContrast(accent, base);
	CHECK_TRUE(ContrastRatio(fixed, base) >= 4.5);
	CHECK_TRUE(GetBValue(fixed) >= GetRValue(fixed));
	CHECK_TRUE(GetBValue(fixed) >= GetGValue(fixed));
}

static void test_ensure_contrast_on_rows_identity_when_compliant()
{
	// Fluent Light: Accent.Text #005FB8 clears 4.5:1 against the white and
	// near-white rows the hit highlight paints on -- nothing to tighten.
	COLORREF accentText = RGB(0x00, 0x5F, 0xB8);
	CHECK_TRUE(EnsureContrastOnRows(accentText, RGB(255, 255, 255), RGB(0xF9, 0xF9, 0xF9), false) == accentText);
}

static void test_ensure_contrast_on_rows_identity_when_compliant_dark()
{
	// Fluent Dark: a light accent already clears against the dark rows.
	COLORREF accentText = RGB(0x4C, 0xC2, 0xFF);
	CHECK_TRUE(EnsureContrastOnRows(accentText, RGB(0x25, 0x25, 0x25), RGB(0x2B, 0x2B, 0x2B), true) == accentText);
}

static void test_ensure_contrast_on_rows_tightens_low_contrast()
{
	// A light-theme accent dropped on dark rows (the wrong-polarity case a
	// custom theme omitting the SearchTextHighlight node can produce) must be
	// stepped until it clears both rows while staying in the accent hue.
	COLORREF accentText = RGB(0x00, 0x5F, 0xB8);
	COLORREF odd = RGB(16, 60, 72);
	COLORREF even = RGB(23, 73, 86);
	CHECK_TRUE(ContrastRatio(accentText, odd) < 4.5);	// precondition: really fails

	COLORREF fixed = EnsureContrastOnRows(accentText, odd, even, true);
	CHECK_TRUE(ContrastRatio(fixed, odd) >= 4.5);
	CHECK_TRUE(ContrastRatio(fixed, even) >= 4.5);
	CHECK_TRUE(GetBValue(fixed) >= GetRValue(fixed));
	CHECK_TRUE(GetBValue(fixed) >= GetGValue(fixed));
}

int main()
{
	test_rel_luminance();
	test_contrast_ratio();
	test_blend_over();
	test_shift_toward();
	test_hsl_to_rgb();
	test_pick_contrast_on();
	test_ensure_contrast_identity_when_compliant();
	test_ensure_contrast_fixes_low_contrast();
	test_ensure_contrast_on_dark_base();
	test_ensure_contrast_on_rows_identity_when_compliant();
	test_ensure_contrast_on_rows_identity_when_compliant_dark();
	test_ensure_contrast_on_rows_tightens_low_contrast();

	if (g_failures > 0)
	{
		std::printf("%d failure(s)\n", g_failures);
		return 1;
	}
	std::printf("all theme colour tests passed\n");
	return 0;
}
