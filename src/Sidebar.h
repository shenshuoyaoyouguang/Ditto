#pragma once

#include <vector>
#include "DPI.h"

// Docked left sidebar of the quick paste window (ui-redesign).
// Top: main navigation rows. Middle: the group tree is docked here.
// Bottom: fixed entries (friends sync).
class CSidebar : public CWnd
{
	DECLARE_DYNAMIC(CSidebar)

public:
	CSidebar();

	BOOL Create(CWnd *pParent, UINT nID, CDPI *dpi);
	void SetDpiInfo(CDPI *dpi);
	void SetNavItems(const std::vector<CString> &csLabels);
	void SetBottomItems(const std::vector<CString> &csLabels);
	void SetSelectedNav(int nIndex);
	int GetSelectedNav() const { return m_nSelectedNav; }

	int GetWidth();
	CRect GetTreeRect();

protected:
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);

	DECLARE_MESSAGE_MAP()

	void Layout();
	int HitTest(CPoint point, bool &bBottom);
	void DrawRow(CDC *pDC, const CRect &rc, const CString &csLabel, bool bSelected);

	std::vector<CString> m_csNavLabels;
	std::vector<CRect> m_rcNav;
	std::vector<CString> m_csBottomLabels;
	std::vector<CRect> m_rcBottom;
	CString m_csGroupLabel;
	CRect m_rcTree;
	int m_nSelectedNav;
	CDPI *m_dpi;
};
