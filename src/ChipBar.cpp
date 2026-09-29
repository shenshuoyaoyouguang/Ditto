// ChipBar.cpp : pill style type filter chips (ui-redesign)

#include "stdafx.h"
#include "ChipBar.h"
#include "CP_Main.h"
#include "QListCtrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CChipBar::CChipBar()
{
	m_nSelected = 0;
	m_nHoverChip = -1;
	m_dpi = NULL;
	m_bTrackingLeave = false;
}

BEGIN_MESSAGE_MAP(CChipBar, CWnd)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSELEAVE()
	ON_WM_SIZE()
END_MESSAGE_MAP()

void CChipBar::Create(CWnd *pParent, UINT nID, CDPI *dpi)
{
	m_dpi = dpi;
	CRect rcInit(0, 0, 0, 0);
	CWnd::Create(NULL, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, rcInit, pParent, nID);
}

void CChipBar::SetDpiInfo(CDPI *dpi)
{
	m_dpi = dpi;
	Layout();
}

void CChipBar::SetChips(const std::vector<CString> &csLabels)
{
	m_csLabels = csLabels;
	m_nCounts.clear();
	m_nCounts.resize(m_csLabels.size(), -1);
	if (m_nSelected >= (int)m_csLabels.size())
	{
		m_nSelected = 0;
	}
	Layout();
}

void CChipBar::SetChipCount(int nIndex, int nCount)
{
	if (nIndex >= 0 && nIndex < (int)m_nCounts.size())
	{
		m_nCounts[nIndex] = nCount;
		Invalidate(FALSE);
	}
}

void CChipBar::SetSelected(int nIndex)
{
	if (nIndex != m_nSelected)
	{
		m_nSelected = nIndex;
		Invalidate(FALSE);
	}
}

int CChipBar::GetBarHeight()
{
	if (m_dpi == NULL)
	{
		return 22;
	}

	return m_dpi->Scale(20);
}

void CChipBar::Layout()
{
	if (m_dpi == NULL || !IsWindow(m_hWnd))
	{
		return;
	}

	CRect rcClient;
	GetClientRect(rcClient);

	m_rcChips.clear();

	CDC *pDC = GetDC();
	int x = m_dpi->Scale(8);
	int y = (rcClient.Height() - GetBarHeight()) / 2;
	int nHeight = GetBarHeight();

	CFont *pFont = GetParent() ? GetParent()->GetFont() : GetFont();
	CFont *pOldFont = pDC->SelectObject(pFont);

	for (size_t i = 0; i < m_csLabels.size(); i++)
	{
		CString csLabel = m_csLabels[i];
		if (i < m_nCounts.size() && m_nCounts[i] >= 0)
		{
			csLabel += StrF(_T("  %d"), m_nCounts[i]);
		}

		CSize szText = pDC->GetTextExtent(csLabel);
		int nWidth = szText.cx + m_dpi->Scale(18);

		CRect rcChip(x, y, x + nWidth, y + nHeight);
		m_rcChips.push_back(rcChip);
		x += nWidth + m_dpi->Scale(5);
	}

	pDC->SelectObject(pOldFont);
	ReleaseDC(pDC);
}

int CChipBar::HitTest(CPoint point)
{
	for (size_t i = 0; i < m_rcChips.size(); i++)
	{
		if (m_rcChips[i].PtInRect(point))
		{
			return (int)i;
		}
	}

	return -1;
}

void CChipBar::GetChipColors(COLORREF &crBG, COLORREF &crText, int nIndex) const
{
	CTheme &theme = CGetSetOptions::m_Theme;

	if (nIndex == m_nSelected)
	{
		crBG = theme.ChipSelectedBG();
		crText = theme.ListBoxOddRowsText();
	}
	else if (nIndex == m_nHoverChip)
	{
		crBG = theme.RowHoverBG();
		crText = theme.SubText();
	}
	else
	{
		crBG = theme.ChipBG();
		crText = theme.SubText();
	}
}

BOOL CChipBar::OnEraseBkgnd(CDC* pDC)
{
	CRect rcClient;
	GetClientRect(rcClient);
	pDC->FillSolidRect(rcClient, CGetSetOptions::m_Theme.MainWindowBG());
	return TRUE;
}

void CChipBar::OnPaint()
{
	CPaintDC dc(this);

	CFont *pFont = GetParent() ? GetParent()->GetFont() : GetFont();
	CFont *pOldFont = dc.SelectObject(pFont);
	int nOldBkMode = dc.SetBkMode(TRANSPARENT);

	int nRadius = (m_dpi ? m_dpi->Scale(9) : 9);

	for (size_t i = 0; i < m_csLabels.size() && i < m_rcChips.size(); i++)
	{
		CString csLabel = m_csLabels[i];
		if (i < m_nCounts.size() && m_nCounts[i] >= 0)
		{
			csLabel += StrF(_T("  %d"), m_nCounts[i]);
		}

		COLORREF crBG;
		COLORREF crText;
		GetChipColors(crBG, crText, (int)i);

		CBrush brush(crBG);
		CPen pen(PS_SOLID, 1, crBG);
		CBrush *pOldBrush = dc.SelectObject(&brush);
		CPen *pOldPen = dc.SelectObject(&pen);
		dc.RoundRect(m_rcChips[i], CPoint(nRadius, nRadius));
		dc.SelectObject(pOldBrush);
		dc.SelectObject(pOldPen);

		COLORREF crOldText = dc.SetTextColor(crText);
		dc.DrawText(csLabel, m_rcChips[i], DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
		dc.SetTextColor(crOldText);
	}

	dc.SetBkMode(nOldBkMode);
	dc.SelectObject(pOldFont);
}

void CChipBar::OnLButtonDown(UINT nFlags, CPoint point)
{
	int nChip = HitTest(point);
	if (nChip >= 0)
	{
		SetSelected(nChip);
		GetParent()->PostMessage(NM_TYPE_FILTER_CHANGED, nChip, 0);
	}

	CWnd::OnLButtonDown(nFlags, point);
}

void CChipBar::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_dpi != NULL && !m_bTrackingLeave)
	{
		TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT), TME_LEAVE, m_hWnd, 0 };
		if (TrackMouseEvent(&tme))
		{
			m_bTrackingLeave = true;
		}
	}

	int nChip = HitTest(point);
	if (nChip != m_nHoverChip)
	{
		m_nHoverChip = nChip;
		Invalidate(FALSE);
	}

	CWnd::OnMouseMove(nFlags, point);
}

void CChipBar::OnMouseLeave()
{
	m_bTrackingLeave = false;
	if (m_nHoverChip >= 0)
	{
		m_nHoverChip = -1;
		Invalidate(FALSE);
	}

	CWnd::OnMouseLeave();
}

void CChipBar::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);
	Layout();
}
