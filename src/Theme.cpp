#include "stdafx.h"
#include ".\theme.h"
#include "..\Shared\TextConvert.h"
#include "Misc.h"
#include "Options.h"
#include "..\Shared\Tokenizer.h"
#include "CP_Main.h"

CTheme::CTheme(void)
{
	m_lFileVersion = 0;
	m_LastWriteTime = 0;
	m_lastTheme = _T("");

	LoadDefaults();
}

CTheme::~CTheme(void)
{
}


void CTheme::LoadDefaults()
{
	m_CaptionLeft = RGB(255, 255, 255);
	m_CaptionRight = RGB(204, 204, 204);

	m_Border = RGB(204, 204, 204);
	m_BorderTopMost = RGB(204, 204, 204);
	m_BorderNotConnected = RGB(204, 204, 204);

	m_CaptionLeftTopMost = RGB(255, 255, 255);
	m_CaptionRightTopMost = RGB(204, 204, 204);
	
	m_CaptionLeftNotConnected = RGB(255, 255, 255);
	m_CaptionRightNotConnected = RGB(255, 255, 0);

	m_CaptionTextColor = RGB(191, 191, 191);
	m_ListBoxOddRowsBG = RGB(255, 255, 255);
	m_ListBoxEvenRowsBG = RGB(243, 243, 243);
	m_ListBoxOddRowsText = RGB(0, 0, 0);
	m_ListBoxEvenRowsText = RGB(0, 0, 0);
	m_ListBoxSelectedBG = RGB(204, 204, 204);
	m_ListBoxSelectedNoFocusBG = RGB(204, 204, 204);
	m_ListBoxSelectedText = RGB(0, 0, 0);
	m_ListBoxSelectedNoFocusText = RGB(0, 0, 0);
	m_clipPastedColor = RGB(0, 255, 0);
	m_listSmallQuickPasteIndexColor = RGB(180, 180, 180);
	m_mainWindowBG = RGB(240, 240, 240);
	m_searchTextBoxFocusBG = RGB(255, 255, 255);
	m_searchTextBoxFocusText = RGB(0, 0, 0);
	m_searchTextBoxFocusBorder = RGB(255, 255, 255);
	m_searchTextHighlight = RGB(255, 0, 0);

	m_groupTreeBG = RGB(240, 240, 240);
	m_groupTreeText = RGB(127, 127, 127);

	m_descriptionWindowBG = RGB(240, 240, 240);// GetSysColor(COLOR_INFOBK);//RGB(240, 240, 240);//
	/*int r = GetRValue(m_descriptionWindowBG);
	int g = GetGValue(m_descriptionWindowBG);
	int b = GetBValue(m_descriptionWindowBG);*/

	m_descriptionWindowText = RGB(0, 0, 0);

	// Modern scrollbar defaults - rounded look
	m_scrollBarThumb = RGB(180, 180, 180);
	m_scrollBarThumbHover = RGB(140, 140, 140);
	m_scrollBarTrack = RGB(240, 240, 240);

	m_captionSize = 32;
	m_captionFontSize = 14;

	ResetTokenFlags();
	LoadTokenDefaults();
}

bool CTheme::Load(CString csTheme, bool bHeaderOnly, bool bCheckLastWriteTime)
{
	bool followWindows10Theme = false;
	if (csTheme.IsEmpty())
	{
		followWindows10Theme = true;

		// empty theme name follows the windows light/dark app setting
		csTheme = DarkAppWindows10Setting() ? _T("Fluent Dark") : _T("Fluent Light");
		Log(_T("Loading theme based on windows setting of dark mode for apps"));
	}

	if (csTheme.IsEmpty() || csTheme == _T("Ditto") || csTheme == _T("(Default)") || csTheme == _T("(Ditto)"))
	{
		LoadDefaults();

		if (followWindows10Theme)
		{
			LoadWindowsAccentColor();
		}

		FinalizeTokens();

		m_LastWriteTime = 0;
		m_lastTheme = _T("");

		Log(_T("Loading default ditto values for themes"));

		return false;
	}

	CString csPath = CGetSetOptions::GetPath(PATH_THEMES);
	csPath += csTheme;
	csPath += ".xml";

	__int64 LastWrite = GetLastWriteTime(csPath);

	if(bCheckLastWriteTime)
	{	
		if(m_lastTheme == csTheme &&
			LastWrite == m_LastWriteTime)
		{
			// The file is unchanged, but the "accent" alias resolves to the live
			// Windows accent: re-resolve it, otherwise changing the system accent
			// colour never reaches the UI.
			if (m_bFollowSystemAccent)
				RefreshSystemAccent();

			return true;
		}
	}

	LoadDefaults();

	Log(StrF(_T("Loading Theme %s"), csPath));

	TiXmlDocument doc;
	if(!doc.LoadFile(csPath.GetBuffer()))
	{
		m_csLastError.Format(_T("Error loading Theme %s - reason = %s"), csPath, doc.ErrorDesc());
		Log(m_csLastError);
		// Leave the cache clear: it is written before parsing, so a failure used
		// to poison it and every later Load(same theme, bCheckLastWriteTime=true)
		// short-circuited to "already loaded" -- the theme was never re-read even
		// after the file was fixed, and the UI kept rendering the defaults.
		m_lastTheme = _T("");
		m_LastWriteTime = 0;

		// The empty theme name resolves to Fluent Light / Fluent Dark, so a
		// missing (or not yet installed) file used to lose the system accent
		// along with it: LoadWindowsAccentColor() only ran on the success path.
		// Fall back to the full default palette, still following the accent.
		if (followWindows10Theme)
		{
			LoadWindowsAccentColor();
			FinalizeTokens();
		}

		return false;
	}

	TiXmlElement *ItemHeader = doc.FirstChildElement("Ditto_Theme_File");
	if(!ItemHeader)
	{
		m_csLastError.Format(_T("Error finding the section Ditto_Theme_File"));
		Log(m_csLastError);
		m_lastTheme = _T("");
		m_LastWriteTime = 0;
		return false;
	}

	CString csVersion = ItemHeader->Attribute("Version");
	m_lFileVersion = ATOI(csVersion);
	m_csAuthor = ItemHeader->Attribute("Author");
	m_csNotes = ItemHeader->Attribute("Notes");

	if(bHeaderOnly)
		return true;

	// Only commit the cache for a full load, and only after the file parsed. The
	// short-circuit above compares against this pair, so committing it during a
	// header-only scan (the options dialog enumerating the theme list) would make
	// the next Load(same theme, bCheckLastWriteTime=true) skip token parsing
	// entirely.
	m_LastWriteTime = LastWrite;
	m_lastTheme = csTheme;


	LoadColor(ItemHeader, "CaptionLeft", m_CaptionLeft);
	LoadColor(ItemHeader, "CaptionRight", m_CaptionRight);
	LoadColor(ItemHeader, "CaptionLeftTopMost", m_CaptionLeftTopMost);
	LoadColor(ItemHeader, "CaptionRightTopMost", m_CaptionRightTopMost);
	LoadColor(ItemHeader, "CaptionLeftNotConnected", m_CaptionLeftNotConnected);
	LoadColor(ItemHeader, "CaptionRightNotConnected", m_CaptionRightNotConnected);
	LoadColor(ItemHeader, "CaptionTextColor", m_CaptionTextColor);
	LoadColor(ItemHeader, "ListBoxOddRowsBG", m_ListBoxOddRowsBG);
	m_hasLegacyListBoxEvenRowsBG = LoadColor(ItemHeader, "ListBoxEvenRowsBG", m_ListBoxEvenRowsBG);
	m_hasLegacyListBoxOddRowsText = LoadColor(ItemHeader, "ListBoxOddRowsText", m_ListBoxOddRowsText);
	LoadColor(ItemHeader, "ListBoxEvenRowsText", m_ListBoxEvenRowsText);
	LoadColor(ItemHeader, "ListBoxSelectedBG", m_ListBoxSelectedBG);
	LoadColor(ItemHeader, "ListBoxSelectedNoFocusBG", m_ListBoxSelectedNoFocusBG);
	LoadColor(ItemHeader, "ListBoxSelectedText", m_ListBoxSelectedText);
	LoadColor(ItemHeader, "ListBoxSelectedNoFocusText", m_ListBoxSelectedNoFocusText);
	m_hasLegacyClipPastedColor = LoadColor(ItemHeader, "ClipPastedColor", m_clipPastedColor);
	m_hasLegacyMainWindowBG = LoadColor(ItemHeader, "MainWindowBG", m_mainWindowBG);
	// Was declared and reset but never loaded, so Indicator.Badge always derived
	// from Text.Secondary and the appendix-A rule was dead. No shipped theme has
	// this node, so wiring it up is a no-op for them and fixes custom themes.
	m_hasLegacySmallQuickPasteIndexColor = LoadColor(ItemHeader, "ListSmallQuickPasteIndexColor", m_listSmallQuickPasteIndexColor);
	m_hasLegacySearchTextBoxFocusBG = LoadColor(ItemHeader, "SearchTextBoxFocusBG", m_searchTextBoxFocusBG);
	LoadColor(ItemHeader, "SearchTextBoxFocusText", m_searchTextBoxFocusText);
	LoadColor(ItemHeader, "SearchTextBoxFocusBorder", m_searchTextBoxFocusBorder);
	LoadColor(ItemHeader, "SearchTextHighlight", m_searchTextHighlight);

	LoadColor(ItemHeader, "Border", m_Border);
	LoadColor(ItemHeader, "BorderTopMost", m_BorderTopMost);
	LoadColor(ItemHeader, "BorderNotConnected", m_BorderNotConnected);

	LoadColor(ItemHeader, "GroupTreeBG", m_groupTreeBG);
	LoadColor(ItemHeader, "GroupTreeText", m_groupTreeText);
	
	LoadInt(ItemHeader, "CaptionSize", m_captionSize);
	LoadInt(ItemHeader, "CaptionFontSize", m_captionFontSize);

	m_hasLegacyDescriptionWindowBG = LoadColor(ItemHeader, "DescriptionWindowBG", m_descriptionWindowBG);
	LoadColor(ItemHeader, "DescriptionWindowText", m_descriptionWindowText);

	// Modern scrollbar colors
	LoadColor(ItemHeader, "ScrollBarThumb", m_scrollBarThumb);
	LoadColor(ItemHeader, "ScrollBarThumbHover", m_scrollBarThumbHover);
	LoadColor(ItemHeader, "ScrollBarTrack", m_scrollBarTrack);

	LoadTokensFromXml(ItemHeader);

	if (followWindows10Theme)
	{
		LoadWindowsAccentColor();
	}

	FinalizeTokens();

	return true;
}

void CTheme::LoadWindowsAccentColor()
{
	DWORD accent = Windows10AccentColor();
	if (accent != -1)
	{
		//windows seems to be bgr, convert to rgb
		auto r = GetRValue(accent);
		auto g = GetGValue(accent);
		auto b = GetBValue(accent);

		m_clipPastedColor = RGB(b, g, r);
		m_searchTextBoxFocusBorder = m_clipPastedColor;
		m_searchTextHighlight = m_clipPastedColor;

	}
}

COLORREF HslToRgb(float h, float s, float l)
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

bool CTheme::LoadColor(TiXmlElement *pParent, CStringA csNode, COLORREF &Color)
{
	int intValue = 0;
	return LoadElement(pParent, csNode, Color, intValue);
}

bool CTheme::LoadInt(TiXmlElement *pParent, CStringA csNode, int &intValue)
{
	COLORREF colorValue = 0;
	return LoadElement(pParent, csNode, colorValue, intValue);
}

bool CTheme::LoadElement(TiXmlElement *pParent, CStringA csNode, COLORREF &Color, int &intValue)
{
	TiXmlElement *pColorNode = pParent->FirstChildElement(csNode);
	if(pColorNode == NULL)
	{
		m_csLastError.Format(_T("Theme Load, error loading Node = %s"), csNode);
		Log(m_csLastError);
		return false;
	}

	TiXmlNode *pColor = pColorNode->FirstChild();
	if(pColor == NULL)
	{
		m_csLastError.Format(_T("Theme Load, error getting node text for = %s"), csNode);
		Log(m_csLastError);
		return false;
	}
	
	CString csColor = pColor->Value();
	csColor.Trim();

	if (csColor.IsEmpty())
	{
		return false;
	}

	if (csColor.GetLength() > 4 && csColor.Left(4).CompareNoCase(_T("rgb(")) == 0)
	{
		CString values = csColor.Mid(4, csColor.GetLength() - 5);
		values.Trim();

		CTokenizer token(values, _T(", "));
		CString csR, csG, csB;

		token.Next(csR);
		token.Next(csG);
		token.Next(csB);

		csR.Trim();
		csG.Trim();
		csB.Trim();

		if (!csR.IsEmpty() && csG.IsEmpty() && csB.IsEmpty())
		{
			Color = ATOI(csR);
		}
		else if (!csR.IsEmpty() && !csG.IsEmpty() && !csB.IsEmpty())
		{
			Color = RGB(ATOI(csR), ATOI(csG), ATOI(csB));
		}
		else
		{
			m_csLastError.Format(_T("Theme Load, malformed/incomplete RGB value for Node = %s, Value = %s"), csNode, csColor);
			Log(m_csLastError);
			return false;
		}
	}
	else if (csColor.GetLength() > 4 && csColor.Left(4).CompareNoCase(_T("hsl(")) == 0)
	{
		CString values = csColor.Mid(4, csColor.GetLength() - 5);
		values.Trim();

		CTokenizer token(values, _T(", %"));
		CString csH, csS, csL;

		token.Next(csH);
		token.Next(csS);
		token.Next(csL);

		csH.Trim();
		csS.Trim();
		csL.Trim();

		if (!csH.IsEmpty() && !csS.IsEmpty() && !csL.IsEmpty())
		{
			float h = (float)_tstof(csH);
			float s = (float)_tstof(csS);
			float l = (float)_tstof(csL);

			s = max(0.0f, min(100.0f, s)) / 100.0f;
			l = max(0.0f, min(100.0f, l)) / 100.0f;

			Color = HslToRgb(h, s, l);
		}
		else
		{
			m_csLastError.Format(_T("Theme Load, malformed/incomplete HSL value for Node = %s, Value = %s"), csNode, csColor);
			Log(m_csLastError);
			return false;
		}
	}
	else if (csColor.GetAt(0) == _T('#') && csColor.GetLength() == 7)
	{
		long r = _tcstol(csColor.Mid(1, 2), NULL, 16);
		long g = _tcstol(csColor.Mid(3, 2), NULL, 16);
		long b = _tcstol(csColor.Mid(5, 2), NULL, 16);

		Color = RGB(r, g, b);
	}
	else
	{
		// ATOI() returned 0 for anything it could not parse, and the function still
		// claimed success: a typo or a #FFF shorthand silently became pure black and
		// the caller's "loaded" flag suppressed the light/dark default as well.
		LPCTSTR start = (LPCTSTR)csColor;
		LPTSTR end = NULL;
		long value = _tcstol(start, &end, 10);

		if (end == start || (end != NULL && *end != _T('\0')))
		{
			m_csLastError.Format(_T("Theme Load, unrecognized color value for Node = %s, Value = %s"), csNode, csColor);
			Log(m_csLastError);
			return false;
		}

		intValue = (int)value;
		Color = (COLORREF)intValue;
	}

	return true;
}

// ---- v4 semantic tokens (docs/design/ui-fluent-redesign-plan.md section 3) ----
// Defaults follow the windows light/dark app setting; explicit xml nodes win;
// anything still missing is derived from the legacy v3 fields so the 12 shipped
// theme files keep rendering exactly as before.

void CTheme::ResetTokenFlags()
{
	m_hasSurfaceBase = false;
	m_hasSurfaceElevated = false;
	m_hasSurfaceRowAlt = false;
	m_hasTextPrimary = false;
	m_hasTextSecondary = false;
	m_hasTextDisabled = false;
	m_hasTextOnAccent = false;
	m_hasAccentDefault = false;
	m_hasAccentSubtle = false;
	m_hasAccentText = false;
	m_hasStrokeCard = false;
	m_hasStrokeDivider = false;
	m_hasControlFill = false;
	m_hasControlHover = false;
	m_hasControlPressed = false;
	m_hasControlDisabledBG = false;
	m_hasStateHover = false;
	m_hasStateSelectedBG = false;
	m_hasStateSelectedText = false;
	m_hasIndicatorBadge = false;
	m_hasFontFamily = false;
	m_hasLegacyMainWindowBG = false;
	m_hasLegacySearchTextBoxFocusBG = false;
	m_hasLegacyListBoxEvenRowsBG = false;
	m_hasLegacyListBoxOddRowsText = false;
	m_hasLegacyClipPastedColor = false;
	m_hasLegacySmallQuickPasteIndexColor = false;
	m_hasLegacyDescriptionWindowBG = false;
	m_bFollowSystemAccent = false;
	m_bParsedThemeXml = false;
}

void CTheme::LoadTokenDefaults()
{
	bool dark = DarkAppWindows10Setting() ? true : false;

	if (dark)
	{
		m_surfaceBase = RGB(0x20, 0x20, 0x20);
		m_surfaceElevated = RGB(0x2B, 0x2B, 0x2B);
		m_surfaceRowAlt = RGB(0x26, 0x26, 0x26);
		m_textPrimary = RGB(0xFF, 0xFF, 0xFF);
		m_textSecondary = RGB(0xC8, 0xC8, 0xC8);
		m_textDisabled = RGB(0x71, 0x71, 0x71);
		m_textOnAccent = RGB(0x00, 0x00, 0x00);
		m_accentDefault = RGB(0x4C, 0xC2, 0xFF);
		m_strokeCard = RGB(0x3D, 0x3D, 0x3D);
		m_strokeDivider = RGB(0x33, 0x33, 0x33);
		m_controlFill = RGB(0x2D, 0x2D, 0x2D);
		m_controlHover = RGB(0x38, 0x38, 0x38);
		m_controlPressed = RGB(0x3A, 0x3A, 0x3A);
		m_controlDisabledBG = RGB(0x29, 0x29, 0x29);
		m_stateSelectedText = RGB(0xFF, 0xFF, 0xFF);
		m_indicatorBadge = RGB(0xC8, 0xC8, 0xC8);
	}
	else
	{
		m_surfaceBase = RGB(0xF3, 0xF3, 0xF3);
		m_surfaceElevated = RGB(0xFF, 0xFF, 0xFF);
		m_surfaceRowAlt = RGB(0xF9, 0xF9, 0xF9);
		m_textPrimary = RGB(0x1A, 0x1A, 0x1A);
		m_textSecondary = RGB(0x61, 0x61, 0x61);
		m_textDisabled = RGB(0x9D, 0x9D, 0x9D);
		m_textOnAccent = RGB(0xFF, 0xFF, 0xFF);
		m_accentDefault = RGB(0x00, 0x5F, 0xB8);
		m_strokeCard = RGB(0xE5, 0xE5, 0xE5);
		m_strokeDivider = RGB(0xED, 0xED, 0xED);
		m_controlFill = RGB(0xFB, 0xFB, 0xFB);
		m_controlHover = RGB(0xF0, 0xF0, 0xF0);
		m_controlPressed = RGB(0xED, 0xED, 0xED);
		m_controlDisabledBG = RGB(0xF5, 0xF5, 0xF5);
		m_stateSelectedText = RGB(0x1A, 0x1A, 0x1A);
		m_indicatorBadge = RGB(0x61, 0x61, 0x61);
	}

	m_stateHover = BlendOver(m_surfaceRowAlt, dark ? RGB(255, 255, 255) : RGB(0, 0, 0), 15);
	m_accentSubtle = BlendOver(m_surfaceBase, m_accentDefault, 31);
	m_stateSelectedBG = m_accentSubtle;
	m_accentText = m_accentDefault;

	m_radiusControl = 4;
	m_rowHeightCompact = 30;
	m_rowHeightComfortable = 44;
	m_csFontFamily = _T("auto");

	m_bDarkTheme = RelLuminance(m_surfaceBase) < 0.5;
}

void CTheme::LoadTokensFromXml(TiXmlElement *pParent)
{
	m_bParsedThemeXml = true;

	auto loadToken = [&](const char *csNode, COLORREF &target, bool &has)
	{
		if (pParent->FirstChildElement(csNode) != NULL)
			has = LoadColor(pParent, csNode, target);
	};

	loadToken("Surface_Base", m_surfaceBase, m_hasSurfaceBase);
	loadToken("Surface_Elevated", m_surfaceElevated, m_hasSurfaceElevated);
	loadToken("Surface_RowAlt", m_surfaceRowAlt, m_hasSurfaceRowAlt);
	loadToken("Text_Primary", m_textPrimary, m_hasTextPrimary);
	loadToken("Text_Secondary", m_textSecondary, m_hasTextSecondary);
	loadToken("Text_Disabled", m_textDisabled, m_hasTextDisabled);
	loadToken("Stroke_Card", m_strokeCard, m_hasStrokeCard);
	loadToken("Stroke_Divider", m_strokeDivider, m_hasStrokeDivider);
	loadToken("Control_Fill", m_controlFill, m_hasControlFill);
	loadToken("Control_Hover", m_controlHover, m_hasControlHover);
	loadToken("Control_Pressed", m_controlPressed, m_hasControlPressed);
	loadToken("Control_DisabledBG", m_controlDisabledBG, m_hasControlDisabledBG);
	loadToken("State_Hover", m_stateHover, m_hasStateHover);
	loadToken("State_SelectedBG", m_stateSelectedBG, m_hasStateSelectedBG);
	loadToken("State_SelectedText", m_stateSelectedText, m_hasStateSelectedText);
	loadToken("Text_OnAccent", m_textOnAccent, m_hasTextOnAccent);
	loadToken("Accent_Subtle", m_accentSubtle, m_hasAccentSubtle);
	loadToken("Accent_Text", m_accentText, m_hasAccentText);
	loadToken("Indicator_Badge", m_indicatorBadge, m_hasIndicatorBadge);

	if (pParent->FirstChildElement("Accent_Default") != NULL)
	{
		TiXmlNode *pValue = pParent->FirstChildElement("Accent_Default")->FirstChild();
		CString csValue = pValue ? pValue->Value() : "";
		csValue.Trim();

		if (csValue.CompareNoCase(_T("accent")) == 0)
		{
			// Remember the alias: the value has to be re-resolved from the live
			// Windows setting, not just when the xml happens to be re-parsed.
			m_bFollowSystemAccent = true;
			m_hasAccentDefault = true;
			RefreshSystemAccent();
		}
		else
		{
			m_bFollowSystemAccent = false;
			m_hasAccentDefault = LoadColor(pParent, "Accent_Default", m_accentDefault);
		}
	}

	if (pParent->FirstChildElement("Radius_Control") != NULL)
		LoadInt(pParent, "Radius_Control", m_radiusControl);
	if (pParent->FirstChildElement("RowHeight_Compact") != NULL)
		LoadInt(pParent, "RowHeight_Compact", m_rowHeightCompact);
	if (pParent->FirstChildElement("RowHeight_Comfortable") != NULL)
		LoadInt(pParent, "RowHeight_Comfortable", m_rowHeightComfortable);

	TiXmlElement *pFontNode = pParent->FirstChildElement("Font_Family");
	if (pFontNode != NULL)
	{
		TiXmlNode *pValue = pFontNode->FirstChild();
		if (pValue != NULL)
		{
			m_csFontFamily = pValue->Value();
			m_csFontFamily.Trim();
			m_hasFontFamily = true;
		}
	}
}

void CTheme::FinalizeTokens()
{
	if (m_bParsedThemeXml)
		DeriveTokensFromLegacy();
	else if (!m_hasAccentDefault)
		m_accentDefault = m_clipPastedColor; // built-in palette: LoadWindowsAccentColor may have stored the system accent here

	m_bDarkTheme = RelLuminance(m_surfaceBase) < 0.5;

	RefreshAccentDerived();
}

void CTheme::DeriveTokensFromLegacy()
{
	// Settle Surface.Base first: the polarity below drives the accent, hover and
	// pressed directions, and for a legacy theme the base comes from the theme's
	// own MainWindowBG -- not from the OS light/dark default that LoadTokenDefaults
	// seeded. Judging it before this line inverted every derived token whenever
	// the Windows app mode disagreed with the theme (e.g. Windows light + a dark
	// theme), leaving the accent at ~1.9:1 against the surface.
	if (!m_hasSurfaceBase && m_hasLegacyMainWindowBG)
		m_surfaceBase = m_mainWindowBG;

	bool dark = RelLuminance(m_surfaceBase) < 0.5;

	if (!m_hasSurfaceElevated)
	{
		// Appendix A maps Surface.Elevated to DescriptionWindowBG; fall back to it
		// before synthesising a shift from the base colour.
		if (m_hasLegacySearchTextBoxFocusBG)
			m_surfaceElevated = m_searchTextBoxFocusBG;
		else if (m_hasLegacyDescriptionWindowBG)
			m_surfaceElevated = m_descriptionWindowBG;
		else
			m_surfaceElevated = ShiftToward(m_surfaceBase, !dark, 0.04);
	}

	if (!m_hasSurfaceRowAlt)
		m_surfaceRowAlt = m_hasLegacyListBoxEvenRowsBG ? m_ListBoxEvenRowsBG
			: ShiftToward(m_surfaceBase, !dark, 0.01);

	// Guard on the legacy flag like the other legacy fallbacks do: LoadElement()
	// returns false for both a missing node and an empty one, in which case
	// m_ListBoxOddRowsText is still the hardcoded RGB(0,0,0) from LoadDefaults()
	// and a dark theme would render black on black.
	if (!m_hasTextPrimary && m_hasLegacyListBoxOddRowsText)
		m_textPrimary = m_ListBoxOddRowsText;
	else if (!m_hasTextPrimary)
		m_textPrimary = (RelLuminance(m_surfaceBase) < 0.5) ? RGB(255, 255, 255) : RGB(0, 0, 0);

	if (!m_hasTextSecondary)
		m_textSecondary = BlendOver(m_surfaceBase, m_textPrimary, 158);

	if (!m_hasTextDisabled)
		m_textDisabled = BlendOver(m_surfaceBase, m_textSecondary, 153);

	if (!m_hasStrokeCard)
	{
		m_strokeCard = RGB((GetRValue(m_surfaceBase) + GetRValue(m_surfaceElevated)) / 2,
			(GetGValue(m_surfaceBase) + GetGValue(m_surfaceElevated)) / 2,
			(GetBValue(m_surfaceBase) + GetBValue(m_surfaceElevated)) / 2);
	}

	if (!m_hasStrokeDivider)
		m_strokeDivider = BlendOver(m_strokeCard, m_surfaceBase, 64);

	if (!m_hasControlFill)
		m_controlFill = m_surfaceElevated;

	if (!m_hasControlHover)
		m_controlHover = ShiftToward(m_surfaceBase, !dark, 0.04);

	if (!m_hasControlPressed)
		m_controlPressed = ShiftToward(m_controlHover, !dark, 0.02);

	if (!m_hasControlDisabledBG)
		m_controlDisabledBG = ShiftToward(m_surfaceBase, !dark, 0.015);

	if (!m_hasIndicatorBadge)
		m_indicatorBadge = m_hasLegacySmallQuickPasteIndexColor ? m_listSmallQuickPasteIndexColor : m_textSecondary;

	if (!m_hasStateSelectedText)
		m_stateSelectedText = m_textPrimary;

	if (!m_hasAccentDefault)
	{
		if (m_hasLegacyClipPastedColor)
			m_accentDefault = m_clipPastedColor;
		else
			m_accentDefault = dark ? RGB(0x4C, 0xC2, 0xFF) : RGB(0x00, 0x5F, 0xB8);
	}
}

void CTheme::RefreshSystemAccent()
{
	DWORD accent = Windows10AccentColor();
	if (accent != -1)
	{
		// windows reports bgr, convert to rgb
		m_accentDefault = RGB(GetBValue(accent), GetGValue(accent), GetRValue(accent));
	}
	else
	{
		// Fall back on the theme's own polarity, not the Windows app setting: a
		// dark theme on a light-mode machine must still get the light accent.
		m_accentDefault = (RelLuminance(m_surfaceBase) < 0.5) ? RGB(0x4C, 0xC2, 0xFF) : RGB(0x00, 0x5F, 0xB8);
	}

	RefreshAccentDerived();
}

void CTheme::RefreshAccentDerived()
{
	if (!m_hasStateHover)
		m_stateHover = BlendOver(m_surfaceRowAlt, m_bDarkTheme ? RGB(255, 255, 255) : RGB(0, 0, 0), 15);

	if (!m_hasAccentSubtle)
		m_accentSubtle = BlendOver(m_surfaceBase, m_accentDefault, 31);

	if (!m_hasStateSelectedBG)
	{
		m_stateSelectedBG = m_accentSubtle;

		// The selection pill has to be distinguishable from the row it sits on, and
		// rows are not all the same colour: QListCtrl fills them with
		// ListBoxOddRowsBG / ListBoxEvenRowsBG, which in Selenized Dark are
		// RGB(16,60,72) and RGB(23,73,86). Accent.Subtle mixes the accent in at
		// only 12%, so the pill measured 1.05:1 against the even row. Walk the mix
		// towards the accent until it separates from the worst of those three
		// surfaces, with a hard cap and a no-progress guard.
		const double kMinSeparation = 1.25;
		for (int step = 0; step < 12; step++)
		{
			// Note: the legacy members, not just m_surfaceRowAlt -- the list draws
			// the legacy pair, and a v3 theme's odd row is unrelated to RowAlt.
			bool bSeparated =
				ContrastRatio(m_stateSelectedBG, m_surfaceBase) >= kMinSeparation &&
				ContrastRatio(m_stateSelectedBG, m_ListBoxOddRowsBG) >= kMinSeparation &&
				ContrastRatio(m_stateSelectedBG, m_ListBoxEvenRowsBG) >= kMinSeparation;
			if (bSeparated)
				break;

			COLORREF previous = m_stateSelectedBG;
			m_stateSelectedBG = BlendOver(m_stateSelectedBG, m_accentDefault, 24);
			if (m_stateSelectedBG == previous)
				break;
		}
	}

	if (!m_hasTextOnAccent)
		m_textOnAccent = PickContrastOn(m_accentDefault, RGB(255, 255, 255), RGB(0, 0, 0));

	if (!m_hasAccentText)
		m_accentText = EnsureContrastOnBase(m_accentDefault);
}

COLORREF CTheme::BlendOver(COLORREF under, COLORREF over, int alpha)
{
	auto blend = [alpha](int u, int o) -> BYTE
	{
		int value = u + MulDiv(o - u, alpha, 255);
		return (BYTE)max(0, min(255, value));
	};

	return RGB(blend(GetRValue(under), GetRValue(over)),
		blend(GetGValue(under), GetGValue(over)),
		blend(GetBValue(under), GetBValue(over)));
}

COLORREF CTheme::ShiftToward(COLORREF color, bool towardWhite, double percent)
{
	int target = towardWhite ? 255 : 0;

	auto shift = [target, percent](int c) -> BYTE
	{
		int value = (int)(c + (target - c) * percent + 0.5);
		return (BYTE)max(0, min(255, value));
	};

	return RGB(shift(GetRValue(color)), shift(GetGValue(color)), shift(GetBValue(color)));
}

double CTheme::RelLuminance(COLORREF color)
{
	auto linear = [](int channel) -> double
	{
		double c = channel / 255.0;
		return c <= 0.03928 ? c / 12.92 : pow((c + 0.055) / 1.055, 2.4);
	};

	return 0.2126 * linear(GetRValue(color)) + 0.7152 * linear(GetGValue(color)) + 0.0722 * linear(GetBValue(color));
}

double CTheme::ContrastRatio(COLORREF a, COLORREF b)
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

COLORREF CTheme::PickContrastOn(COLORREF background, COLORREF first, COLORREF second)
{
	return ContrastRatio(background, first) >= ContrastRatio(background, second) ? first : second;
}

COLORREF CTheme::EnsureContrastOnBase(COLORREF color)
{
	if (ContrastRatio(color, m_surfaceBase) >= 4.5)
		return color;

	bool dark = RelLuminance(m_surfaceBase) < 0.5;

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
			if (ContrastRatio(adjusted, m_surfaceBase) >= 4.5)
				return adjusted;
		}
	}

	return PickContrastOn(m_surfaceBase, RGB(0, 0, 0), RGB(255, 255, 255));
}