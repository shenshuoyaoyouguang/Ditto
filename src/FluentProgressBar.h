#pragma once

#include "DPI.h"

// CFluentProgressBar: gdi+ progress bar aligned with the theme tokens
// (plan section 5.7) - replaces the old avi animation control.
class CFluentProgressBar : public CProgressCtrl
{
public:
	CFluentProgressBar();

	BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);

	// Indeterminate mode: the bar is fully owner drawn, so this drives its own
	// animation instead of relying on the native PBS_MARQUEE repaint. Used while
	// the connection is being opened and there is no determinate progress yet.
	void SetMarquee(bool bEnable);
	bool IsMarquee() const { return m_bMarquee; }

protected:
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnTimer(UINT_PTR nEvent);
	afx_msg void OnDestroy();

	void DrawMarqueeSegment(Graphics& graphics, const CRect& rc);

	DECLARE_MESSAGE_MAP()

	enum { MARQUEE_TIMER_ID = 1 };

	bool m_bMarquee;
	int m_nMarqueePos;
};
