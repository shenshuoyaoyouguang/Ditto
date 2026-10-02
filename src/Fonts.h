#pragma once

// AppFonts: central font service (plan section 4.2).
// One family resolved once per process (CJK-capable fallback chain), a fixed
// size ramp scaled to the current dpi, and a helper to push the font onto all
// children of a dialog.

enum FontToken
{
	Font_Caption,    // 12 - badges, meta text
	Font_Body,       // 14 - default ui text
	Font_BodyStrong, // 14 semibold - titles, emphasized
	Font_Subtitle,   // 16 - section headers
	Font_Title,      // 20 - window titles

	Font_Count
};

class AppFonts
{
public:
	static AppFonts& Inst();

	// Rebuilds all ramp fonts at the given dpi (96 = logical sizes below).
	void Init(UINT dpi = 96, const CString& csFamilyOverride = _T(""));

	// Inits at the system dpi on first use, no-op afterwards.
	bool EnsureInitialized();

	CFont* Get(FontToken token);
	const LOGFONT& BodyLogFont() const { return m_lfBody; }
	CString ResolvedFamily() const { return m_csFamily; }
	UINT Dpi() const { return m_dpi; }

	// Sends WM_SETFONT to every child of pWnd (default: Body).
	static void ApplyToChildren(CWnd* pWnd, CFont* pFont = NULL);

private:
	AppFonts();

	CString ResolveFamily(const CString& csOverride);
	bool FamilyCoversTestGlyphs(const CString& csFamily);
	// isBody marks the single token that BodyLogFont() exposes; it must be
	// explicit because Font_Body and Font_BodyStrong share size 14.
	bool BuildFont(CFont& font, int size96, int weight, bool underline = false, bool isBody = false);

	CFont m_fonts[Font_Count];
	LOGFONT m_lfBody;
	CString m_csFamily;
	UINT m_dpi;
	bool m_initialized;
};
