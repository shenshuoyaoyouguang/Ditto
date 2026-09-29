#pragma once

#include "DPI.h"

// Line style icons drawn for each row of the quick paste list (ui-redesign).
// Icons are defined on a 16 unit design grid and rendered with GDI+ so they
// stay crisp at any DPI without shipping per resolution bitmaps.
enum class ClipRowIcon
{
	Text = 0,
	Code = 1,
	Link = 2,
	Image = 3,
	File = 4,
	RichText = 5,
	Email = 6,
	Folder = 7,
};

class CRowIcons
{
public:
	// Draws the given icon centered in rc, stroked with color.
	static void Draw(HDC hdc, CDPI &dpi, ClipRowIcon icon, const CRect &rc, COLORREF color);

	// Content based classification of a clip description.
	static ClipRowIcon Classify(const CString &csDesc);

protected:
	static void DrawIcon(Gdiplus::Graphics &graphics, ClipRowIcon icon, Gdiplus::Pen &pen);
};
