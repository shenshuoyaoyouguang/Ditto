#pragma once

#include "CGdiPlusBitmap.h"
#include "DPI.h"

class CGdiImageDrawer
{
public:
	CGdiImageDrawer();
	~CGdiImageDrawer();

	BOOL LoadStdImage(UINT id, LPCTSTR pType);
	BOOL LoadStdImageDPI(int dpi, UINT id96, UINT id120, UINT id144, UINT id168, UINT id192, LPCTSTR pType, UINT id225 = 0, UINT id250 = 0, UINT id275 = 0, UINT id300 = 0, UINT id325 = 0, UINT id350 = 0);
	void Draw(CDC* pScreenDC, CDPI &dpi, CWnd *pWnd, int posX, int posY, bool mouseHover, bool mouseDown, int forceWidth = INT_MAX, int forceHeight = INT_MAX);
	void Draw(CDC* pScreenDC, CDPI &dpi, CWnd *pWnd, CRect rc, bool mouseHover, bool mouseDown);

	// like Draw(posX, posY) but remaps the black icon pixels to tintColor,
	// keeping alpha - used to theme monochrome glyphs (plan section 5.9)
	void DrawTinted(CDC* pScreenDC, CDPI &dpi, CWnd *pWnd, int posX, int posY, bool mouseHover, bool mouseDown, COLORREF tintColor);
	BOOL LoadRaw(unsigned char* bitmapData, int imageSize);

	// Whether a bitmap is actually loaded. Every entry point below dereferences
	// m_pStdImage->m_pBitmap, and a failed PNG load (missing resource, GDI+
	// failure) leaves it NULL -- the crash then surfaced seconds later inside
	// an unrelated repaint.
	bool HasImage() const;

	UINT ImageWidth();
	UINT ImageHeight();

	void Reset();

protected:
	CGdiPlusBitmapResource* m_pStdImage;
	//CDC*	m_pCurBtn;		// current pointer to one of the above
	//CDC		m_dcStd;		// standard button

	//CDC m_dcBk;
};

