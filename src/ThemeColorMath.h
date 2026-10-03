#pragma once

// Pure colour maths shared by the theme token derivation (Theme.cpp) and the
// owner-drawn Fluent option controls (FluentOptionPaint). No MFC/CString
// dependencies: only Win32 COLORREF macros, so tests/test_theme_colors.cpp
// can compile this header outside the main project.
//
// The functions keep the names CTheme's static members used to have; the
// bodies are unchanged, only EnsureContrast gained the base colour as an
// explicit parameter (it used to read the theme's m_surfaceBase).

#include <windows.h>
#include <cmath>

// min/max as used here are macros under MSVC's windows.h but absent under
// MinGW, so the clamping is spelled out to compile in both.
inline BYTE ClampByte(int value)
{
	if (value < 0)
		return 0;
	if (value > 255)
		return 255;
	return (BYTE)value;
}

inline COLORREF HslToRgb(float h, float s, float l)
{
	if (s == 0.0f)
	{
		// Grayscale, achromatic
		BYTE gray = static_cast<BYTE>(l * 255.0f + 0.5f);
		return RGB(gray, gray, gray);
	}

	auto hueToRgb = [](float p, float q, float t) -> float
	{
		if (t < 0.0f) t += 1.0f;
		if (t > 1.0f) t -= 1.0f;
		if (t < 1.0f / 6.0f) return p + (q - p) * 6.0f * t;
		if (t < 1.0f / 2.0f) return q;
		if (t < 2.0f / 3.0f) return p + (q - p) * (2.0f / 3.0f - t) * 6.0f;
		return p;
	};

	float q = l < 0.5f ? l * (1.0f + s) : l + s - l * s;
	float p = 2.0f * l - q;
	float h_norm = h / 360.0f;

	float r_f = hueToRgb(p, q, h_norm + 1.0f / 3.0f);
	float g_f = hueToRgb(p, q, h_norm);
	float b_f = hueToRgb(p, q, h_norm - 1.0f / 3.0f);

	BYTE r = static_cast<BYTE>(r_f * 255.0f + 0.5f);
	BYTE g = static_cast<BYTE>(g_f * 255.0f + 0.5f);
	BYTE b = static_cast<BYTE>(b_f * 255.0f + 0.5f);

	return RGB(r, g, b);
}

inline COLORREF BlendOver(COLORREF under, COLORREF over, int alpha)
{
	auto blend = [alpha](int u, int o) -> BYTE
	{
		return ClampByte(u + MulDiv(o - u, alpha, 255));
	};

	return RGB(blend(GetRValue(under), GetRValue(over)),
		blend(GetGValue(under), GetGValue(over)),
		blend(GetBValue(under), GetBValue(over)));
}

inline COLORREF ShiftToward(COLORREF color, bool towardWhite, double percent)
{
	int target = towardWhite ? 255 : 0;

	auto shift = [target, percent](int c) -> BYTE
	{
		return ClampByte((int)(c + (target - c) * percent + 0.5));
	};

	return RGB(shift(GetRValue(color)), shift(GetGValue(color)), shift(GetBValue(color)));
}

inline double RelLuminance(COLORREF color)
{
	auto linear = [](int channel) -> double
	{
		double c = channel / 255.0;
		return c <= 0.03928 ? c / 12.92 : pow((c + 0.055) / 1.055, 2.4);
	};

	return 0.2126 * linear(GetRValue(color)) + 0.7152 * linear(GetGValue(color)) + 0.0722 * linear(GetBValue(color));
}

// WCAG 2.1 relative contrast ratio, 1.0 .. 21.0
inline double ContrastRatio(COLORREF a, COLORREF b)
{
	double l1 = RelLuminance(a);
	double l2 = RelLuminance(b);

	if (l1 < l2)
	{
		double temp = l1;
		l1 = l2;
		l2 = temp;
	}

	return (l1 + 0.05) / (l2 + 0.05);
}

// Returns whichever of first/second contrasts better against background.
inline COLORREF PickContrastOn(COLORREF background, COLORREF first, COLORREF second)
{
	return ContrastRatio(background, first) >= ContrastRatio(background, second) ? first : second;
}

// Nudges color until it clears 4.5:1 (WCAG AA for body text) against base,
// preserving the hue; gives up on black or white when stepping cannot get there.
inline COLORREF EnsureContrast(COLORREF color, COLORREF base)
{
	if (ContrastRatio(color, base) >= 4.5)
		return color;

	bool dark = RelLuminance(base) < 0.5;

	// Step away from the surface, not towards it: passing !dark moved the accent
	// towards the background, so every iteration lowered the ratio further and the
	// loop could never reach 4.5:1 -- it always fell through to the black/white
	// fallback and the accent hue was lost. Try both directions to be safe.
	for (int direction = 0; direction < 2; direction++)
	{
		for (int step = 1; step <= 9; step++)
		{
			double percent = step / 10.0;
			COLORREF adjusted = ShiftToward(color, direction == 0 ? dark : !dark, percent);
			if (ContrastRatio(adjusted, base) >= 4.5)
				return adjusted;
		}
	}

	return PickContrastOn(base, RGB(0, 0, 0), RGB(255, 255, 255));
}
