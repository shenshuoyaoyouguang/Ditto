#include "stdafx.h"
#include "FluentDialog.h"
#include "Fonts.h"
#include "Options.h"

// ------------------------------------------------------------------
// CFluentDialog

CFluentDialog::CFluentDialog()
	: CDialog()
	, m_pResizer(NULL)
{
}

CFluentDialog::CFluentDialog(UINT nIDTemplate, CWnd* pParentWnd)
	: CDialog(nIDTemplate, pParentWnd)
	, m_pResizer(NULL)
{
}

CFluentDialog::CFluentDialog(LPCTSTR lpszTemplateName, CWnd* pParentWnd)
	: CDialog(lpszTemplateName, pParentWnd)
	, m_pResizer(NULL)
{
}

IMPLEMENT_DYNAMIC(CFluentDialog, CDialog)

BEGIN_MESSAGE_MAP(CFluentDialog, CDialog)
	ON_WM_ERASEBKGND()
	ON_WM_CTLCOLOR()
	ON_WM_SIZE()
	ON_MESSAGE(WM_DPICHANGED, OnDpiChanged)
END_MESSAGE_MAP()

BOOL CFluentDialog::OnInitDialog()
{
	BOOL result = CDialog::OnInitDialog();
	ApplyFluentStyle();
	return result;
}

void CFluentDialog::ApplyFluentStyle()
{
	AppFonts::Inst().EnsureInitialized();
	AppFonts::ApplyToChildren(this);

	CTheme& theme = CGetSetOptions::m_Theme;
	m_brBackground.DeleteObject();
	m_brBackground.CreateSolidBrush(theme.SurfaceBase());
}

BOOL CFluentDialog::OnEraseBkgnd(CDC* pDC)
{
	if (m_brBackground.GetSafeHandle() != NULL)
	{
		CRect rcClient;
		GetClientRect(rcClient);
		pDC->FillRect(rcClient, &m_brBackground);
		return TRUE;
	}

	return CDialog::OnEraseBkgnd(pDC);
}

HBRUSH CFluentDialog::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	CTheme& theme = CGetSetOptions::m_Theme;

	switch (nCtlColor)
	{
	case CTLCOLOR_DLG:
		if (m_brBackground.GetSafeHandle() != NULL)
			return (HBRUSH)m_brBackground.GetSafeHandle();
		break;

	case CTLCOLOR_STATIC:
		pDC->SetTextColor(theme.TextPrimary());
		pDC->SetBkMode(TRANSPARENT);
		if (m_brBackground.GetSafeHandle() != NULL)
			return (HBRUSH)m_brBackground.GetSafeHandle();
		break;

	default:
		break;
	}

	return CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
}

void CFluentDialog::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	if (m_pResizer != NULL && nType != SIZE_MINIMIZED)
		m_pResizer->MoveControls(CSize(cx, cy));
}

LRESULT CFluentDialog::OnDpiChanged(WPARAM wParam, LPARAM lParam)
{
	UINT newDpi = HIWORD(wParam);
	RECT* pSuggested = (RECT*)lParam;

	CRect rcOldClient;
	GetClientRect(rcOldClient);

	SetWindowPos(NULL, pSuggested->left, pSuggested->top,
		pSuggested->right - pSuggested->left, pSuggested->bottom - pSuggested->top,
		SWP_NOZORDER | SWP_NOACTIVATE);

	CRect rcNewClient;
	GetClientRect(rcNewClient);
	RescaleChildren(rcOldClient, rcNewClient);

	AppFonts::Inst().Init(newDpi, CGetSetOptions::m_Theme.FontFamily());
	AppFonts::ApplyToChildren(this);

	return 0;
}

void CFluentDialog::RescaleChildren(const CRect& rcOldClient, const CRect& rcNewClient)
{
	int oldWidth = max(1, rcOldClient.Width());
	int oldHeight = max(1, rcOldClient.Height());

	for (CWnd* pChild = GetWindow(GW_CHILD); pChild != NULL; pChild = pChild->GetNextWindow())
	{
		CRect rc;
		pChild->GetWindowRect(rc);
		ScreenToClient(rc);

		CRect rcScaled;
		rcScaled.left = rcNewClient.left + MulDiv(rc.left - rcOldClient.left, rcNewClient.Width(), oldWidth);
		rcScaled.top = rcNewClient.top + MulDiv(rc.top - rcOldClient.top, rcNewClient.Height(), oldHeight);
		rcScaled.right = rcNewClient.left + MulDiv(rc.right - rcOldClient.left, rcNewClient.Width(), oldWidth);
		rcScaled.bottom = rcNewClient.top + MulDiv(rc.bottom - rcOldClient.top, rcNewClient.Height(), oldHeight);

		pChild->MoveWindow(rcScaled);
	}

	if (m_pResizer != NULL)
		m_pResizer->MoveControls(CSize(rcNewClient.Width(), rcNewClient.Height()));
}

// ------------------------------------------------------------------
// CFluentPropertyPage

CFluentPropertyPage::CFluentPropertyPage()
	: CPropertyPage()
{
}

CFluentPropertyPage::CFluentPropertyPage(UINT nIDTemplate, UINT nIDCaption, DWORD dwSize)
	: CPropertyPage(nIDTemplate, nIDCaption, dwSize)
{
}

IMPLEMENT_DYNAMIC(CFluentPropertyPage, CPropertyPage)

BEGIN_MESSAGE_MAP(CFluentPropertyPage, CPropertyPage)
	ON_WM_ERASEBKGND()
	ON_WM_CTLCOLOR()
	ON_MESSAGE(WM_DPICHANGED, OnDpiChanged)
END_MESSAGE_MAP()

BOOL CFluentPropertyPage::OnInitDialog()
{
	BOOL result = CPropertyPage::OnInitDialog();
	ApplyFluentStyle();
	return result;
}

void CFluentPropertyPage::ApplyFluentStyle()
{
	AppFonts::Inst().EnsureInitialized();
	AppFonts::ApplyToChildren(this);

	CTheme& theme = CGetSetOptions::m_Theme;
	m_brBackground.DeleteObject();
	m_brBackground.CreateSolidBrush(theme.SurfaceBase());
}

BOOL CFluentPropertyPage::OnEraseBkgnd(CDC* pDC)
{
	if (m_brBackground.GetSafeHandle() != NULL)
	{
		CRect rcClient;
		GetClientRect(rcClient);
		pDC->FillRect(rcClient, &m_brBackground);
		return TRUE;
	}

	return CPropertyPage::OnEraseBkgnd(pDC);
}

HBRUSH CFluentPropertyPage::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	CTheme& theme = CGetSetOptions::m_Theme;

	switch (nCtlColor)
	{
	case CTLCOLOR_DLG:
		if (m_brBackground.GetSafeHandle() != NULL)
			return (HBRUSH)m_brBackground.GetSafeHandle();
		break;

	case CTLCOLOR_STATIC:
		pDC->SetTextColor(theme.TextPrimary());
		pDC->SetBkMode(TRANSPARENT);
		if (m_brBackground.GetSafeHandle() != NULL)
			return (HBRUSH)m_brBackground.GetSafeHandle();
		break;

	default:
		break;
	}

	return CPropertyPage::OnCtlColor(pDC, pWnd, nCtlColor);
}

LRESULT CFluentPropertyPage::OnDpiChanged(WPARAM wParam, LPARAM lParam)
{
	UINT newDpi = HIWORD(wParam);
	RECT* pSuggested = (RECT*)lParam;

	CRect rcOldClient;
	GetClientRect(rcOldClient);

	SetWindowPos(NULL, pSuggested->left, pSuggested->top,
		pSuggested->right - pSuggested->left, pSuggested->bottom - pSuggested->top,
		SWP_NOZORDER | SWP_NOACTIVATE);

	CRect rcNewClient;
	GetClientRect(rcNewClient);
	RescaleChildren(rcOldClient, rcNewClient);

	AppFonts::Inst().Init(newDpi, CGetSetOptions::m_Theme.FontFamily());
	AppFonts::ApplyToChildren(this);

	return 0;
}

void CFluentPropertyPage::RescaleChildren(const CRect& rcOldClient, const CRect& rcNewClient)
{
	int oldWidth = max(1, rcOldClient.Width());
	int oldHeight = max(1, rcOldClient.Height());

	for (CWnd* pChild = GetWindow(GW_CHILD); pChild != NULL; pChild = pChild->GetNextWindow())
	{
		CRect rc;
		pChild->GetWindowRect(rc);
		ScreenToClient(rc);

		CRect rcScaled;
		rcScaled.left = rcNewClient.left + MulDiv(rc.left - rcOldClient.left, rcNewClient.Width(), oldWidth);
		rcScaled.top = rcNewClient.top + MulDiv(rc.top - rcOldClient.top, rcNewClient.Height(), oldHeight);
		rcScaled.right = rcNewClient.left + MulDiv(rc.right - rcOldClient.left, rcNewClient.Width(), oldWidth);
		rcScaled.bottom = rcNewClient.top + MulDiv(rc.bottom - rcOldClient.top, rcNewClient.Height(), oldHeight);

		pChild->MoveWindow(rcScaled);
	}
}
