#pragma once

#include "tinyxml\Tinyxml.h"
#include "tinyxml\tinystr.h"

class CTheme
{
public:
	CTheme(void);
	~CTheme(void);

	bool Load(CString csTheme, bool bHeaderOnly = false, bool bCheckLastWriteTime = false);	

	COLORREF CaptionLeft() const { return m_CaptionLeft; }
	COLORREF CaptionRight() const { return m_CaptionRight; }
	COLORREF Border() const { return m_Border; }
	COLORREF BorderTopMost() const { return m_BorderTopMost; }
	COLORREF BorderNotConnected() const { return m_BorderNotConnected; }
	COLORREF CaptionLeftTopMost() const { return m_CaptionLeftTopMost; }
	COLORREF CaptionRightTopMost() const { return m_CaptionRightTopMost; }
	COLORREF CaptionLeftNotConnected() const { return m_CaptionLeftNotConnected; }
	COLORREF CaptionRightNotConnected() const { return m_CaptionRightNotConnected; }
	COLORREF CaptionTextColor() const { return m_CaptionTextColor; }
	
	COLORREF ListBoxOddRowsBG() const { return m_ListBoxOddRowsBG; }
	COLORREF ListBoxEvenRowsBG() const { return m_ListBoxEvenRowsBG; }
	COLORREF ListBoxOddRowsText() const { return m_ListBoxOddRowsText; }
	COLORREF ListBoxEvenRowsText() const { return m_ListBoxEvenRowsText; }
	COLORREF ListBoxSelectedBG() const { return m_ListBoxSelectedBG; }
	COLORREF ListBoxSelectedNoFocusBG() const { return m_ListBoxSelectedNoFocusBG; }
	COLORREF ListBoxSelectedText() const { return m_ListBoxSelectedText; }
	COLORREF ListBoxSelectedNoFocusText() const { return m_ListBoxSelectedNoFocusText; }
	COLORREF ClipPastedColor() const { return m_clipPastedColor; }

	COLORREF ListSmallQuickPasteIndexColor() const { return m_listSmallQuickPasteIndexColor;  }
	COLORREF MainWindowBG() const { return m_mainWindowBG; }
	COLORREF SearchTextBoxFocusBG() const { return m_searchTextBoxFocusBG; }
	COLORREF SearchTextBoxFocusText() const { return m_searchTextBoxFocusText; }
	COLORREF SearchTextBoxFocusBorder() const { return m_searchTextBoxFocusBorder; }
	COLORREF SearchTextHighlight() const { return m_searchTextHighlight; }

	COLORREF GroupTreeBG() const { return m_groupTreeBG; }
	COLORREF GroupTreeText() const { return m_groupTreeText; }

	int GetCaptionSize() const { return m_captionSize; }
	int GetCaptionFontSize() const { return m_captionFontSize; }

	COLORREF DescriptionWindowBG() const { return m_descriptionWindowBG; }
	COLORREF DescriptionWindowText() const { return m_descriptionWindowText; }

	// Modern scrollbar colors
	COLORREF ScrollBarThumb() const { return m_scrollBarThumb; }
	COLORREF ScrollBarThumbHover() const { return m_scrollBarThumbHover; }
	COLORREF ScrollBarTrack() const { return m_scrollBarTrack; }

	// v4 semantic tokens (see docs/design/ui-fluent-redesign-plan.md section 3)
	COLORREF SurfaceBase() const { return m_surfaceBase; }
	COLORREF SurfaceElevated() const { return m_surfaceElevated; }
	COLORREF SurfaceRowAlt() const { return m_surfaceRowAlt; }
	COLORREF TextPrimary() const { return m_textPrimary; }
	COLORREF TextSecondary() const { return m_textSecondary; }
	COLORREF TextDisabled() const { return m_textDisabled; }
	COLORREF TextOnAccent() const { return m_textOnAccent; }
	COLORREF AccentDefault() const { return m_accentDefault; }
	COLORREF AccentSubtle() const { return m_accentSubtle; }
	COLORREF AccentText() const { return m_accentText; }
	COLORREF StrokeCard() const { return m_strokeCard; }
	COLORREF StrokeDivider() const { return m_strokeDivider; }
	COLORREF ControlFill() const { return m_controlFill; }
	COLORREF ControlHover() const { return m_controlHover; }
	COLORREF ControlPressed() const { return m_controlPressed; }
	COLORREF ControlDisabledBG() const { return m_controlDisabledBG; }
	COLORREF StateHover() const { return m_stateHover; }
	COLORREF StateSelectedBG() const { return m_stateSelectedBG; }
	COLORREF StateSelectedText() const { return m_stateSelectedText; }
	COLORREF IndicatorBadge() const { return m_indicatorBadge; }
	int RadiusControl() const { return m_radiusControl; }
	int RowHeightCompact() const { return m_rowHeightCompact; }
	int RowHeightComfortable() const { return m_rowHeightComfortable; }
	CString FontFamily() const { return m_csFontFamily; }
	bool IsDarkTheme() const { return m_bDarkTheme; }

	CString Notes() const { return m_csNotes; }
	CString Author() const { return m_csAuthor; }
	long FileVersion() const { return m_lFileVersion; }

	CString LastError() const { return m_csLastError; }

protected:
	bool LoadElement(TiXmlElement *pParent, CStringA csNode, COLORREF &Color, int &intValue);

	bool LoadInt(TiXmlElement *pParent, CStringA csNode, int &intValue);
	bool LoadColor(TiXmlElement *pParent, CStringA csNode, COLORREF &Color);
	void LoadWindowsAccentColor();

	// v4 token handling
	void LoadTokenDefaults();
	void LoadTokensFromXml(TiXmlElement *pParent);
	void FinalizeTokens();
	void DeriveTokensFromLegacy();
	void RefreshAccentDerived();
	void ResetTokenFlags();

	static COLORREF BlendOver(COLORREF under, COLORREF over, int alpha);
	static COLORREF ShiftToward(COLORREF color, bool towardWhite, double percent);
	static double RelLuminance(COLORREF color);
	static double ContrastRatio(COLORREF a, COLORREF b);
	static COLORREF PickContrastOn(COLORREF background, COLORREF first, COLORREF second);
	COLORREF EnsureContrastOnBase(COLORREF color);

protected:
	COLORREF m_CaptionLeft;
	COLORREF m_CaptionRight;
	COLORREF m_CaptionLeftTopMost;
	COLORREF m_CaptionRightTopMost;
	COLORREF m_CaptionLeftNotConnected;
	COLORREF m_CaptionRightNotConnected;
	COLORREF m_CaptionTextColor;

	COLORREF m_ListBoxOddRowsBG;
	COLORREF m_ListBoxEvenRowsBG;
	COLORREF m_ListBoxOddRowsText;
	COLORREF m_ListBoxEvenRowsText;
	COLORREF m_ListBoxSelectedBG;
	COLORREF m_ListBoxSelectedNoFocusBG;
	COLORREF m_ListBoxSelectedText;
	COLORREF m_ListBoxSelectedNoFocusText;	
	COLORREF m_clipPastedColor;
	COLORREF m_listSmallQuickPasteIndexColor;
	COLORREF m_mainWindowBG;
	COLORREF m_Border;
	COLORREF m_BorderTopMost;
	COLORREF m_BorderNotConnected;
	COLORREF m_searchTextBoxFocusBG;
	COLORREF m_searchTextBoxFocusText;
	COLORREF m_searchTextBoxFocusBorder;
	COLORREF m_searchTextHighlight;

	COLORREF m_groupTreeBG;
	COLORREF m_groupTreeText;

	COLORREF m_descriptionWindowBG;
	COLORREF m_descriptionWindowText;

	// Modern scrollbar colors
	COLORREF m_scrollBarThumb;
	COLORREF m_scrollBarThumbHover;
	COLORREF m_scrollBarTrack;

	int m_captionSize;
	int m_captionFontSize;

	// v4 semantic tokens
	COLORREF m_surfaceBase;
	COLORREF m_surfaceElevated;
	COLORREF m_surfaceRowAlt;
	COLORREF m_textPrimary;
	COLORREF m_textSecondary;
	COLORREF m_textDisabled;
	COLORREF m_textOnAccent;
	COLORREF m_accentDefault;
	COLORREF m_accentSubtle;
	COLORREF m_accentText;
	COLORREF m_strokeCard;
	COLORREF m_strokeDivider;
	COLORREF m_controlFill;
	COLORREF m_controlHover;
	COLORREF m_controlPressed;
	COLORREF m_controlDisabledBG;
	COLORREF m_stateHover;
	COLORREF m_stateSelectedBG;
	COLORREF m_stateSelectedText;
	COLORREF m_indicatorBadge;
	int m_radiusControl;
	int m_rowHeightCompact;
	int m_rowHeightComfortable;
	CString m_csFontFamily;

	// true when the value came from the theme xml, false when defaulted/derived
	bool m_hasSurfaceBase, m_hasSurfaceElevated, m_hasSurfaceRowAlt;
	bool m_hasTextPrimary, m_hasTextSecondary, m_hasTextDisabled, m_hasTextOnAccent;
	bool m_hasAccentDefault, m_hasAccentSubtle, m_hasAccentText;
	bool m_hasStrokeCard, m_hasStrokeDivider;
	bool m_hasControlFill, m_hasControlHover, m_hasControlPressed, m_hasControlDisabledBG;
	bool m_hasStateHover, m_hasStateSelectedBG, m_hasStateSelectedText, m_hasIndicatorBadge;
	bool m_hasFontFamily;
	// legacy nodes read during derivation
	bool m_hasLegacyMainWindowBG, m_hasLegacySearchTextBoxFocusBG, m_hasLegacyListBoxEvenRowsBG;
	bool m_hasLegacyListBoxOddRowsText, m_hasLegacyClipPastedColor, m_hasLegacySmallQuickPasteIndexColor;
	bool m_bParsedThemeXml;
	bool m_bDarkTheme;

	CString m_csLastError;
	long m_lFileVersion;
	CString m_csAuthor;
	CString m_csNotes;

	__int64 m_LastWriteTime;
	CString m_lastTheme;

	void LoadDefaults();
};
