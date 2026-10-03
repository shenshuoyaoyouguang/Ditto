#include "stdafx.h"
#include "FluentOptionPaint.h"
#include "Fonts.h"
#include "Options.h"
#include "DPI.h"

using namespace Gdiplus;

// ---------------------------------------------------------------------------
// helpers

void FluentOptionPaint::AddRoundPath(GraphicsPath& path, const RectF& rect, float radius)
{
	// The clamp is load bearing, not defensive tidiness. AddArc takes the arc
	// bounding box, so a radius larger than half the height hands it a negative
	// extent and nothing is drawn at all -- which is exactly what a 16px
	// checkbox box would hit if RadiusControl ever came back larger than 8.
	float r = min(radius, rect.Height / 2);
	r = max(1.0f, r);
	float d = r * 2;

	path.AddArc(rect.X, rect.Y, d, d, 180, 90);
	path.AddArc(rect.X + rect.Width - d, rect.Y, d, d, 270, 90);
	path.AddArc(rect.X + rect.Width - d, rect.Y + rect.Height - d, d, d, 0, 90);
	path.AddArc(rect.X, rect.Y + rect.Height - d, d, d, 90, 90);
	path.CloseFigure();
}

static void FillPath(HDC hdc, const CRect& rc, int radius, COLORREF fill)
{
	if (rc.Width() <= 0 || rc.Height() <= 0)
		return;

	Graphics graphics(hdc);
	graphics.SetSmoothingMode(SmoothingModeAntiAlias);

	RectF rect((REAL)rc.left, (REAL)rc.top, (REAL)rc.Width() - 1, (REAL)rc.Height() - 1);
	GraphicsPath path;
	FluentOptionPaint::AddRoundPath(path, rect, (float)max(1, radius));

	SolidBrush brush(Color(255, GetRValue(fill), GetGValue(fill), GetBValue(fill)));
	graphics.FillPath(&brush, &path);
}

static void StrokePath(HDC hdc, const CRect& rc, int radius, COLORREF stroke, int width)
{
	if (rc.Width() <= 0 || rc.Height() <= 0)
		return;

	Graphics graphics(hdc);
	graphics.SetSmoothingMode(SmoothingModeAntiAlias);

	RectF rect((REAL)rc.left, (REAL)rc.top, (REAL)rc.Width() - 1, (REAL)rc.Height() - 1);
	GraphicsPath path;
	FluentOptionPaint::AddRoundPath(path, rect, (float)max(1, radius));

	Pen pen(Color(255, GetRValue(stroke), GetGValue(stroke), GetBValue(stroke)), (REAL)max(1, width));
	graphics.DrawPath(&pen, &path);
}

static void FillSolid(HDC hdc, const CRect& rc, COLORREF color)
{
	if (rc.Width() <= 0 || rc.Height() <= 0)
		return;
	::SetDCBrushColor(hdc, color);
	::FillRect(hdc, &rc, (HBRUSH)GetStockObject(DC_BRUSH));
}

FluentOptionPaint::Metrics FluentOptionPaint::GetMetrics(HWND hwndControl)
{
	CDPI dpi;
	dpi.SetHwnd(hwndControl);

	CTheme& theme = CGetSetOptions::m_Theme;

	Metrics m;
	m.box = dpi.Scale(16);
	m.boxGap = dpi.Scale(8);
	m.ringGap = dpi.Scale(1);
	m.ringWidth = dpi.Scale(2);
	m.markWidth = dpi.Scale(1.6f);	// Scale is int-only; the fractional part
									// is what makes the glyph look drawn
									// rather than stepped
	m.radius = dpi.Scale(theme.RadiusControl());
	m.rowH = dpi.Scale(theme.RowHeightCompact());
	m.comboH = dpi.Scale(24);
	m.arrowZoneW = dpi.Scale(20);
	m.arrowW = dpi.Scale(9);
	m.arrowH = dpi.Scale(5);
	m.textPadL = dpi.Scale(10);
	m.selBarW = dpi.Scale(3);
	m.markInset = dpi.Scale(3.5f);
	return m;
}

CSize FluentOptionPaint::MeasureText(HWND hwndControl, const CString& csText)
{
	CSize sz(0, 0);
	if (hwndControl == NULL || csText.IsEmpty())
		return sz;

	// Against the control's own font, not the default DC: AppFonts pushes a
	// 14px face onto every child, and measuring with DEFAULT_GUI_FONT would
	// silently produce the wrong width for every caller.
	HFONT hFont = (HFONT)::SendMessageW(hwndControl, WM_GETFONT, 0, 0);
	if (hFont == NULL)
		hFont = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);

	HDC hdc = ::GetDC(hwndControl);
	if (hdc == NULL)
		return sz;

	HFONT hOld = (HFONT)::SelectObject(hdc, hFont);
	::GetTextExtentPoint32(hdc, csText, csText.GetLength(), &sz);
	::SelectObject(hdc, hOld);
	::ReleaseDC(hwndControl, hdc);

	return sz;
}

// ---------------------------------------------------------------------------
// state helpers

// Everything below re-reads the authoritative state from the window instead of
// trusting the cached flags. CheckDlgButton(), CheckRadioButton() and
// EnableWindow() all change it without our window procedure ever seeing the
// message, so a cached value goes stale the moment any dialog code touches it.
static bool IsReallyEnabled(HWND hwnd, const FluentControlState& state)
{
	return state.bEnabled && ::IsWindowEnabled(hwnd) != 0;
}

static bool IsFocused(HWND hwnd)
{
	return ::GetFocus() == hwnd;
}

static CString ControlText(HWND hwnd){
	LRESULT nLen = ::SendMessageW(hwnd, WM_GETTEXTLENGTH, 0, 0);
	if (nLen <= 0)
		return CString();

	CString csText;
	csText.GetBuffer((int)nLen + 1);
	::SendMessageW(hwnd, WM_GETTEXT, (WPARAM)(nLen + 1), (LPARAM)csText.GetBuffer(0));
	csText.ReleaseBuffer();
	return csText;
}

// ---------------------------------------------------------------------------
// checkbox

void FluentOptionPaint::PaintCheckBox(HDC hdc, const CRect& rc, const Metrics& m,
	const FluentControlState& state, bool bChecked)
{
	CTheme& theme = CGetSetOptions::m_Theme;
	const bool bEnabled = IsReallyEnabled(state.hwnd, state);
	const bool bFocus = IsFocused(state.hwnd);

	// Box, vertically centred in the control rect. 1px from the left so the
	// focus ring has somewhere to go without being clipped by the rect.
	const int boxTop = rc.top + max(0, (rc.Height() - m.box) / 2);
	CRect rcBox(rc.left + 1, boxTop, rc.left + 1 + m.box, boxTop + m.box);

	// Stroke weight carries the hover / pressed affordance on the box only; the
	// label deliberately gets no fill of its own (see the header note).
	int strokeW = m.ringGap;
	COLORREF fill = theme.SurfaceBase();
	COLORREF stroke = theme.StrokeCard();

	if (bEnabled)
	{
		if (state.bPressed)
		{
			stroke = theme.ControlPressed();
			strokeW = m.ringWidth;
		}
		else if (state.bHover)
		{
			stroke = theme.ControlHover();
			strokeW = m.ringWidth;
		}
	}
	else
	{
		stroke = theme.StrokeCard();
	}

	if (bChecked)
	{
		fill = bEnabled ? theme.AccentDefault() : theme.ControlDisabledBG();
		if (bEnabled && state.bPressed)
			fill = ShiftToward(fill, false, 0.08);
		else if (bEnabled && state.bHover)
			fill = ShiftToward(fill, true, 0.08);
		stroke = fill;		// a filled box carries no separate outline
	}
	else
	{
		fill = bEnabled ? theme.SurfaceBase() : theme.SurfaceBase();
	}

	FillPath(hdc, rcBox, m.ringGap, fill);

	if (bChecked == false || bEnabled == false)
		StrokePath(hdc, rcBox, m.ringGap, stroke, strokeW);

	// Check glyph: two segments, round joins, drawn only when there is enough
	// contrast to see it. Using a DT_CHECK glyph instead would depend on the
	// resolved UI font having a checkmark codepoint -- AppFonts may resolve to
	// Microsoft YaHei UI, and a missing glyph silently renders as nothing.
	if (bChecked)
	{
		Graphics graphics(hdc);
		graphics.SetSmoothingMode(SmoothingModeAntiAlias);

		const float w = (float)rcBox.Width();
		const float h = (float)rcBox.Height();
		const float x = (float)rcBox.left;
		const float y = (float)rcBox.top;
		const float inset = (float)m.markInset;

		GraphicsPath tick;
		tick.AddLine(x + inset, y + h * 0.52f,
			x + w * 0.42f, y + h - inset);
		tick.AddLine(x + w * 0.42f, y + h - inset,
			x + w - inset * 0.85f, y + inset);

		COLORREF tickColor = bEnabled ? theme.TextOnAccent() : theme.TextDisabled();
		Pen pen(Color(255, GetRValue(tickColor), GetGValue(tickColor), GetBValue(tickColor)), m.markWidth);
		pen.SetLineJoin(LineJoinRound);
		pen.SetStartCap(LineCapRound);
		pen.SetEndCap(LineCapRound);
		graphics.DrawPath(&pen, &tick);
	}

	// Focus ring, inside the box outline rather than outside it. The 10 DLU
	// tall checkboxes in the rc would clip an outward ring, and a ring that
	// gets clipped reads as a rendering bug rather than as focus.
	if (bFocus && bEnabled)
	{
		CRect rcRing(rcBox);
		rcRing.InflateRect(m.ringGap, m.ringGap);
		StrokePath(hdc, rcRing, m.ringGap + m.ringWidth, theme.AccentDefault(), m.ringWidth);
	}

	// Label
	CRect rcText(rcBox.right + m.boxGap, rc.top, rc.right, rc.bottom);
	if (rcText.Width() <= 0)
		return;

	CFont* pFont = AppFonts::Inst().Get(Font_Body);
	HFONT hOldFont = (HFONT)::SelectObject(hdc, pFont->GetSafeHandle());
	::SetBkMode(hdc, TRANSPARENT);
	::SetTextColor(hdc, bEnabled ? theme.TextPrimary() : theme.TextDisabled());
	// No DT_NOPREFIX: "&" in a caption is a keyboard accelerator and the system
	// draws the underline for us, which is what keeps Alt+A working once this
	// replaces the stock button painting.
	::DrawTextW(hdc, ControlText(state.hwnd), -1, &rcText,
		DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
	::SelectObject(hdc, hOldFont);
}

// ---------------------------------------------------------------------------
// radio button

void FluentOptionPaint::PaintRadio(HDC hdc, const CRect& rc, const Metrics& m,
	const FluentControlState& state, bool bChecked)
{
	CTheme& theme = CGetSetOptions::m_Theme;
	const bool bEnabled = IsReallyEnabled(state.hwnd, state);
	const bool bFocus = IsFocused(state.hwnd);

	const int boxTop = rc.top + max(0, (rc.Height() - m.box) / 2);
	CRect rcBox(rc.left + 1, boxTop, rc.left + 1 + m.box, boxTop + m.box);
	const int r = m.box / 2;

	if (bEnabled == false)
	{
		FillPath(hdc, rcBox, r, theme.SurfaceBase());
		StrokePath(hdc, rcBox, r, theme.StrokeCard(), m.ringGap);
	}
	else if (bChecked)
	{
		FillPath(hdc, rcBox, r, theme.SurfaceBase());
		StrokePath(hdc, rcBox, r, theme.AccentDefault(), m.ringWidth);

		const int dot = m.box / 2;
		CRect rcDot(rcBox.left + (m.box - dot) / 2, rcBox.top + (m.box - dot) / 2,
			rcBox.left + (m.box + dot) / 2, rcBox.top + (m.box + dot) / 2);
		FillPath(hdc, rcDot, dot / 2, theme.AccentDefault());
	}
	else
	{
		FillPath(hdc, rcBox, r, theme.SurfaceBase());
		COLORREF stroke = state.bPressed ? theme.ControlPressed()
			: state.bHover ? theme.ControlHover()
			: theme.StrokeCard();
		StrokePath(hdc, rcBox, r, stroke, (state.bHover || state.bPressed) ? m.ringWidth : m.ringGap);
	}

	if (bFocus && bEnabled)
	{
		CRect rcRing(rcBox);
		rcRing.InflateRect(m.ringGap, m.ringGap);
		StrokePath(hdc, rcRing, r + m.ringGap, theme.AccentDefault(), m.ringWidth);
	}

	CRect rcText(rcBox.right + m.boxGap, rc.top, rc.right, rc.bottom);
	if (rcText.Width() <= 0)
		return;

	CFont* pFont = AppFonts::Inst().Get(Font_Body);
	HFONT hOldFont = (HFONT)::SelectObject(hdc, pFont->GetSafeHandle());
	::SetBkMode(hdc, TRANSPARENT);
	::SetTextColor(hdc, bEnabled ? theme.TextPrimary() : theme.TextDisabled());
	::DrawTextW(hdc, ControlText(state.hwnd), -1, &rcText,
		DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
	::SelectObject(hdc, hOldFont);
}

// ---------------------------------------------------------------------------
// push button

void FluentOptionPaint::PaintButton(HDC hdc, const CRect& rc, const Metrics& m,
	const FluentControlState& state, int nCtrlId)
{
	FluentButtonStyle style = FBS_Secondary;
	if (nCtrlId == IDOK || nCtrlId == ID_APPLY_NOW || nCtrlId == IDYES)
		style = FBS_Accent;

	PaintButtonAs(hdc, rc, m, state, style);
}

void FluentOptionPaint::PaintButtonAs(HDC hdc, const CRect& rc, const Metrics& mIn,
	const FluentControlState& state, FluentButtonStyle style, int radiusOverride)
{
	CTheme& theme = CGetSetOptions::m_Theme;
	const bool bEnabled = IsReallyEnabled(state.hwnd, state);
	const bool bFocus = IsFocused(state.hwnd);

	Metrics m = mIn;
	if (radiusOverride >= 0)
	{
		CDPI dpi;
		dpi.SetHwnd(state.hwnd);
		m.radius = dpi.Scale(radiusOverride);
	}

	COLORREF background;
	COLORREF text;
	bool bStroke = false;
	COLORREF stroke = 0;

	if (bEnabled == false)
	{
		background = theme.ControlDisabledBG();
		text = theme.TextDisabled();
	}
	else if (style == FBS_Accent)
	{
		background = theme.AccentDefault();
		if (state.bPressed)
			background = ShiftToward(background, false, 0.08);
		else if (state.bHover)
			background = ShiftToward(background, true, 0.08);
		text = theme.TextOnAccent();
	}
	else if (style == FBS_Subtle)
	{
		background = state.bPressed ? theme.ControlPressed()
			: state.bHover ? theme.ControlHover()
			: theme.SurfaceBase();
		text = theme.TextPrimary();
	}
	else
	{
		background = state.bPressed ? theme.ControlPressed()
			: state.bHover ? theme.ControlHover()
			: theme.ControlFill();
		text = theme.TextPrimary();
		bStroke = true;
		stroke = theme.StrokeCard();
	}

	FillPath(hdc, rc, m.radius, background);

	if (bStroke)
		StrokePath(hdc, rc, m.radius, stroke, m.ringGap);

	if (bFocus && bEnabled)
	{
		// Inset, unlike the option controls: a push button is 30px tall so an
		// inward ring costs nothing, and a button this size is never clipped.
		CRect rcRing(rc);
		rcRing.DeflateRect(m.ringGap, m.ringGap);
		StrokePath(hdc, rcRing, max(1, m.radius - m.ringGap), theme.AccentDefault(), m.ringWidth);
	}

	CString csText = ControlText(state.hwnd);
	if (csText.IsEmpty())
		return;

	CFont* pFont = AppFonts::Inst().Get(Font_Body);
	HFONT hOldFont = (HFONT)::SelectObject(hdc, pFont->GetSafeHandle());
	::SetBkMode(hdc, TRANSPARENT);
	::SetTextColor(hdc, text);
	CRect rcText(rc);
	::DrawTextW(hdc, csText, -1, &rcText, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
	::SelectObject(hdc, hOldFont);
}

// ---------------------------------------------------------------------------
// combo field (the closed state)

static void PaintComboFieldImpl(HDC hdc, const CRect& rc, const FluentOptionPaint::Metrics& m,
	HWND hwnd, const FluentControlState& state)
{
	CTheme& theme = CGetSetOptions::m_Theme;
	const bool bEnabled = IsReallyEnabled(hwnd, state);
	const bool bFocus = IsFocused(hwnd);

	COLORREF background;
	if (bEnabled == false)
		background = theme.ControlDisabledBG();
	else if (state.bPressed)
		background = theme.ControlPressed();
	else if (state.bHover)
		background = theme.ControlHover();
	else
		background = theme.ControlFill();

	FillPath(hdc, rc, m.radius, background);
	StrokePath(hdc, rc, m.radius,
		bFocus && bEnabled ? theme.AccentDefault() : theme.StrokeCard(), m.ringGap);

	// Chevron rather than a filled triangle, matching Win11. Two strokes, so
	// it inherits the same anti-aliasing as everything else here.
	{
		const int cx = rc.right - m.arrowZoneW / 2;
		const int cy = rc.top + rc.Height() / 2;
		const int halfW = m.arrowW / 2;
		const int halfH = m.arrowH / 2;

		Graphics graphics(hdc);
		graphics.SetSmoothingMode(SmoothingModeAntiAlias);

		GraphicsPath chevron;
		chevron.AddLine((REAL)(cx - halfW), (REAL)(cy - halfH / 2),
			(REAL)cx, (REAL)(cy + halfH / 2));
		chevron.AddLine((REAL)cx, (REAL)(cy + halfH / 2),
			(REAL)(cx + halfW), (REAL)(cy - halfH / 2));

		COLORREF pen = bEnabled
			? ((bFocus || state.bHover) ? theme.TextPrimary() : theme.TextSecondary())
			: theme.TextDisabled();
		Pen p(Color(255, GetRValue(pen), GetGValue(pen), GetBValue(pen)), m.markWidth);
		p.SetLineJoin(LineJoinRound);
		p.SetStartCap(LineCapRound);
		p.SetEndCap(LineCapRound);
		graphics.DrawPath(&p, &chevron);
	}

	// Selected text, clipped short of the arrow zone.
	CRect rcText(rc);
	rcText.left += m.textPadL;
	rcText.right -= m.arrowZoneW;
	if (rcText.Width() <= 0)
		return;

	LRESULT nLen = ::SendMessageW(hwnd, WM_GETTEXTLENGTH, 0, 0);
	CString csText;
	if (nLen > 0)
	{
		csText.GetBuffer((int)nLen + 1);
		::SendMessageW(hwnd, WM_GETTEXT, (WPARAM)(nLen + 1), (LPARAM)csText.GetBuffer(0));
		csText.ReleaseBuffer();
	}
	if (csText.IsEmpty())
		return;

	CFont* pFont = AppFonts::Inst().Get(Font_Body);
	HFONT hOldFont = (HFONT)::SelectObject(hdc, pFont->GetSafeHandle());
	::SetBkMode(hdc, TRANSPARENT);
	::SetTextColor(hdc, bEnabled ? theme.TextPrimary() : theme.TextDisabled());
	::DrawTextW(hdc, csText, -1, &rcText, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
	::SelectObject(hdc, hOldFont);
}

void FluentOptionPaint::UpdateComboDropWidth(HWND hwndCombo)
{
	if (hwndCombo == NULL)
		return;

	CDPI dpi;
	dpi.SetHwnd(hwndCombo);
	const Metrics m = GetMetrics(hwndCombo);

	// Space the label needs, plus the arrow zone and a little breathing room on
	// both sides. A drop list is allowed to be wider than its field, which is
	// the only reason a long theme name or a deep group name stays readable
	// instead of being ellipsised.
	const int chrome = m.textPadL + m.arrowZoneW + dpi.Scale(12);

	int nCount = (int)::SendMessageW(hwndCombo, CB_GETCOUNT, 0, 0);
	int nWidest = 0;

	HFONT hFont = (HFONT)::SendMessageW(hwndCombo, WM_GETFONT, 0, 0);
	if (hFont == NULL)
		hFont = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);

	HDC hdc = ::GetDC(hwndCombo);
	if (hdc == NULL)
		return;

	HFONT hOld = (HFONT)::SelectObject(hdc, hFont);
	for (int i = 0; i < nCount; i++)
	{
		LRESULT nLen = ::SendMessageW(hwndCombo, CB_GETLBTEXTLEN, (WPARAM)i, 0);
		if (nLen <= 0)
			continue;

		// CB_GETLBTEXT copies into the buffer and NUL-terminates, so the
		// allocation has to be the reported length + 1 or the last character of
		// the longest item is cut.
		CString csItem;
		csItem.GetBuffer((int)nLen + 1);
		::SendMessageW(hwndCombo, CB_GETLBTEXT, (WPARAM)i, (LPARAM)csItem.GetBuffer(0));
		csItem.ReleaseBuffer();

		CSize sz(0, 0);
		::GetTextExtentPoint32(hdc, csItem, csItem.GetLength(), &sz);
		nWidest = max(nWidest, sz.cx);
	}
	::SelectObject(hdc, hOld);
	::ReleaseDC(hwndCombo, hdc);

	CRect rcField;
	::GetWindowRect(hwndCombo, &rcField);

	int nWant = nWidest + chrome;
	nWant = max(nWant, rcField.Width());	// never shrink below the field
	nWant = min(nWant, dpi.Scale(360));		// cap so a deep tree cannot leave the screen

	if (nWant != (int)::SendMessageW(hwndCombo, CB_GETDROPPEDWIDTH, 0, 0))
		::SendMessageW(hwndCombo, CB_SETDROPPEDWIDTH, (WPARAM)nWant, 0);
}

// ---------------------------------------------------------------------------
// combo drop item

void FluentOptionPaint::PaintComboItem(LPDRAWITEMSTRUCT pDrawItemStruct, const Metrics& m)
{
	if (pDrawItemStruct == NULL || pDrawItemStruct->CtlType != ODT_COMBOBOX)
		return;

	CTheme& theme = CGetSetOptions::m_Theme;

	CDC* pDC = CDC::FromHandle(pDrawItemStruct->hDC);
	CRect rc(pDrawItemStruct->rcItem);

	const bool bSelected = (pDrawItemStruct->itemState & ODS_SELECTED) != 0;
	const bool bDisabled = (pDrawItemStruct->itemState & ODS_DISABLED) != 0;

	// Full-width fill, no inset: this list lives in a system popup window, so
	// any gap left unfilled shows up as a light stripe in a dark theme.
	FillSolid(pDC->GetSafeHdc(), rc,
		bSelected ? theme.StateSelectedBG() : theme.SurfaceElevated());

	CDPI dpi;
	dpi.SetHwnd(pDrawItemStruct->hwndItem);
	const int vInset = dpi.Scale(5);

	if (bSelected)
	{
		// Same marker as CSidebar and the options nav list, so "selected" means
		// one thing across the app. CChipBar keeps the outlined variant.
		CRect rcBar(rc.left, rc.top + vInset, rc.left + m.selBarW, rc.bottom - vInset);
		FillSolid(pDC->GetSafeHdc(), rcBar, theme.AccentDefault());
	}

	CRect rcText(rc);
	rcText.left += m.textPadL;
	rcText.right -= dpi.Scale(8);
	if (rcText.Width() <= 0)
		return;

	LRESULT nLen = ::SendMessageW(pDrawItemStruct->hwndItem, CB_GETLBTEXTLEN,
		(WPARAM)pDrawItemStruct->itemID, 0);
	CString csText;
	if (nLen > 0)
	{
		csText.GetBuffer((int)nLen + 1);
		::SendMessageW(pDrawItemStruct->hwndItem, CB_GETLBTEXT,
			(WPARAM)pDrawItemStruct->itemID, (LPARAM)csText.GetBuffer(0));
		csText.ReleaseBuffer();
	}
	if (csText.IsEmpty())
		return;

	CFont* pFont = AppFonts::Inst().Get(Font_Body);
	HFONT hOldFont = (HFONT)::SelectObject(pDC->GetSafeHdc(), pFont->GetSafeHandle());
	::SetBkMode(pDC->GetSafeHdc(), TRANSPARENT);
	::SetTextColor(pDC->GetSafeHdc(),
		bDisabled ? theme.TextDisabled()
		: bSelected ? theme.StateSelectedText()
		: theme.TextPrimary());
	::DrawTextW(pDC->GetSafeHdc(), csText, -1, &rcText,
		DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
	::SelectObject(pDC->GetSafeHdc(), hOldFont);
}

// ---------------------------------------------------------------------------
// entry point

LRESULT FluentOptionPaint::Paint(HWND hwnd, FluentControlState* pState)
{
	if (hwnd == NULL || pState == NULL)
		return 0;

	HDC hdc = ::GetDC(hwnd);
	if (hdc == NULL)
		return 0;

	// Double buffered through a memory dc: these are the only owner-drawn
	// controls in the dialog, and a checkbox repainting straight to the screen
	// dc flickers visibly against the themed dialog background.
	CRect rc;
	::GetClientRect(hwnd, &rc);

	CDC dc;
	dc.CreateCompatibleDC(CDC::FromHandle(hdc));
	CBitmap bmp;
	bmp.CreateCompatibleBitmap(CDC::FromHandle(hdc), max(1, rc.Width()), max(1, rc.Height()));
	CBitmap* pOldBmp = dc.SelectObject(&bmp);

	const Metrics m = GetMetrics(hwnd);

	// The dialog background is behind us; the control rect is not opaque, so
	// fill with the same colour the dialog paints rather than leaving the
	// bitmap's undefined contents behind.
	FillSolid(dc.GetSafeHdc(), rc, CGetSetOptions::m_Theme.SurfaceBase());

	switch (pState->kind)
	{
	case FK_CheckBox:
	{
		BOOL bChecked = ::SendMessageW(hwnd, BM_GETCHECK, 0, 0) == BST_CHECKED;
		PaintCheckBox(dc.GetSafeHdc(), rc, m, *pState, bChecked != FALSE);
		break;
	}
	case FK_Radio:
	{
		BOOL bChecked = ::SendMessageW(hwnd, BM_GETCHECK, 0, 0) == BST_CHECKED;
		PaintRadio(dc.GetSafeHdc(), rc, m, *pState, bChecked != FALSE);
		break;
	}
	case FK_Combo:
		PaintComboFieldImpl(dc.GetSafeHdc(), rc, m, hwnd, *pState);
		break;
	case FK_Button:
		PaintButton(dc.GetSafeHdc(), rc, m, *pState, ::GetDlgCtrlID(hwnd));
		break;
	default:
		break;
	}

	::BitBlt(hdc, 0, 0, rc.Width(), rc.Height(), dc.GetSafeHdc(), 0, 0, SRCCOPY);

	dc.SelectObject(pOldBmp);
	::ReleaseDC(hwnd, hdc);
	return 1;
}

LRESULT FluentOptionPaint::EraseBkgnd(HWND hwnd, FluentControlState* /*pState*/)
{
	// Every Paint* path fills its whole clip area, so there is nothing to erase
	// and returning TRUE here is what keeps the dialog from flashing through.
	(void)hwnd;
	return TRUE;
}
