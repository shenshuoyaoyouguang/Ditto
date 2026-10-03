#pragma once

// FluentOptionPaint: the single owner of how every owner-drawn option control
// looks. Checkbox, radio button, combo box field, combo drop item and push
// button all draw through here, which is what keeps them reading as one set
// instead of five.
//
// The controls themselves are installed as window subclasses on the stock
// Button / ComboBox classes (see FluentOptionSubclass), so nothing in the rc
// files or the DDX bindings changes to adopt these.
//
// Colours come from the v4 theme tokens on every paint -- never cached -- so a
// theme switch is picked up by the next redraw with no extra invalidation
// plumbing. Sizes come from the tokens too, which is also what gives
// CTheme::RadiusControl() and CTheme::RowHeightCompact() their first callers.

#include <windows.h>
#include <afxwin.h>

// Only ever passed by reference, so forward declarations are enough here and
// the header stays free of the gdiplus include.
namespace Gdiplus
{
	class GraphicsPath;
	struct RectF;
}

class CDPI;

// What kind of stock control a subclass instance is attached to. Decided once at
// install time from the window class name + style bits.
enum FluentControlKind
{
	FK_None = 0,
	FK_CheckBox,
	FK_Radio,
	FK_Combo,
	FK_Button,
};

// Which of the three button looks to draw. Only meaningful for FK_Button; the
// other kinds derive their appearance from their own state matrix.
enum FluentButtonStyle
{
	FBS_Accent = 0,		// solid accent fill, TextOnAccent
	FBS_Secondary,		// Control.Fill + 1px Stroke.Card
	FBS_Subtle,			// transparent, fill only on hover / press
};

// Per-control state the subclass keeps. Deliberately holds the HWND and never a
// CWnd*: the window is owned by the dialog's own DDX_Control binding, and the
// subclass must not outlive or contradict it. Everything here is a cache of
// what the paint code needs; the authoritative values (checked state, enabled
// state, focus) are re-read from the window at paint time, because code like
// CheckDlgButton() changes them without ever reaching our window procedure.
struct FluentControlState
{
	HWND hwnd;
	int kind;
	bool bHover;
	bool bPressed;
	bool bFocus;
	bool bEnabled;
	bool bMouseTracked;	// TME_LEAVE already requested
	HFONT hFont;
	bool bDirtyWidth;		// combo: item list changed, drop width needs recomputing
};

namespace FluentOptionPaint
{
	// Shared geometry, in physical pixels for the given window's dpi. The two
	// theme-derived fields (radius, rowH) are what re-enable the RadiusControl
	// and RowHeightCompact tokens; the rest are the fixed part of the visual
	// language from the design plan section 2.2.
	struct Metrics
	{
		int box;			// checkbox / radio box edge
		int boxGap;			// box -> label
		int ringGap;		// focus ring inset from the box edge
		int ringWidth;		// focus ring stroke
		int markWidth;		// check glyph / radio dot stroke; Scale() is int-only,
							// so the 1.6px logical value is what makes the
							// glyph look drawn rather than stepped
		int radius;			// rounded corner radius, from RadiusControl
		int rowH;			// combo drop item height, from RowHeightCompact
		int comboH;			// combo field height; set via SetWindowPos, not
							// CB_SETITEMHEIGHT (that one only affects list
							// items and its wParam meaning is documented
							// inconsistently across SDK revisions)
		int arrowZoneW;		// combo arrow hit zone
		int arrowW;			// combo chevron width
		int arrowH;			// combo chevron height
		int textPadL;		// combo drop item left padding
		int selBarW;		// selected marker bar, matches CSidebar
		int markInset;		// checkbox glyph inset from the box edge
	};

	Metrics GetMetrics(HWND hwndControl);

	// Entry point for the subclass: paints whichever kind hwnd is. Called
	// instead of DefSubclassProc for WM_PAINT.
	LRESULT Paint(HWND hwnd, FluentControlState* pState);

	// WM_ERASEBKGND handler. Returns TRUE without painting: every Paint* below
	// fills its whole clip area, so letting the default erase through first
	// would just cost a flicker.
	LRESULT EraseBkgnd(HWND hwnd, FluentControlState* pState);

	void PaintCheckBox(HDC hdc, const CRect& rc, const Metrics& m, const FluentControlState& state, bool bChecked);
	void PaintRadio(HDC hdc, const CRect& rc, const Metrics& m, const FluentControlState& state, bool bChecked);

	// styleOfCtrlId picks the variant from a control id (IDOK -> accent, and so
	// on) for the subclass path; PaintButtonAs takes the style directly for
	// CFluentButton, which was configured in code rather than from the rc.
	// radiusOverride lets CFluentButton::SetCornerRadius keep working; pass -1
	// to use the RadiusControl token.
	void PaintButton(HDC hdc, const CRect& rc, const Metrics& m, const FluentControlState& state, int nCtrlId);
	void PaintButtonAs(HDC hdc, const CRect& rc, const Metrics& m, const FluentControlState& state,
		FluentButtonStyle style, int radiusOverride = -1);

	// Combo drop list item, driven by the parent's WM_DRAWITEM once the rc
	// carries CBS_OWNERDRAWFIXED. rcItem is the full item rect and is filled
	// edge to edge on purpose: the drop list is a system window, so leaving a
	// system-coloured gap inside it would be the one light artefact left in a
	// dark theme.
	void PaintComboItem(LPDRAWITEMSTRUCT pDrawItemStruct, const Metrics& m);

	// Text measurement against the control's own font. Single entry point on
	// purpose -- measuring against a default DC yields DEFAULT_GUI_FONT metrics
	// and every caller then computes the wrong width.
	CSize MeasureText(HWND hwndControl, const CString& csText);

	// Resizes a combo's drop list to fit its widest item, capped so a deep
	// group hierarchy cannot push the list off screen.
	void UpdateComboDropWidth(HWND hwndCombo);

	// Shared with CFluentButton and the sidebar/chip rows. Kept here so the
	// radius clamp lives in exactly one place: AddArc with an out-of-range
	// diameter draws nothing, and the 16px checkbox box is the tight case.
	void AddRoundPath(Gdiplus::GraphicsPath& path, const Gdiplus::RectF& rect, float radius);
}
