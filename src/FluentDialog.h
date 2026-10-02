#pragma once

#include "DialogResizer.h"

// CFluentDialog / CFluentPropertyPage: themed dialog bases (plan section 4.3).
// Subclasses switch their base class and keep ddx unchanged; they get
//  - the app font pushed onto all children
//  - theme background / static text colors
//  - per-monitor dpi rescaling of the whole layout
//  - an optional resize hook via SetResizer (subclass owns the CDialogResizer)

class CFluentDialog : public CDialog
{
	DECLARE_DYNAMIC(CFluentDialog)

public:
	CFluentDialog();
	CFluentDialog(UINT nIDTemplate, CWnd* pParentWnd = NULL);
	CFluentDialog(LPCTSTR lpszTemplateName, CWnd* pParentWnd = NULL);

	void SetResizer(CDialogResizer* pResizer) { m_pResizer = pResizer; }

protected:
	virtual BOOL OnInitDialog();

	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg LRESULT OnDpiChanged(WPARAM wParam, LPARAM lParam);

	void ApplyFluentStyle();
	void RescaleChildren(const CRect& rcOldClient, const CRect& rcNewClient);

	CBrush m_brBackground;
	CDialogResizer* m_pResizer;

	DECLARE_MESSAGE_MAP()
};

class CFluentPropertyPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CFluentPropertyPage)

public:
	CFluentPropertyPage();
	CFluentPropertyPage(UINT nIDTemplate, UINT nIDCaption = 0, DWORD dwSize = sizeof(PROPSHEETPAGE));

protected:
	virtual BOOL OnInitDialog();

	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg LRESULT OnDpiChanged(WPARAM wParam, LPARAM lParam);

	void ApplyFluentStyle();
	void RescaleChildren(const CRect& rcOldClient, const CRect& rcNewClient);

	CBrush m_brBackground;

	DECLARE_MESSAGE_MAP()
};
