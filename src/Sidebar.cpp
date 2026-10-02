#include "stdafx.h"
#include "Sidebar.h"
#include "Fonts.h"
#include "Options.h"

CSidebar::CSidebar()
{
	m_dpi = NULL;
	m_bViewsLaidOut = false;
	m_active = VIEW_ALL;
	m_hover = -1;
	m_bHoverTracked = false;
}

BEGIN_MESSAGE_MAP(CSidebar, CWnd)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSELEAVE()
	ON_WM_SIZE()
END_MESSAGE_MAP()

BOOL CSidebar::Create(CWnd* pParent, UINT nID)
{
	if (pParent == NULL)
		return FALSE;

	if (CreateEx(0, AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, ::LoadCursor(NULL, IDC_ARROW)),
		NULL, WS_CHILD | WS_CLIPSIBLINGS, CRect(0, 0, 0, 0), pParent, nID) == FALSE)
	{
		return FALSE;
	}

	return TRUE;
}

void CSidebar::SetLabels(const CString csLabels[VIEW_COUNT])
{
	for (int i = 0; i < VIEW_COUNT; i++)
		m_labels[i] = csLabels[i];

	m_bViewsLaidOut = false;
	Invalidate(FALSE);
}

int CSidebar::ViewHitTest(CPoint point)
{
	for (int i = 0; i < VIEW_COUNT; i++)
	{
		if (m_views[i].PtInRect(point))
			return i;
	}

	return -1;
}

void CSidebar::LayoutViews()
{
	if (m_dpi == NULL)
		return;

	CRect rcClient;
	GetClientRect(rcClient);

	int rowHeight = m_dpi->Scale(28);
	int top = m_dpi->Scale(6);
	int inset = m_dpi->Scale(6);

	for (int i = 0; i < VIEW_COUNT; i++)
	{
		m_views[i].SetRect(inset, top, rcClient.Width() - inset, top + rowHeight);
		top += rowHeight;
	}

	m_bViewsLaidOut = true;
}

void CSidebar::GetGroupsArea(CRect& rc)
{
	if (m_bViewsLaidOut == false)
		LayoutViews();

	CRect rcClient;
	GetClientRect(rcClient);

	int divider = m_dpi != NULL ? m_dpi->Scale(28) : 28;	// "groups" caption strip
	rc.SetRect(m_dpi != NULL ? m_dpi->Scale(4) : 4,
		(m_bViewsLaidOut ? m_views[VIEW_COUNT - 1].bottom : 0) + divider,
		rcClient.Width() - (m_dpi != NULL ? m_dpi->Scale(4) : 4),
		rcClient.bottom);
}

void CSidebar::OnPaint()
{
	CPaintDC dc(this);

	CTheme& theme = CGetSetOptions::m_Theme;

	if (m_bViewsLaidOut == false)
		LayoutViews();

	CRect rcClient;
	GetClientRect(rcClient);

	dc.FillSolidRect(rcClient, theme.SurfaceBase());

	// right divider separates the sidebar from the list
	dc.FillSolidRect(rcClient.right - m_dpi->Scale(1), 0, m_dpi->Scale(1), rcClient.Height(), theme.StrokeDivider());

	dc.SetBkMode(TRANSPARENT);
	CFont* pFont = AppFonts::Inst().Get(Font_Caption);

	for (int i = 0; i < VIEW_COUNT; i++)
	{
		const CRect& rc = m_views[i];
		bool bActive = (i == m_active);
		bool bHover = (i == m_hover && i != m_active);

		if (bActive)
		{
			CRect rcActive(rc);
			rcActive.DeflateRect(m_dpi->Scale(2), 0, m_dpi->Scale(2), 0);
			dc.FillSolidRect(rcActive, theme.AccentSubtle());

			// accent bar on the left edge of the active row
			CRect rcBar(rc.left, rc.top + m_dpi->Scale(4), rc.left + m_dpi->Scale(3), rc.bottom - m_dpi->Scale(4));
			dc.FillSolidRect(rcBar, theme.AccentDefault());
		}
		else if (bHover)
		{
			CRect rcHover(rc);
			rcHover.DeflateRect(m_dpi->Scale(2), 0, m_dpi->Scale(2), 0);
			dc.FillSolidRect(rcHover, theme.ControlHover());
		}

		CFont* pOld = dc.SelectObject(pFont);
		dc.SetTextColor(bActive ? theme.TextPrimary() : theme.TextSecondary());
		CRect rcText(rc);
		rcText.left += m_dpi->Scale(10);
		dc.DrawText(m_labels[i], rcText, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
		dc.SelectObject(pOld);
	}

	// "groups" caption above the tree area
	CRect rcGroups;
	GetGroupsArea(rcGroups);

	CRect rcCaption(rcGroups.left, rcGroups.top - m_dpi->Scale(24), rcGroups.right, rcGroups.top);
	dc.SetTextColor(theme.TextSecondary());
	CFont* pCaption = dc.SelectObject(AppFonts::Inst().Get(Font_Caption));
	dc.DrawText(m_csGroupsCaption, rcCaption, DT_LEFT | DT_BOTTOM | DT_SINGLELINE | DT_NOPREFIX);
	dc.SelectObject(pCaption);
}

BOOL CSidebar::OnEraseBkgnd(CDC* /*pDC*/)
{
	return TRUE;
}

void CSidebar::OnLButtonDown(UINT nFlags, CPoint point)
{
	int view = ViewHitTest(point);
	if (view >= 0)
	{
		GetParent()->SendMessage(NM_SIDEBAR_VIEW_CLICKED, view, 0);
	}

	CWnd::OnLButtonDown(nFlags, point);
}

void CSidebar::OnMouseMove(UINT nFlags, CPoint point)
{
	int view = ViewHitTest(point);

	if (view != m_hover)
	{
		int old = m_hover;
		m_hover = view;

		if (old >= 0)
			InvalidateRect(m_views[old], FALSE);
		if (view >= 0)
			InvalidateRect(m_views[view], FALSE);
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

void CSidebar::OnMouseLeave()
{
	m_bHoverTracked = false;

	if (m_hover >= 0)
	{
		InvalidateRect(m_views[m_hover], FALSE);
		m_hover = -1;
	}

	CWnd::OnMouseLeave();
}

void CSidebar::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	// the tree area depends on the client size
	Invalidate(FALSE);
}
