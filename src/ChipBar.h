#pragma once

#include "DPI.h"

#define NM_CHIP_CLICKED		(WM_USER + 0x140)	// wParam = chip index

enum ChipFilter
{
	CHIP_ALL = 0,
	CHIP_TEXT,
	CHIP_IMAGE,
	CHIP_FILE,
	CHIP_LINK,
	CHIP_COUNT
};

// CChipBar: owner drawn type filter chip row (plan section 5.5).
// Sends NM_CHIP_CLICKED to the parent with the chip index.
class CChipBar : public CWnd
{
public:
	CChipBar();

	BOOL Create(CWnd* pParent, UINT nID);
	void SetDpiInfo(CDPI* dpi) { m_dpi = dpi; }
	void SetLabels(const CString csLabels[CHIP_COUNT]);
	void SetActive(int index) { m_active = index; Invalidate(FALSE); }
	void SetHoverEnabled(bool enable) { m_bHoverEnabled = enable; }

	int HitTestChip(CPoint point);

protected:
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();

	void LayoutChips();

	CDPI* m_dpi;
	CString m_labels[CHIP_COUNT];
	CRect m_chips[CHIP_COUNT];
	bool m_bChipsLaidOut;
	int m_active;
	int m_hover;
	bool m_bHoverTracked;
	bool m_bHoverEnabled;

	DECLARE_MESSAGE_MAP()
};
