#include "stdafx.h"
#include "ChipBar.h"
#include "Fonts.h"
#include "Options.h"

static void FillChipRect(CDC* pDC, const CRect& rc, int radius, COLORREF color, COLORREF strokeColor)
{
	if (rc.Width() <= 0 || rc.Height() <= 0)
		return;

	Graphics graphics(pDC->GetSafeHdc());
	graphics.SetSmoothingMode(SmoothingModeAntiAlias);

	RectF rect((REAL)rc.left, (REAL)rc.top, (REAL)rc.Width() - 1, (REAL)rc.Height() - 1);
	float r = (float)min(radius, min(rc.Width(), rc.Height()) / 2);
	float d = r * 2;

	GraphicsPath path;
	path.AddArc(rect.X, rect.Y, d, d, 180, 90);
	path.AddArc(rect.X + rect.Width - d, rect.Y, d, d, 270, 90);
	path.AddArc(rect.X + rect.Width - d, rect.Y + rect.Height - d, d, d, 0, 90);
	path.AddArc(rect.X, rect.Y + rect.Height - d, d, d, 90, 90);
	path.CloseFigure();

	SolidBrush brush(Color(255, GetRValue(color), GetGValue(color), GetBValue(color)));
	graphics.FillPath(&brush, &path);

	Pen stroke(Color(255, GetRValue(strokeColor), GetGValue(strokeColor), GetBValue(strokeColor)));
	graphics.DrawPath(&stroke, &path);
}

CChipBar::CChipBar()
{
	m_dpi = NULL;
	m_bChipsLaidOut = false;
	m_active = CHIP_ALL;
	m_hover = -1;
	m_bHoverTracked = false;
	m_bHoverEnabled = true;
	m_bHasFocus = false;
}

BEGIN_MESSAGE_MAP(CChipBar, CWnd)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSELEAVE()
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_KEYDOWN()
END_MESSAGE_MAP()

BOOL CChipBar::Create(CWnd* pParent, UINT nID)
{
	if (pParent == NULL)
		return FALSE;

	if (CreateEx(0, AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, ::LoadCursor(NULL, IDC_ARROW)),
		// WS_TABSTOP: the row was mouse-only, so keyboard users could not reach
		// the type filters at all.
		NULL, WS_CHILD | WS_CLIPSIBLINGS | WS_TABSTOP, CRect(0, 0, 0, 0), pParent, nID) == FALSE)
	{
		return FALSE;
	}

	return TRUE;
}

void CChipBar::SetLabels(const CString csLabels[CHIP_COUNT])
{
	for (int i = 0; i < CHIP_COUNT; i++)
		m_labels[i] = csLabels[i];

	m_bChipsLaidOut = false;
	Invalidate(FALSE);
}

int CChipBar::HitTestChip(CPoint point)
{
	for (int i = 0; i < CHIP_COUNT; i++)
	{
		if (m_chips[i].PtInRect(point))
			return i;
	}

	return -1;
}

void CChipBar::LayoutChips()
{
	if (m_dpi == NULL)
		return;

	CRect rcClient;
	GetClientRect(rcClient);

	CDC* pDC = GetDC();
	CFont* pOld = pDC->SelectObject(AppFonts::Inst().Get(Font_Caption));

	int chipHeight = m_dpi->Scale(22);
	int top = (rcClient.Height() - chipHeight) / 2;
	int left = m_dpi->Scale(4);
	int gap = m_dpi->Scale(6);
	int padding = m_dpi->Scale(10);

	for (int i = 0; i < CHIP_COUNT; i++)
	{
		CSize size = pDC->GetTextExtent(m_labels[i]);
		int width = size.cx + padding * 2;

		m_chips[i].SetRect(left, top, left + width, top + chipHeight);
		left = m_chips[i].right + gap;
	}

	pDC->SelectObject(pOld);
	ReleaseDC(pDC);

	m_bChipsLaidOut = true;
}

void CChipBar::OnPaint()
{
	CPaintDC dc(this);

	CTheme& theme = CGetSetOptions::m_Theme;

	if (m_bChipsLaidOut == false)
		LayoutChips();

	dc.FillSolidRect(&dc.m_ps.rcPaint, theme.SurfaceBase());

	CFont* pFont = AppFonts::Inst().Get(Font_Caption);
	int radius = m_dpi != NULL ? m_dpi->Scale(11) : 11;

	dc.SetBkMode(TRANSPARENT);
	dc.SetTextColor(theme.TextSecondary());

	for (int i = 0; i < CHIP_COUNT; i++)
	{
		const CRect& rc = m_chips[i];
		bool bActive = (i == m_active);
		bool bHover = (i == m_hover && m_bHoverEnabled);

		if (bActive)
		{
			FillChipRect(&dc, rc, radius, theme.AccentSubtle(), theme.AccentDefault());
			dc.SetTextColor(theme.TextPrimary());

			// Focus ring, so the keyboard path is visible. Without it the row
			// gave no indication that it had focus at all. The fill matches the
			// row background so only the stroke shows.
			if (m_bHasFocus)
			{
				CRect rcFocus = rc;
				rcFocus.InflateRect(m_dpi != NULL ? m_dpi->Scale(2) : 2, m_dpi != NULL ? m_dpi->Scale(2) : 2);
				FillChipRect(&dc, rcFocus, radius, theme.SurfaceBase(), theme.AccentDefault());
			}
		}
		else if (bHover)
		{
			FillChipRect(&dc, rc, radius, theme.ControlHover(), theme.StrokeCard());
		}
		else
		{
			FillChipRect(&dc, rc, radius, theme.ControlFill(), theme.StrokeCard());
		}

		CFont* pOld = dc.SelectObject(pFont);
		CRect rcText(rc);
		dc.DrawText(m_labels[i], &rcText, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
		dc.SelectObject(pOld);

		if (bActive)
			dc.SetTextColor(theme.TextSecondary());
	}
}

BOOL CChipBar::OnEraseBkgnd(CDC* /*pDC*/)
{
	return TRUE;
}

void CChipBar::OnLButtonDown(UINT nFlags, CPoint point)
{
	int chip = HitTestChip(point);
	if (chip >= 0)
	{
		GetParent()->SendMessage(NM_CHIP_CLICKED, chip, 0);
	}

	CWnd::OnLButtonDown(nFlags, point);
}

void CChipBar::OnMouseMove(UINT nFlags, CPoint point)
{
	int chip = HitTestChip(point);

	if (chip != m_hover)
	{
		int old = m_hover;
		m_hover = chip;

		if (old >= 0)
			InvalidateRect(m_chips[old], FALSE);
		if (chip >= 0)
			InvalidateRect(m_chips[chip], FALSE);
	}

	if (m_bHoverTracked == false)
	{
		TRACKMOUSEEVENT track;
		track.cbSize = sizeof(track);
		track.dwFlags = TME_LEAVE;
		track.hwndTrack = GetSafeHwnd();
		track.dwHoverTime = 0;

		if (TrackMouseEvent(&track))
			m_bHoverTracked = true;
	}

	CWnd::OnMouseMove(nFlags, point);
}

void CChipBar::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	// Drop the cached geometry: the chip sizes come from font metrics scaled by
	// the dpi and the hit rects are derived from them, so a resize (or a dpi
	// change, which resizes us) has to invalidate the layout.
	if (nType != SIZE_MINIMIZED)
		m_bChipsLaidOut = false;

	Invalidate(FALSE);
}

void CChipBar::OnSetFocus(CWnd* pPrevWnd)
{
	CWnd::OnSetFocus(pPrevWnd);

	m_bHasFocus = true;
	Invalidate(FALSE);
}

void CChipBar::OnKillFocus(CWnd* pNextWnd)
{
	CWnd::OnKillFocus(pNextWnd);

	m_bHasFocus = false;
	Invalidate(FALSE);
}

void CChipBar::MoveActiveTo(int index)
{
	if (index < 0 || index >= CHIP_COUNT)
		return;

	m_active = index;
	Invalidate(FALSE);

	if (GetParent() != NULL)
		GetParent()->SendMessage(NM_CHIP_CLICKED, (WPARAM)index, 0);
}

void CChipBar::OnKeyDown(UINT nChar, UINT nRepCount, UINT nFlags)
{
	int next = m_active;

	switch (nChar)
	{
	case VK_LEFT:
		next = (m_active <= 0) ? CHIP_COUNT - 1 : m_active - 1;
		break;

	case VK_RIGHT:
		next = (m_active >= CHIP_COUNT - 1) ? 0 : m_active + 1;
		break;

	case VK_HOME:
		next = 0;
		break;

	case VK_END:
		next = CHIP_COUNT - 1;
		break;

	case VK_SPACE:
	case VK_RETURN:
		// The arrows already moved the active chip, so it is what the user means.
		MoveActiveTo(m_active);
		return;

	default:
		CWnd::OnKeyDown(nChar, nRepCount, nFlags);
		return;
	}

	MoveActiveTo(next);
}

void CChipBar::OnMouseLeave()
{
	m_bHoverTracked = false;

	if (m_hover >= 0)
	{
		InvalidateRect(m_chips[m_hover], FALSE);
		m_hover = -1;
	}

	CWnd::OnMouseLeave();
}
