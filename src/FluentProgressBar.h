#pragma once

#include "DPI.h"

// CFluentProgressBar: gdi+ progress bar aligned with the theme tokens
// (plan section 5.7) - replaces the old avi animation control.
class CFluentProgressBar : public CProgressCtrl
{
public:
	CFluentProgressBar();

	BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);

protected:
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);

	DECLARE_MESSAGE_MAP()
};
