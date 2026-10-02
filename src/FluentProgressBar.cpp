#include "stdafx.h"
#include "FluentProgressBar.h"
#include "Fonts.h"
#include "Options.h"

CFluentProgressBar::CFluentProgressBar()
	: m_bMarquee(false)
	, m_nMarqueePos(0)
{
}

BEGIN_MESSAGE_MAP(CFluentProgressBar, CProgressCtrl)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_TIMER()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

BOOL CFluentProgressBar::Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID)
{
	// progress visuals are fully owner drawn, the native control only feeds
	// the pos/range state
	return CProgressCtrl::Create(dwStyle | PBS_SMOOTH | WS_CHILD | WS_VISIBLE, rect, pParentWnd, nID);
}

void CFluentProgressBar::SetMarquee(bool bEnable)
{
	if (GetSafeHwnd() == NULL)
		return;

	if (bEnable == m_bMarquee)
		return;

	m_bMarquee = bEnable;

	if (bEnable)
	{
		m_nMarqueePos = 0;
		SetTimer(MARQUEE_TIMER_ID, 30, NULL);
	}
	else
	{
		KillTimer(MARQUEE_TIMER_ID);
	}

	Invalidate();
}

void CFluentProgressBar::OnTimer(UINT_PTR nEvent)
{
	if (nEvent == MARQUEE_TIMER_ID && m_bMarquee)
	{
		CRect rc;
		GetClientRect(rc);

		// ~2.5 widths per second, wrapping. Keep the offset inside the trough so
		// the segment disappears fully before it reappears on the other side.
		m_nMarqueePos += max(2, rc.Width() / 40);
		if (m_nMarqueePos > max(1, rc.Width()))
			m_nMarqueePos = 0;

		Invalidate();
	}

	CProgressCtrl::OnTimer(nEvent);
}

void CFluentProgressBar::OnDestroy()
{
	KillTimer(MARQUEE_TIMER_ID);
	m_bMarquee = false;

	CProgressCtrl::OnDestroy();
}

BOOL CFluentProgressBar::OnEraseBkgnd(CDC* pDC)
{
	// Claiming "erased" without painting anything leaves the pixels outside the
	// rounded path holding whatever was there before (stale or undefined), which
	// shows up as dark corners against the dialog.
	if (pDC != NULL)
	{
		CRect rc;
		GetClientRect(rc);
		pDC->FillSolidRect(rc, CGetSetOptions::m_Theme.SurfaceBase());
		return TRUE;
	}

	return TRUE;
}

void CFluentProgressBar::DrawMarqueeSegment(Graphics& graphics, const CRect& rc)
{
	int radius = max(2, (int)(rc.Height() / 2));
	RectF trough((REAL)rc.left, (REAL)rc.top, (REAL)rc.Width() - 1, (REAL)rc.Height() - 1);
	float r = (float)min(radius, (int)(trough.Height / 2));
	float d = r * 2;

	GraphicsPath path;
	path.AddArc(trough.X, trough.Y, d, d, 180, 90);
	path.AddArc(trough.X + trough.Width - d, trough.Y, d, d, 270, 90);
	path.AddArc(trough.X + trough.Width - d, trough.Y + trough.Height - d, d, d, 0, 90);
	path.AddArc(trough.X, trough.Y + trough.Height - d, d, d, 90, 90);
	path.CloseFigure();

	SolidBrush troughBrush(Color(255,
		GetRValue(CGetSetOptions::m_Theme.ControlFill()),
		GetGValue(CGetSetOptions::m_Theme.ControlFill()),
		GetBValue(CGetSetOptions::m_Theme.ControlFill())));
	graphics.FillPath(&troughBrush, &path);

	// A third of the trough, offset by the animated position.
	float segment = trough.Width / 3.0f;
	float x = trough.X + (float)m_nMarqueePos;
	if (x + segment > trough.X + trough.Width)
		segment = trough.X + trough.Width - x;
	if (segment <= 1.0f)
		return;

	RectF seg(x, trough.Y, segment, trough.Height);
	float segD = min(d, min(seg.Width, seg.Height));

	GraphicsPath segPath;
	segPath.AddArc(seg.X, seg.Y, segD, segD, 180, 90);
	segPath.AddArc(seg.X + seg.Width - segD, seg.Y, segD, segD, 270, 90);
	segPath.AddArc(seg.X + seg.Width - segD, seg.Y + seg.Height - segD, segD, segD, 0, 90);
	segPath.AddArc(seg.X, seg.Y + seg.Height - segD, segD, segD, 90, 90);
	segPath.CloseFigure();

	COLORREF accent = CGetSetOptions::m_Theme.AccentDefault();
	SolidBrush segBrush(Color(255, GetRValue(accent), GetGValue(accent), GetBValue(accent)));
	graphics.FillPath(&segBrush, &segPath);
}

void CFluentProgressBar::OnPaint()
{
	CPaintDC dc(this);

	CTheme& theme = CGetSetOptions::m_Theme;
	CRect rc;
	GetClientRect(rc);

	Graphics graphics(dc.GetSafeHdc());
	graphics.SetSmoothingMode(SmoothingModeAntiAlias);

	// paint the whole client area first: the corners outside the rounded path
	// belong to the dialog surface, not to the bar
	SolidBrush surface(Color(255,
		GetRValue(theme.SurfaceBase()),
		GetGValue(theme.SurfaceBase()),
		GetBValue(theme.SurfaceBase())));
	graphics.FillRectangle(&surface, 0, 0, (REAL)rc.Width(), (REAL)rc.Height());

	if (m_bMarquee)
	{
		DrawMarqueeSegment(graphics, rc);
		return;
	}

	int radius = max(2, (int)(rc.Height() / 2));
	RectF rect((REAL)rc.left, (REAL)rc.top, (REAL)rc.Width() - 1, (REAL)rc.Height() - 1);
	float r = (float)min(radius, (int)(rect.Height / 2));
	float d = r * 2;

	GraphicsPath path;
	path.AddArc(rect.X, rect.Y, d, d, 180, 90);
	path.AddArc(rect.X + rect.Width - d, rect.Y, d, d, 270, 90);
	path.AddArc(rect.X + rect.Width - d, rect.Y + rect.Height - d, d, d, 0, 90);
	path.AddArc(rect.X, rect.Y + rect.Height - d, d, d, 90, 90);
	path.CloseFigure();

	SolidBrush trough(Color(255, GetRValue(theme.ControlFill()), GetGValue(theme.ControlFill()), GetBValue(theme.ControlFill())));
	graphics.FillPath(&trough, &path);

	int nLower = 0;
	int nUpper = 100;
	GetRange(nLower, nUpper);
	int nPos = GetPos();

	if (nUpper > nLower && nPos > nLower)
	{
		double fraction = (double)(nPos - nLower) / (double)(nUpper - nLower);
		fraction = max(0.0, min(1.0, fraction));

		if (fraction > 0.0)
		{
			RectF fill(rect.X, rect.Y, (REAL)(rect.Width * fraction), rect.Height);

			// The corner diameter cannot exceed the fill box, otherwise the right
			// side's arc bounding boxes start left of fill.X and the filled area
			// bulges backwards past the actual progress position.
			float fillD = (float)min(d, min(fill.Width, fill.Height));

			GraphicsPath fillPath;
			fillPath.AddArc(fill.X, fill.Y, fillD, fillD, 180, 90);
			fillPath.AddArc(fill.X + fill.Width - fillD, fill.Y, fillD, fillD, 270, 90);
			fillPath.AddArc(fill.X + fill.Width - fillD, fill.Y + fill.Height - fillD, fillD, fillD, 0, 90);
			fillPath.AddArc(fill.X, fill.Y + fill.Height - fillD, fillD, fillD, 90, 90);
			fillPath.CloseFigure();

			SolidBrush fillBrush(Color(255, GetRValue(theme.AccentDefault()), GetGValue(theme.AccentDefault()), GetBValue(theme.AccentDefault())));
			graphics.FillPath(&fillBrush, &fillPath);
		}
	}

	// stroke last, otherwise a full progress value paints over the outline
	COLORREF strokeColor = theme.StrokeCard();
	Pen stroke(Color(255, GetRValue(strokeColor), GetGValue(strokeColor), GetBValue(strokeColor)));
	graphics.DrawPath(&stroke, &path);
}
