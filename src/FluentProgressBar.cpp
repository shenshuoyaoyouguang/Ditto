#include "stdafx.h"
#include "FluentProgressBar.h"
#include "Fonts.h"
#include "Options.h"

CFluentProgressBar::CFluentProgressBar()
{
}

BEGIN_MESSAGE_MAP(CFluentProgressBar, CProgressCtrl)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

BOOL CFluentProgressBar::Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID)
{
	// progress visuals are fully owner drawn, the native control only feeds
	// the pos/range state
	return CProgressCtrl::Create(dwStyle | PBS_SMOOTH | WS_CHILD | WS_VISIBLE, rect, pParentWnd, nID);
}

BOOL CFluentProgressBar::OnEraseBkgnd(CDC* /*pDC*/)
{
	return TRUE;
}

void CFluentProgressBar::OnPaint()
{
	CPaintDC dc(this);

	CTheme& theme = CGetSetOptions::m_Theme;
	CRect rc;
	GetClientRect(rc);

	Graphics graphics(dc.GetSafeHdc());
	graphics.SetSmoothingMode(SmoothingModeAntiAlias);

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

	COLORREF strokeColor = theme.StrokeCard();
	Pen stroke(Color(255, GetRValue(strokeColor), GetGValue(strokeColor), GetBValue(strokeColor)));
	graphics.DrawPath(&stroke, &path);

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

			GraphicsPath fillPath;
			fillPath.AddArc(fill.X, fill.Y, d, d, 180, 90);
			fillPath.AddArc(fill.X + fill.Width - d, fill.Y, d, d, 270, 90);
			fillPath.AddArc(fill.X + fill.Width - d, fill.Y + fill.Height - d, d, d, 0, 90);
			fillPath.AddArc(fill.X, fill.Y + fill.Height - d, d, d, 90, 90);
			fillPath.CloseFigure();

			SolidBrush fillBrush(Color(255, GetRValue(theme.AccentDefault()), GetGValue(theme.AccentDefault()), GetBValue(theme.AccentDefault())));
			graphics.FillPath(&fillBrush, &fillPath);
		}
	}
}
