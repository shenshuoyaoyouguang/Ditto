// OptionsSheet.cpp : implementation file
//

#include "stdafx.h"
#include "CP_Main.h"
#include "OptionsSheet.h"
#include "OptionsKeyBoard.h"
#include "OptionsGeneral.h"
#include "OptionsQuickPaste.h"
#include "OptionsStats.h"
#include "OptionsTypes.h"
#include "About.h"
#include "OptionFriends.h"
#include "OptionsCopyBuffers.h"
#include "Misc.h"
#include "QuickPasteKeyboard.h"
#include "Fonts.h"
#include "DwmTheme.h"
#include "DPI.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif 


/////////////////////////////////////////////////////////////////////////////
// COptionsSheet

IMPLEMENT_DYNAMIC(COptionsSheet, CPropertySheet)

COptionsSheet::COptionsSheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
	m_themeChanged = FALSE;
	m_pKeyBoardOptions = NULL;
	m_pGeneralOptions = NULL;

	m_pCopyBuffers = NULL;
	m_pStats = NULL;
	m_pTypes = NULL;
	m_pAbout = NULL;
	m_pFriends = NULL;

	m_pQuickPasteShortCuts = NULL;
	

	m_hWndParent = NULL;
	m_navWidth = 0;
	m_bNavCreated = false;

	EnableStackedTabs(TRUE);

	m_pGeneralOptions = new COptionsGeneral;
	m_pKeyBoardOptions = new COptionsKeyBoard;
	m_pQuickPasteShortCuts = new CQuickPasteKeyboard;

	m_pCopyBuffers = new COptionsCopyBuffers;
	m_pStats = new COptionsStats;
	m_pTypes = new COptionsTypes;
	m_pAbout = new CAbout;

	AddPage(m_pGeneralOptions);
	AddPage(m_pTypes);
	AddPage(m_pKeyBoardOptions);
	AddPage(m_pCopyBuffers);

	AddPage(m_pQuickPasteShortCuts);
	if(CGetSetOptions::GetAllowFriends())
	{
		m_pFriends = new COptionFriends;
		AddPage(m_pFriends);
	}
	AddPage(m_pStats);
	AddPage(m_pAbout);

	
}

COptionsSheet::~COptionsSheet()
{
	delete m_pGeneralOptions;
	delete m_pKeyBoardOptions;
	delete m_pCopyBuffers;
	delete m_pStats;
	delete m_pTypes;
	delete m_pAbout;	
	delete m_pFriends;

	delete m_pQuickPasteShortCuts;	
}

#define IDC_NAV_LIST 0x37F

BEGIN_MESSAGE_MAP(COptionsSheet, CPropertySheet)
	//{{AFX_MSG_MAP(COptionsSheet)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	ON_WM_DESTROY()
	ON_WM_NCDESTROY()
	ON_WM_DRAWITEM()
	ON_WM_MEASUREITEM()
	ON_LBN_SELCHANGE(IDC_NAV_LIST, OnNavSelect)
	ON_WM_SIZE()
	//ON_WM_CLOSE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COptionsSheet message handlers

void COptionsSheet::OnDestroy()
{
	CPropertySheet::OnDestroy();
}

void COptionsSheet::SetNotifyWnd(HWND hWnd)
{
	m_hWndParent = hWnd;
}

BOOL COptionsSheet::OnInitDialog()
{
	m_bModeless = FALSE;
	m_nFlags |= WF_CONTINUEMODAL;

	HICON b = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_MAINFRAME), IMAGE_ICON, 64, 64, LR_SHARED);
	SetIcon(b, TRUE);

	BOOL bResult = CPropertySheet::OnInitDialog();

	SetWindowText(_T("Options"));

	theApp.m_Language.UpdateOptionsSheet(this);

	::ShowWindow(::GetDlgItem(m_hWnd, ID_APPLY_NOW), SW_HIDE);

	// left navigation shell instead of stacked tabs (plan section 5.6)
	CRect rcWindow;
	GetWindowRect(rcWindow);

	CDPI dpi;
	dpi.SetHwnd(m_hWnd);
	m_navWidth = dpi.Scale(180);

	if (GetTabControl() != NULL)
	{
		GetTabControl()->ShowWindow(SW_HIDE);
	}

	m_nav.Create(WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_OWNERDRAWVARIABLE | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | LBS_HASSTRINGS,
		CRect(0, 0, m_navWidth, rcWindow.Height()), this, IDC_NAV_LIST);
	m_nav.SetFont(AppFonts::Inst().Get(Font_Body));
	m_bNavCreated = true;
	FillNavItems();

	// widen the sheet and push the pages right of the nav column
	SetWindowPos(NULL, 0, 0, rcWindow.Width() + m_navWidth, rcWindow.Height(),
		SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

	int pageCount = (int)GetPageCount();
	for (int i = 0; i < pageCount; i++)
	{
		CPropertyPage* pPage = GetPage(i);
		CRect rcPage;
		pPage->GetWindowRect(rcPage);
		ScreenToClient(rcPage);
		pPage->MoveWindow(rcPage.left + m_navWidth, rcPage.top, rcPage.Width(), rcPage.Height());
	}

	m_nav.SetCurSel(GetActiveIndex());

	// fluent window chrome (no custom frame rework on the property sheet)
	DwmTheme::ApplyRoundedCorners(m_hWnd, true);
	DwmTheme::ApplyDarkCaption(m_hWnd, CGetSetOptions::m_Theme.IsDarkTheme());

	m_bModeless = TRUE;
	m_nFlags &= ~WF_CONTINUEMODAL;

	return bResult;
}

void COptionsSheet::FillNavItems()
{
	m_nav.ResetContent();

	// labels must match the AddPage order in the constructor
	CString csTitles[8];
	csTitles[0] = theApp.m_Language.GetString(_T("GeneralTitle"), _T("General"));
	csTitles[1] = theApp.m_Language.GetString(_T("SupportedTypesTitle"), _T("Supported Types"));
	csTitles[2] = theApp.m_Language.GetString(_T("KeyboardShortcutsTitle"), _T("Keyboard Shortcuts"));
	csTitles[3] = theApp.m_Language.GetString(_T("CopyBuffersTitle"), _T("Copy Buffers"));
	csTitles[4] = theApp.m_Language.GetString(_T("QuickPasteKeyboardTitle"), _T("Quick Paste Keyboard"));

	int index = 5;
	if (m_pFriends != NULL)
	{
		csTitles[index++] = theApp.m_Language.GetString(_T("FriendsTitle"), _T("Friends"));
	}
	csTitles[index++] = theApp.m_Language.GetString(_T("StatsTitle"), _T("Stats"));
	csTitles[index++] = theApp.m_Language.GetString(_T("AboutTitle"), _T("About Ditto"));

	for (int i = 0; i < index; i++)
	{
		m_nav.AddString(csTitles[i]);
	}
}

void COptionsSheet::LayoutNav(int cx, int cy)
{
	if (m_bNavCreated == false)
		return;

	m_nav.MoveWindow(0, 0, m_navWidth, cy);
}

void COptionsSheet::OnNavSelect()
{
	int sel = m_nav.GetCurSel();
	if (sel >= 0 && sel < (int)GetPageCount() && sel != GetActiveIndex())
	{
		SetActivePage(sel);
	}
}

void COptionsSheet::OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
	if (lpMeasureItemStruct->CtlType == ODT_LISTBOX)
	{
		CDPI dpi;
		dpi.SetHwnd(m_hWnd);
		lpMeasureItemStruct->itemHeight = dpi.Scale(34);
	}
}

void COptionsSheet::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	if (lpDrawItemStruct == NULL ||
		lpDrawItemStruct->CtlType != ODT_LISTBOX ||
		lpDrawItemStruct->hwndItem != m_nav.GetSafeHwnd())
	{
		CPropertySheet::OnDrawItem(nIDCtl, lpDrawItemStruct);
		return;
	}

	CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
	CRect rc(lpDrawItemStruct->rcItem);
	CTheme& theme = CGetSetOptions::m_Theme;

	bool bSelected = (lpDrawItemStruct->itemState & ODS_SELECTED) != 0;
	bool bFocused = (lpDrawItemStruct->itemState & ODS_FOCUS) != 0;

	CRect rcClient;
	m_nav.GetClientRect(rcClient);
	ScreenToClient(rcClient);

	// the nav column paints as one surface; the full-width item background
	// plus the active accent bar read as a single selection pill
	pDC->FillSolidRect(rc, bSelected ? theme.AccentSubtle() : theme.SurfaceBase());

	if (bSelected)
	{
		CRect rcBar(rc.left + 4, rc.top + 6, rc.left + 7, rc.bottom - 6);
		pDC->FillSolidRect(rcBar, theme.AccentDefault());
	}

	CString csText;
	m_nav.GetText(lpDrawItemStruct->itemID, csText);

	pDC->SetBkMode(TRANSPARENT);
	pDC->SetTextColor(bSelected ? theme.TextPrimary() : theme.TextSecondary());
	CFont* pOld = pDC->SelectObject(bSelected ? AppFonts::Inst().Get(Font_BodyStrong) : AppFonts::Inst().Get(Font_Body));
	pDC->DrawText(csText, rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
	pDC->SelectObject(pOld);

	if (bFocused)
	{
		// keyboard focus ring on the nav entry
		CRect rcFocus(rc);
		rcFocus.DeflateRect(2, 2, 2, 2);
		CPen pen(PS_SOLID, 1, theme.AccentDefault());
		CPen* pOldPen = pDC->SelectObject(&pen);
		pDC->SelectStockObject(NULL_BRUSH);
		pDC->Rectangle(rcFocus);
		pDC->SelectObject(pOldPen);
	}
}

void COptionsSheet::OnSize(UINT nType, int cx, int cy)
{
	CPropertySheet::OnSize(nType, cx, cy);
	LayoutNav(cx, cy);
}

void COptionsSheet::OnNcDestroy()
{
	CPropertySheet::OnNcDestroy();
	::PostMessage(m_hWndParent, WM_OPTIONS_CLOSED, m_themeChanged, 0);
}
