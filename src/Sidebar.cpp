// Sidebar.cpp : docked left sidebar of the quick paste window (ui-redesign)

#include "stdafx.h"
#include "Sidebar.h"
#include "CP_Main.h"
#include "QListCtrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CSidebar, CWnd)

CSidebar::CSidebar()
{
	m_nSelectedNav = 0;
	m_dpi = NULL;
}

BEGIN_MESSAGE_MAP(CSidebar, CWnd)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_SIZE()
END_MESSAGE_MAP()

void CSidebar::Create(CWnd *pParent, UINT nID, CDPI *dpi)
{
	m_dpi = dpi;
	CRect rcInit(0, 0, 0, 0);
	CWnd::Create(NULL, NULL, WS_CHILD | WS_CLIPCHILDREN, rcInit, pParent, nID);
}

void CSidebar::SetDpiInfo(CDPI *dpi)
{
	m_dpi = dpi;
	Layout();
}

void CSidebar::SetNavItems(const std::vector<CString> &csLabels)
{
	m_csNavLabels = csLabels;
	if (m_nSelectedNav >= (int)m_csNavLabels.size())
	{
		m_nSelectedNav = 0;
	}
	Layout();
}

void CSidebar::SetBottomItems(const std::vector<CString> &csLabels)
{
	m_csBottomLabels = csLabels;
	Layout();
}

void CSidebar::SetSelectedNav(int nIndex)
{
	if (nIndex != m_nSelectedNav)
	{
		m_nSelectedNav = nIndex;
		Invalidate(FALSE);
	}
}

int CSidebar::GetWidth()
{
	if (m_dpi == NULL)
	{
		return 158;
	}

	return m_dpi->Scale(158);
}

void CSidebar::Layout()
{
	if (m_dpi == NULL || !IsWindow(m_hWnd))
	{
		return;
	}

	CRect rcClient;
	GetClientRect(rcClient);

	m_rcNav.clear();
	m_rcBottom.clear();

	int x = m_dpi->Scale(8);
	int nRowHeight = m_dpi->Scale(28);
	int y = m_dpi->Scale(8);

	for (size_t i = 0; i < m_csNavLabels.size(); i++)
	{
		CRect rc(x, y, rcClient.Width() - x, y + nRowHeight);
		m_rcNav.push_back(rc);
		y += nRowHeight + m_dpi->Scale(2);
	}

	// group header label sits below the nav, the tree fills to the bottom entries
	y += m_dpi->Scale(8);
	y += m_dpi->Scale(16);

	int nBottomHeight = m_dpi->Scale(28) + m_dpi->Scale(8);
	int yBottom = rcClient.Height() - nBottomHeight;

	for (size_t i = 0; i < m_csBottomLabels.size(); i++)
	{
		CRect rc(x, yBottom, rcClient.Width() - x, yBottom + nRowHeight);
		m_rcBottom.push_back(rc);
		yBottom += nRowHeight;
	}

	m_rcTree = CRect(x - m_dpi->Scale(4), y, rcClient.Width() - m_dpi->Scale(2), yBottom - m_dpi->Scale(8));
}

CRect CSidebar::GetTreeRect()
{
	return m_rcTree;
}

int CSidebar::HitTest(CPoint point, bool &bBottom)
{
	bBottom = false;

	for (size_t i = 0; i < m_rcNav.size(); i++)
	{
		if (m_rcNav[i].PtInRect(point))
		{
			return (int)i;
		}
	}

	for (size_t i = 0; i < m_rcBottom.size(); i++)
	{
		if (m_rcBottom[i].PtInRect(point))
		{
			bBottom = true;
			return (int)i;
		}
	}

	return -1;
}

void CSidebar::DrawRow(CDC *pDC, const CRect &rc, const CString &csLabel, bool bSelected)
{
	CTheme &theme = CGetSetOptions::m_Theme;

	if (bSelected)
	{
		int nRadius = m_dpi ? m_dpi->Scale(6) : 6;
		CBrush brush(theme.ChipSelectedBG());
		CPen pen(PS_SOLID, 1, theme.ChipSelectedBG());
		CBrush *pOldBrush = pDC->SelectObject(&brush);
		CPen *pOldPen = pDC->SelectObject(&pen);
		pDC->RoundRect(&rc, CPoint(nRadius, nRadius));
		pDC->SelectObject(pOldBrush);
		pDC->SelectObject(pOldPen);
	}

	COLORREF crText = bSelected ? theme.ListBoxOddRowsText() : theme.SidebarText();
	COLORREF crOld = pDC->SetTextColor(crText);

	CRect rcText(rc);
	rcText.left += m_dpi ? m_dpi->Scale(10) : 10;
	pDC->DrawText(csLabel, rcText, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

	pDC->SetTextColor(crOld);
}

BOOL CSidebar::OnEraseBkgnd(CDC* pDC)
{
	CRect rcClient;
	GetClientRect(rcClient);
	pDC->FillSolidRect(rcClient, CGetSetOptions::m_Theme.SidebarBG());
	return TRUE;
}

void CSidebar::OnPaint()
{
	CPaintDC dc(this);
	CTheme &theme = CGetSetOptions::m_Theme;

	CRect rcClient;
	GetClientRect(rcClient);

	CFont *pFont = GetParent() ? GetParent()->GetFont() : GetFont();
	CFont *pOldFont = dc.SelectObject(pFont);
	int nOldBkMode = dc.SetBkMode(TRANSPARENT);

	// right separator line
	dc.FillSolidRect(rcClient.right - m_dpi->Scale(1), 0, m_dpi->Scale(1), rcClient.Height(), theme.SeparatorLine());

	// main navigation
	for (size_t i = 0; i < m_csNavLabels.size() && i < m_rcNav.size(); i++)
	{
		DrawRow(&dc, m_rcNav[i], m_csNavLabels[i], (int)i == m_nSelectedNav);
	}

	// group section header between nav and tree
	int yGroup = m_dpi->Scale(8) + (int)m_rcNav.size() * (m_dpi->Scale(28) + m_dpi->Scale(2)) + m_dpi->Scale(8);
	CRect rcGroupLabel(m_dpi->Scale(8), yGroup, rcClient.Width() - m_dpi->Scale(8), yGroup + m_dpi->Scale(16));
	COLORREF crOld = dc.SetTextColor(theme.FaintText());
	dc.DrawText(theApp.m_Language.GetString(_T("GroupsSection"), _T("Groups")), rcGroupLabel, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
	dc.SetTextColor(crOld);

	// bottom entries
	for (size_t i = 0; i < m_csBottomLabels.size() && i < m_rcBottom.size(); i++)
	{
		DrawRow(&dc, m_rcBottom[i], m_csBottomLabels[i], false);
	}

	dc.SetBkMode(nOldBkMode);
	dc.SelectObject(pOldFont);
}

void CSidebar::OnLButtonDown(UINT nFlags, CPoint point)
{
	bool bBottom = false;
	int nItem = HitTest(point, bBottom);
	if (nItem >= 0)
	{
		if (!bBottom)
		{
			SetSelectedNav(nItem);
		}
		GetParent()->PostMessage(NM_SIDEBAR_NAV, nItem, bBottom ? 1 : 0);
	}

	CWnd::OnLButtonDown(nFlags, point);
}

void CSidebar::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);
	Layout();
}
