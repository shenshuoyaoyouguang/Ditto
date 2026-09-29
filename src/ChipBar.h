#pragma once

#include <vector>
#include "DPI.h"

// Pill style type filter chips shown under the search box (ui-redesign).
// The first chip is "All", selecting it clears the type filter.
class CChipBar : public CWnd
{
	DECLARE_DYNAMIC(CChipBar)

public:
	CChipBar();

	void Create(CWnd *pParent, UINT nID, CDPI *dpi);
	void SetDpiInfo(CDPI *dpi);

	void SetChips(const std::vector<CString> &csLabels);
	void SetChipCount(int nIndex, int nCount);
	void SetSelected(int nIndex);
	int GetSelected() const { return m_nSelected; }

	int GetBarHeight(int width = -1);
	virtual BOOL PreTranslateMessage(MSG *pMsg);

protected:
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg UINT OnGetDlgCode();
	afx_msg void OnSetFocus(CWnd *pOldWnd);
	afx_msg void OnKillFocus(CWnd *pNewWnd);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();
	afx_msg void OnSize(UINT nType, int cx, int cy);

	DECLARE_MESSAGE_MAP()

	int Layout(int width = -1);
	int HitTest(CPoint point);
	void GetChipColors(COLORREF &crBG, COLORREF &crText, int nIndex) const;

	std::vector<CString> m_csLabels;
	std::vector<CRect> m_rcChips;
	std::vector<int> m_nCounts;
	int m_nSelected;
	int m_nHoverChip;
	CDPI *m_dpi;
	bool m_bTrackingLeave;
};
