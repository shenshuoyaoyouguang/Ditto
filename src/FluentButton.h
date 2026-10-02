#pragma once

// CFluentButton: gdi+ owner-drawn button with the three fluent variants
// (plan section 4.4). Colors come from the v4 theme tokens each draw.

class CFluentButton : public CButton
{
public:
	enum Style
	{
		Style_Accent,    // solid accent fill, text on accent
		Style_Secondary, // control fill + 1px card stroke
		Style_Subtle,    // transparent, fill on hover/press only
	};

	CFluentButton();

	void SetStyle(Style style) { m_style = style; Invalidate(FALSE); }
	void SetCornerRadius(int radius) { m_radius = radius; Invalidate(FALSE); }

protected:
	virtual void PreSubclassWindow();
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);

	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnEnable(BOOL bEnable);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);

	Style m_style;
	int m_radius;
	bool m_bHover;
	bool m_bPressed;

	DECLARE_MESSAGE_MAP()
};
