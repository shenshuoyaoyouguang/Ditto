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

IMPLEMENT_DYNAMIC(CChipBar, CWnd)

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
	ON_WM_KEYDOWN()
	ON_WM_GETDLGCODE()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSELEAVE()
	ON_WM_SIZE()
END_MESSAGE_MAP()

void CChipBar::Create(CWnd *pParent, UINT nID, CDPI *dpi)
{
	m_dpi = dpi;
	CRect rcInit(0, 0, 0, 0);
	CWnd::Create(NULL, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_TABSTOP, rcInit, pParent, nID);
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
		Layout();
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

int CChipBar::GetBarHeight(int width)
{
	return Layout(width);
}

int CChipBar::Layout(int width)
{
	int nHeight = m_dpi ? m_dpi->Scale(20) : 22;
	if (m_dpi == NULL || !IsWindow(m_hWnd))
	{
		return nHeight;
	}

	CRect rcClient;
	GetClientRect(rcClient);

	m_rcChips.clear();

	CDC *pDC = GetDC();
	int clientWidth = max(1, width >= 0 ? width : rcClient.Width());
	int left = min(m_dpi->Scale(8), clientWidth - 1);
	int x = left;
	int y = 0;
	int gap = m_dpi->Scale(5);

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
		int nWidth = min(szText.cx + m_dpi->Scale(18), clientWidth - left);
		if (x > left && x + nWidth > clientWidth)
		{
			x = left;
			y += nHeight + gap;
		}

		CRect rcChip(x, y, x + nWidth, y + nHeight);
		m_rcChips.push_back(rcChip);
		x += nWidth + gap;
	}

	pDC->SelectObject(pOldFont);
	ReleaseDC(pDC);

	// Keep the original vertical centering when there is only one row.
	if (y == 0 && width < 0)
	{
		for (auto &rect : m_rcChips)
			rect.OffsetRect(0, max(0, (rcClient.Height() - nHeight) / 2));
	}
	return y + nHeight;
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
	CRect rcClient;
	GetClientRect(rcClient);
	dc.FillSolidRect(rcClient, CGetSetOptions::m_Theme.MainWindowBG());

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
		dc.DrawText(csLabel, m_rcChips[i], DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
		dc.SetTextColor(crOldText);
		if (GetFocus() == this && (int)i == m_nSelected)
		{
			CRect rcFocus(m_rcChips[i]);
			rcFocus.DeflateRect(2, 2);
			dc.DrawFocusRect(rcFocus);
		}
	}

	dc.SetBkMode(nOldBkMode);
	dc.SelectObject(pOldFont);
}

void CChipBar::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetFocus();
	int nChip = HitTest(point);
	if (nChip >= 0)
	{
		SetSelected(nChip);
		GetParent()->PostMessage(NM_TYPE_FILTER_CHANGED, nChip, 0);
	}

	CWnd::OnLButtonDown(nFlags, point);
}

BOOL CChipBar::PreTranslateMessage(MSG *pMsg)
{
	// Handle chip keys before the parent's list accelerators see them.
	if (pMsg->message == WM_KEYDOWN &&
		(pMsg->wParam == VK_LEFT || pMsg->wParam == VK_RIGHT ||
		 pMsg->wParam == VK_UP || pMsg->wParam == VK_DOWN ||
		 pMsg->wParam == VK_HOME || pMsg->wParam == VK_END ||
		 pMsg->wParam == VK_SPACE || pMsg->wParam == VK_RETURN))
	{
		OnKeyDown((UINT)pMsg->wParam, 1, 0);
		return TRUE;
	}
	return CWnd::PreTranslateMessage(pMsg);
}

UINT CChipBar::OnGetDlgCode()
{
	return CWnd::OnGetDlgCode() | DLGC_WANTARROWS;
}

void CChipBar::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (m_csLabels.empty())
		return;

	int selected = m_nSelected;
	switch (nChar)
	{
	case VK_LEFT:
	case VK_UP: selected = max(0, selected - 1); break;
	case VK_RIGHT:
	case VK_DOWN: selected = min((int)m_csLabels.size() - 1, selected + 1); break;
	case VK_HOME: selected = 0; break;
	case VK_END: selected = (int)m_csLabels.size() - 1; break;
	case VK_SPACE:
	case VK_RETURN: break;
	default:
		CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
		return;
	}
	SetSelected(selected);
	GetParent()->PostMessage(NM_TYPE_FILTER_CHANGED, selected, 0);
}

void CChipBar::OnSetFocus(CWnd *pOldWnd)
{
	CWnd::OnSetFocus(pOldWnd);
	Invalidate(FALSE);
}

void CChipBar::OnKillFocus(CWnd *pNewWnd)
{
	CWnd::OnKillFocus(pNewWnd);
	Invalidate(FALSE);
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
	Invalidate(FALSE);
}
