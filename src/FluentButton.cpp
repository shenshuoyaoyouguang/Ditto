#include "stdafx.h"
#include "FluentButton.h"
#include "Fonts.h"
#include "DPI.h"
#include "Options.h"

static COLORREF ShiftColor(COLORREF color, bool towardWhite, double percent)
{
	int target = towardWhite ? 255 : 0;

	auto shift = [target, percent](int c) -> BYTE
	{
		int value = (int)(c + (target - c) * percent + 0.5);
		return (BYTE)max(0, min(255, value));
	};

	return RGB(shift(GetRValue(color)), shift(GetGValue(color)), shift(GetBValue(color)));
}

static void AddRoundPath(GraphicsPath& path, const RectF& rect, float radius)
{
	float r = min(radius, rect.Height / 2);
	r = max(1.0f, r);
	float d = r * 2;

	path.AddArc(rect.X, rect.Y, d, d, 180, 90);
	path.AddArc(rect.X + rect.Width - d, rect.Y, d, d, 270, 90);
	path.AddArc(rect.X + rect.Width - d, rect.Y + rect.Height - d, d, d, 0, 90);
	path.AddArc(rect.X, rect.Y + rect.Height - d, d, d, 90, 90);
	path.CloseFigure();
}

CFluentButton::CFluentButton()
	: m_style(Style_Secondary)
	, m_radius(4)
	, m_bHover(false)
	, m_bPressed(false)
{
}

BEGIN_MESSAGE_MAP(CFluentButton, CButton)
	ON_WM_ERASEBKGND()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSELEAVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_ENABLE()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
END_MESSAGE_MAP()

void CFluentButton::PreSubclassWindow()
{
	CButton::PreSubclassWindow();
	ModifyStyle(0, BS_OWNERDRAW);
}

BOOL CFluentButton::OnEraseBkgnd(CDC* pDC)
{
	return TRUE; // DrawItem paints the full face, avoid the default gray flash
}

void CFluentButton::OnMouseMove(UINT nFlags, CPoint point)
{
	if (!m_bHover)
	{
		m_bHover = true;

		TRACKMOUSEEVENT tme;
		tme.cbSize = sizeof(tme);
		tme.dwFlags = TME_LEAVE;
		tme.hwndTrack = GetSafeHwnd();
		tme.dwHoverTime = 0;
		TrackMouseEvent(&tme);

		Invalidate(FALSE);
	}

	CButton::OnMouseMove(nFlags, point);
}

void CFluentButton::OnMouseLeave()
{
	if (m_bHover)
	{
		m_bHover = false;
		m_bPressed = false;
		Invalidate(FALSE);
	}

	CButton::OnMouseLeave();
}

void CFluentButton::OnLButtonDown(UINT nFlags, CPoint point)
{
	m_bPressed = true;
	Invalidate(FALSE);
	CButton::OnLButtonDown(nFlags, point);
}

void CFluentButton::OnLButtonUp(UINT nFlags, CPoint point)
{
	m_bPressed = false;
	Invalidate(FALSE);
	CButton::OnLButtonUp(nFlags, point);
}

void CFluentButton::OnEnable(BOOL bEnable)
{
	Invalidate(FALSE);
	CButton::OnEnable(bEnable);
}

void CFluentButton::OnSetFocus(CWnd* pOldWnd)
{
	Invalidate(FALSE);
	CButton::OnSetFocus(pOldWnd);
}

void CFluentButton::OnKillFocus(CWnd* pNewWnd)
{
	Invalidate(FALSE);
	CButton::OnKillFocus(pNewWnd);
}

void CFluentButton::DrawItem(LPDRAWITEMSTRUCT pDrawItemStruct)
{
	if (pDrawItemStruct == NULL || pDrawItemStruct->CtlType != ODT_BUTTON)
		return;

	CDC* pDC = CDC::FromHandle(pDrawItemStruct->hDC);
	CRect rc(pDrawItemStruct->rcItem);
	if (rc.IsRectEmpty())
		return;

	CTheme& theme = CGetSetOptions::m_Theme;
	bool enabled = (pDrawItemStruct->itemState & ODS_DISABLED) == 0;
	bool focused = (pDrawItemStruct->itemState & ODS_FOCUS) != 0;

	CDPI dpi;
	dpi.SetHwnd(GetSafeHwnd());
	float radius = (float)max(1, dpi.Scale(m_radius));

	COLORREF background;
	COLORREF text;
	bool drawStroke = false;
	COLORREF stroke = 0;

	if (!enabled)
	{
		background = theme.ControlDisabledBG();
		text = theme.TextDisabled();
	}
	else
	{
		switch (m_style)
		{
		case Style_Accent:
			background = theme.AccentDefault();
			if (m_bPressed)
				background = ShiftColor(background, false, 0.08);
			else if (m_bHover)
				background = ShiftColor(background, true, 0.08);
			text = theme.TextOnAccent();
			break;

		case Style_Secondary:
			background = m_bPressed ? theme.ControlPressed()
				: m_bHover ? theme.ControlHover()
				: theme.ControlFill();
			text = theme.TextPrimary();
			drawStroke = true;
			stroke = theme.StrokeCard();
			break;

		case Style_Subtle:
		default:
			background = m_bPressed ? theme.ControlPressed()
				: m_bHover ? theme.ControlHover()
				: theme.SurfaceBase();
			text = theme.TextPrimary();
			break;
		}
	}

	RectF rect((REAL)rc.left, (REAL)rc.top, (REAL)rc.Width() - 1, (REAL)rc.Height() - 1);

	Graphics graphics(pDC->GetSafeHdc());
	graphics.SetSmoothingMode(SmoothingModeAntiAlias);

	GraphicsPath path;
	AddRoundPath(path, rect, radius);

	SolidBrush backgroundBrush(Color(255, GetRValue(background), GetGValue(background), GetBValue(background)));
	graphics.FillPath(&backgroundBrush, &path);

	if (drawStroke)
	{
		Pen strokePen(Color(255, GetRValue(stroke), GetGValue(stroke), GetBValue(stroke)));
		graphics.DrawPath(&strokePen, &path);
	}

	if (focused && enabled)
	{
		RectF inner(rect.X + 2, rect.Y + 2, rect.Width - 4, rect.Height - 4);
		GraphicsPath focusPath;
		AddRoundPath(focusPath, inner, max(1.0f, radius - 2));
		COLORREF ring = theme.AccentDefault();
		Pen focusPen(Color(255, GetRValue(ring), GetGValue(ring), GetBValue(ring)));
		graphics.DrawPath(&focusPen, &focusPath);
	}

	CString csText;
	GetWindowText(csText);

	if (!csText.IsEmpty())
	{
		pDC->SetBkMode(TRANSPARENT);
		pDC->SetTextColor(text);

		CFont* pOldFont = pDC->SelectObject(AppFonts::Inst().Get(Font_Body));
		pDC->DrawText(csText, rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
		pDC->SelectObject(pOldFont);
	}
}
