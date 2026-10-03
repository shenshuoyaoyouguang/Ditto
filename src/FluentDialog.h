#pragma once

#include "DialogResizer.h"
#include "FluentButton.h"

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
	void WireFluentButtons();
	void RescaleChildren(const CRect& rcOldClient, const CRect& rcNewClient);

	// True while OnDpiChanged is driving the resize. A dpi change resizes the
	// window, which synchronously dispatches WM_SIZE and runs the derived OnSize
	// (its own CDialogResizer pass), and then RescaleChildren scales every child
	// proportionally -- two layout passes for one dpi change, which displaces or
	// double-grows anchored and stretched controls. Derived OnSize handlers skip
	// their own pass while this is set.
	bool IsRescaling() const { return m_bRescaling; }

	CBrush m_brBackground;
	CBrush m_brControl;
	CDialogResizer* m_pResizer;
	bool m_bRescaling;

	// the fluent-styled standard buttons (plan section 5.7), subclassed onto
	// the template's IDOK / IDCANCEL when present and not already owned
	CFluentButton m_btnOk;
	CFluentButton m_btnCancel;

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

	bool IsRescaling() const { return m_bRescaling; }

	CBrush m_brBackground;
	CBrush m_brControl;
	bool m_bRescaling;

	DECLARE_MESSAGE_MAP()
};
