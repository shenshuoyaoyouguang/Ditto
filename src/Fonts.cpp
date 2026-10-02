#include "stdafx.h"
#include "Fonts.h"
#include "Options.h"

AppFonts& AppFonts::Inst()
{
	static AppFonts fonts;
	return fonts;
}

AppFonts::AppFonts()
{
	ZeroMemory(&m_lfBody, sizeof(m_lfBody));
	m_dpi = 96;
	m_initialized = false;
}

void AppFonts::Init(UINT dpi, const CString& csFamilyOverride)
{
	if (dpi == 0)
		dpi = 96;

	CString csFamily = ResolveFamily(csFamilyOverride);

	// BuildFont() starts with DeleteObject(), and ApplyToChildren() handed the
	// raw HFONT to child windows (WM_SETFONT). Rebuilding for unchanged inputs
	// would leave those windows holding a deleted font, so only rebuild when
	// the dpi or the resolved family actually changed.
	if (m_initialized && m_dpi == dpi && m_csFamily == csFamily)
		return;

	m_dpi = dpi;
	m_csFamily = csFamily;
	m_initialized = true;

	BuildFont(m_fonts[Font_Caption], 12, FW_NORMAL);
	BuildFont(m_fonts[Font_Body], 14, FW_NORMAL, false, true);
	BuildFont(m_fonts[Font_BodyStrong], 14, FW_SEMIBOLD);
	BuildFont(m_fonts[Font_Subtitle], 16, FW_NORMAL);
	BuildFont(m_fonts[Font_Title], 20, FW_NORMAL);
}

bool AppFonts::EnsureInitialized()
{
	if (m_fonts[Font_Body].GetSafeHandle() != NULL)
		return true;

	HDC hdc = GetDC(NULL);
	UINT dpi = 96;
	if (hdc != NULL)
	{
		dpi = (UINT)GetDeviceCaps(hdc, LOGPIXELSX);
		ReleaseDC(NULL, hdc);
	}

	Init(dpi, CGetSetOptions::m_Theme.FontFamily());
	return m_fonts[Font_Body].GetSafeHandle() != NULL;
}

CFont* AppFonts::Get(FontToken token)
{
	if (token < 0 || token >= Font_Count)
		return NULL;

	if (m_fonts[Font_Body].GetSafeHandle() == NULL)
		EnsureInitialized();

	// If only this token failed to build (e.g. GDI exhaustion) fall back to
	// Body so callers never select an invalid HFONT into a DC.
	if (m_fonts[token].GetSafeHandle() == NULL && token != Font_Body)
		return &m_fonts[Font_Body];

	return &m_fonts[token];
}

bool AppFonts::BuildFont(CFont& font, int size96, int weight, bool underline, bool isBody)
{
	font.DeleteObject();

	LOGFONT lf;
	ZeroMemory(&lf, sizeof(lf));
	lf.lfHeight = -MulDiv(size96, m_dpi, 96);
	lf.lfWeight = weight;
	lf.lfUnderline = underline ? TRUE : FALSE;
	lf.lfCharSet = DEFAULT_CHARSET;
	lf.lfOutPrecision = OUT_TT_PRECIS;
	lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	lf.lfQuality = CLEARTYPE_QUALITY;
	lf.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
	// m_csFamily comes from the theme xml and can be arbitrarily long; copy
	// with truncation instead of overrunning lfFaceName[LF_FACESIZE].
	_tcsncpy_s(lf.lfFaceName, LF_FACESIZE, (LPCTSTR)m_csFamily, _TRUNCATE);

	// Font_Body and Font_BodyStrong are both size 14, so the size alone cannot
	// identify which one BodyLogFont() must expose -- the caller says so.
	if (isBody)
		m_lfBody = lf;

	return font.CreateFontIndirect(&lf) ? true : false;
}

// The chosen family must cover the latin and cjk test glyphs with its own
// outlines: GDI font linking would render the cjk text, but metrics and the
// semibold weight would come from a different face, so we require a real match.
// With this rule the chain deterministically lands on Microsoft YaHei UI on
// any os that has it (Segoe UI* carries no CJK outlines).
CString AppFonts::ResolveFamily(const CString& csOverride)
{
	if (!csOverride.IsEmpty() && csOverride.CompareNoCase(_T("auto")) != 0)
	{
		if (FamilyCoversTestGlyphs(csOverride))
			return csOverride;
	}

	static LPCTSTR candidates[] =
	{
		_T("Segoe UI Variable"),
		_T("Segoe UI"),
		_T("Microsoft YaHei UI"),
		_T("Microsoft YaHei"),
	};

	for (int i = 0; i < sizeof(candidates) / sizeof(candidates[0]); i++)
	{
		if (FamilyCoversTestGlyphs(candidates[i]))
			return candidates[i];
	}

	return _T("Microsoft YaHei UI");
}

bool AppFonts::FamilyCoversTestGlyphs(const CString& csFamily)
{
	// latin base, latin extended and a cjk ideograph
	static const wchar_t testGlyphs[] = { 0x0041, 0x00E9, 0x4E2D };

	HDC hdc = GetDC(NULL);
	if (hdc == NULL)
		return false;

	HFONT font = CreateFont(-16, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
		OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
		DEFAULT_PITCH | FF_SWISS, csFamily);
	if (font == NULL)
	{
		ReleaseDC(NULL, hdc);
		return false;
	}

	HGDIOBJ oldFont = SelectObject(hdc, font);

	bool covered = true;
	for (int i = 0; i < sizeof(testGlyphs) / sizeof(testGlyphs[0]); i++)
	{
		WORD glyph = 0xFFFF;
		if (GetGlyphIndicesW(hdc, &testGlyphs[i], 1, &glyph, GGI_MARK_NONEXISTING_GLYPHS) == GDI_ERROR ||
			glyph == 0xFFFF)
		{
			covered = false;
			break;
		}
	}

	SelectObject(hdc, oldFont);
	DeleteObject(font);
	ReleaseDC(NULL, hdc);

	return covered;
}

struct FontEnumContext
{
	HFONT hFont;
};

static BOOL CALLBACK FontEnumProc(HWND hWnd, LPARAM lParam)
{
	FontEnumContext* pContext = (FontEnumContext*)lParam;
	::SendMessage(hWnd, WM_SETFONT, (WPARAM)pContext->hFont, TRUE);
	return TRUE;
}

void AppFonts::ApplyToChildren(CWnd* pWnd, CFont* pFont)
{
	if (pWnd == NULL || pWnd->GetSafeHwnd() == NULL)
		return;

	CFont* font = pFont != NULL ? pFont : Inst().Get(Font_Body);
	if (font == NULL || font->GetSafeHandle() == NULL)
		return;

	FontEnumContext context;
	context.hFont = (HFONT)font->GetSafeHandle();

	::EnumChildWindows(pWnd->GetSafeHwnd(), FontEnumProc, (LPARAM)&context);
}
