#include "stdafx.h"
#include "FluentButton.h"
#include "Fonts.h"
#include "DPI.h"
#include "Options.h"
#include "FluentOptionPaint.h"

// ShiftColor and AddRoundPath now live in FluentOptionPaint so that the
// owner-drawn option controls and this button share one implementation -- the
// accent hover / pressed shift and the corner radius clamp in particular, where
// two copies would be free to drift apart.

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

	CRect rc(pDrawItemStruct->rcItem);
	if (rc.IsRectEmpty())
		return;

	// The actual painting moved to FluentOptionPaint so this button and the
	// owner-drawn option controls (checkbox / radio / combo) cannot drift apart
	// -- they used to carry their own copies of the same colour and radius
	// logic. The state is marshalled into the same struct the subclass path
	// uses, which keeps one DrawItem implementation for both routes.
	FluentControlState state;
	state.hwnd = GetSafeHwnd();
	state.kind = FK_Button;
	state.bHover = m_bHover;
	state.bPressed = m_bPressed;
	state.bFocus = (pDrawItemStruct->itemState & ODS_FOCUS) != 0;
	state.bEnabled = (pDrawItemStruct->itemState & ODS_DISABLED) == 0;
	state.bMouseTracked = false;
	state.hFont = NULL;

	const FluentOptionPaint::Metrics metrics = FluentOptionPaint::GetMetrics(GetSafeHwnd());

	HDC hdc = ::GetDC(GetSafeHwnd());
	if (hdc == NULL)
		return;

	CDC* pDC = CDC::FromHandle(hdc);

	switch (m_style)
	{
	case Style_Accent:		FluentOptionPaint::PaintButtonAs(hdc, rc, metrics, state, FBS_Accent, m_radius); break;
	case Style_Subtle:		FluentOptionPaint::PaintButtonAs(hdc, rc, metrics, state, FBS_Subtle, m_radius); break;
	case Style_Secondary:
	default:				FluentOptionPaint::PaintButtonAs(hdc, rc, metrics, state, FBS_Secondary, m_radius); break;
	}

	::ReleaseDC(GetSafeHwnd(), hdc);
	(void)pDC;
}
