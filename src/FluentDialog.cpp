#include "stdafx.h"
#include "FluentDialog.h"
#include "Fonts.h"
#include "Options.h"

// ------------------------------------------------------------------
// CFluentDialog

CFluentDialog::CFluentDialog()
	: CDialog()
	, m_pResizer(NULL)
	, m_bRescaling(false)
{
}

CFluentDialog::CFluentDialog(UINT nIDTemplate, CWnd* pParentWnd)
	: CDialog(nIDTemplate, pParentWnd)
	, m_pResizer(NULL)
	, m_bRescaling(false)
{
}

CFluentDialog::CFluentDialog(LPCTSTR lpszTemplateName, CWnd* pParentWnd)
	: CDialog(lpszTemplateName, pParentWnd)
	, m_pResizer(NULL)
	, m_bRescaling(false)
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
	WireFluentButtons();
	ApplyFluentStyle();
	return result;
}

// The plan's uniform dialog action (section 5.7): the template's standard
// OK / Cancel buttons become the fluent owner-draw variants. Controls a
// derived dialog already owns (DDX_Control or its own subclassing, e.g.
// FileTransferProgressDlg's cancel button) are left alone.
void CFluentDialog::WireFluentButtons()
{
	struct Wiring
	{
		CFluentButton& button;
		UINT id;
		CFluentButton::Style style;
	};

	const Wiring wirings[] =
	{
		{ m_btnOk,		IDOK,		CFluentButton::Style_Accent },
		{ m_btnCancel,	IDCANCEL,	CFluentButton::Style_Secondary },
	};

	for (const Wiring& wiring : wirings)
	{
		HWND hwnd = ::GetDlgItem(GetSafeHwnd(), wiring.id);
		if (hwnd == NULL || CWnd::FromHandlePermanent(hwnd) != NULL)
			continue;

		if (wiring.button.SubclassDlgItem(wiring.id, this))
			wiring.button.SetStyle(wiring.style);
	}
}

void CFluentDialog::ApplyFluentStyle()
{
	AppFonts::Inst().EnsureInitialized();
	AppFonts::ApplyToChildren(this);

	CTheme& theme = CGetSetOptions::m_Theme;
	m_brBackground.DeleteObject();
	m_brBackground.CreateSolidBrush(theme.SurfaceBase());

	// List boxes and edit controls paint their own background, so they need a
	// brush of their own -- returning the dialog brush would make them
	// indistinguishable from the dialog.
	m_brControl.DeleteObject();
	m_brControl.CreateSolidBrush(theme.SurfaceElevated());
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

	case CTLCOLOR_LISTBOX:
	case CTLCOLOR_EDIT:
		// These actually send WM_CTLCOLOR (unlike the common controls such as
		// SysTreeView32 / SysDateTimePick32, which never do and therefore kept
		// their system colours on a themed dialog).
		// WM_CTLCOLOR does not carry the enabled state, so a disabled edit or
		// combo (IDC_EDIT_CLIP_TITLE / IDC_COMBO_DATA_FORMAT in the delete clip
		// data dialog are WS_DISABLED) would keep the full strength text colour
		// and lose the greyed-out affordance.
		pDC->SetTextColor(pWnd != NULL && pWnd->IsWindowEnabled()
			? theme.TextPrimary()
			: theme.TextDisabled());
		pDC->SetBkColor(theme.SurfaceElevated());
		if (m_brControl.GetSafeHandle() != NULL)
			return (HBRUSH)m_brControl.GetSafeHandle();
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
	if (pSuggested == NULL)
		return 0;

	CRect rcOldClient;
	GetClientRect(rcOldClient);

	// SetWindowPos dispatches WM_SIZE synchronously, which runs the derived
	// OnSize and its own CDialogResizer pass; RescaleChildren below then scales
	// every child proportionally. Two passes for one dpi change displaces or
	// double-grows anchored and stretched controls, so flag the rescale and let
	// derived handlers skip their own.
	m_bRescaling = true;

	SetWindowPos(NULL, pSuggested->left, pSuggested->top,
		pSuggested->right - pSuggested->left, pSuggested->bottom - pSuggested->top,
		SWP_NOZORDER | SWP_NOACTIVATE);

	CRect rcNewClient;
	GetClientRect(rcNewClient);
	RescaleChildren(rcOldClient, rcNewClient);

	AppFonts::Inst().Init(newDpi, CGetSetOptions::m_Theme.FontFamily());
	AppFonts::ApplyToChildren(this);

	m_bRescaling = false;

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
	, m_bRescaling(false)
{
}

CFluentPropertyPage::CFluentPropertyPage(UINT nIDTemplate, UINT nIDCaption, DWORD dwSize)
	: CPropertyPage(nIDTemplate, nIDCaption, dwSize)
	, m_bRescaling(false)
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

	m_brControl.DeleteObject();
	m_brControl.CreateSolidBrush(theme.SurfaceElevated());
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

	case CTLCOLOR_LISTBOX:
	case CTLCOLOR_EDIT:
		// WM_CTLCOLOR does not carry the enabled state, so a disabled edit or
		// combo (IDC_EDIT_CLIP_TITLE / IDC_COMBO_DATA_FORMAT in the delete clip
		// data dialog are WS_DISABLED) would keep the full strength text colour
		// and lose the greyed-out affordance.
		pDC->SetTextColor(pWnd != NULL && pWnd->IsWindowEnabled()
			? theme.TextPrimary()
			: theme.TextDisabled());
		pDC->SetBkColor(theme.SurfaceElevated());
		if (m_brControl.GetSafeHandle() != NULL)
			return (HBRUSH)m_brControl.GetSafeHandle();
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
	if (pSuggested == NULL)
		return 0;

	CRect rcOldClient;
	GetClientRect(rcOldClient);

	m_bRescaling = true;

	SetWindowPos(NULL, pSuggested->left, pSuggested->top,
		pSuggested->right - pSuggested->left, pSuggested->bottom - pSuggested->top,
		SWP_NOZORDER | SWP_NOACTIVATE);

	CRect rcNewClient;
	GetClientRect(rcNewClient);
	RescaleChildren(rcOldClient, rcNewClient);

	AppFonts::Inst().Init(newDpi, CGetSetOptions::m_Theme.FontFamily());
	AppFonts::ApplyToChildren(this);

	m_bRescaling = false;

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
