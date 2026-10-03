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

	// Force the chip geometry to be recomputed on the next paint. Needed after a
	// dpi change, where MoveWindow may be a no-op (same rect) and therefore never
	// delivers the WM_SIZE that would otherwise drop the cache.
	void InvalidateLayout() { m_bChipsLaidOut = false; Invalidate(FALSE); }

	int HitTestChip(CPoint point);

protected:
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();
	// The chip layout is cached in m_bChipsLaidOut and only reset by the
	// constructor and SetLabels, so without ON_WM_SIZE a dpi change or any
	// resize left the chips at the old scale and the hit rects stale.
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSetFocus(CWnd* pPrevWnd);
	afx_msg void OnKillFocus(CWnd* pNextWnd);
	afx_msg LRESULT OnGetDlgCode();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCount, UINT nFlags);

	void LayoutChips();
	void MoveActiveTo(int index);

	CDPI* m_dpi;
	CString m_labels[CHIP_COUNT];
	CRect m_chips[CHIP_COUNT];
	bool m_bChipsLaidOut;
	int m_active;
	int m_hover;
	bool m_bHasFocus;
	bool m_bHoverTracked;
	bool m_bHoverEnabled;

	DECLARE_MESSAGE_MAP()
};
