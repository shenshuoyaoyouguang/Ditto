// AutoRichEditCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "RichEditCtrlEx.h"
#include "..\Shared\TextConvert.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAutoRichEditCtrl

_AFX_RICHEDITEX_STATE _afxRichEditStateEx ;

BOOL PASCAL AfxInitRichEditEx()
{
    if( ! ::AfxInitRichEdit() )
    {
        return FALSE ;
    }

    _AFX_RICHEDITEX_STATE* l_pState = &_afxRichEditStateEx ;

    if( l_pState->m_hInstRichEdit20 == NULL )
    {
#ifdef _UNICODE 
        l_pState->m_hInstRichEdit20 = LoadLibraryW(_T("MSFTEDIT.DLL"));
#else
		l_pState->m_hInstRichEdit20 = LoadLibraryA(_T("RICHED20.DLL"));
#endif

    }

    return l_pState->m_hInstRichEdit20 != NULL ;
}


CRichEditCtrlEx::CRichEditCtrlEx()
{
}

CRichEditCtrlEx::~CRichEditCtrlEx()
{
}


BEGIN_MESSAGE_MAP(CRichEditCtrlEx, CRichEditCtrl)
	//{{AFX_MSG_MAP(CRichEditCtrlEx)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRichEditCtrlEx message handlers

CString CRichEditCtrlEx::GetRTF()
{
	// Return the RTF string of the text in the control.
	
	// Stream out here.
	EDITSTREAM es;
	es.dwError = 0;
	es.pfnCallback = CBStreamOut;		// Set the callback

	CString sRTF = "";

	es.dwCookie = (DWORD_PTR) &sRTF;	// so sRTF receives the string
	
	StreamOut(SF_RTF, es);			// Call CRichEditCtrl::StreamOut to get the string.
	///

	return sRTF;

}

void CRichEditCtrlEx::SetRTF(const char *pRTF)
{
	// Put the RTF string sRTF into the rich edit control.

	// Read the text in
	EDITSTREAM es;
	es.dwError = 0;
	es.pfnCallback = CBStreamIn;

#ifdef _UNICODE
	CString cs;
	es.dwCookie = (DWORD_PTR) &cs;
#else
	CString cs(pRTF);
	es.dwCookie = (DWORD_PTR) &cs;
#endif

	StreamIn(SF_RTF, es);	// Do it.

#ifdef _UNICODE
	SETTEXTEX stex;
	stex.flags = ST_SELECTION | ST_KEEPUNDO;
	SendMessage(EM_SETTEXTEX, (WPARAM)&stex, (LPARAM)pRTF); 
#endif

}

void CRichEditCtrlEx::SetRTF(CStringA sRTF)
{
	// Put the RTF string sRTF into the rich edit control.

	// Read the text in
	EDITSTREAM es;
	es.dwError = 0;
	es.pfnCallback = CBStreamIn;

#ifdef _UNICODE
	CString cs;
	es.dwCookie = (DWORD_PTR) &cs;
#else
	es.dwCookie = (DWORD_PTR) &sRTF;
#endif

	StreamIn(SF_RTF, es);	// Do it.

#ifdef _UNICODE
	SETTEXTEX stex;
    stex.flags = ST_SELECTION | ST_KEEPUNDO;

    SendMessage(EM_SETTEXTEX, (WPARAM)&stex, (LPARAM)sRTF.GetBuffer(sRTF.GetLength())); 
#endif

}

CString CRichEditCtrlEx::GetText()
{
	CString sText;
	
#ifdef _UNICODE
	GETTEXTEX stex;
	stex.codepage = 1200;  // Unicode code page(set SETTEXTEX documentation)

	int nSize = GetTextLength();
	//increase the size incase of unicode text
	nSize++;
	nSize = nSize * 2;
	stex.cb = nSize;

	TCHAR *pText = new TCHAR[nSize];
	if(pText)
	{
		SendMessage(EM_GETTEXTEX, (WPARAM)&stex, (LPARAM)pText); 
		sText = pText;

		delete []pText;
		pText = NULL;
	}
#else
	// Stream out here.
	EDITSTREAM es;
	es.dwError = 0;
	es.pfnCallback = CBStreamOut;		// Set the callback
	es.dwCookie = (DWORD_PTR) &sText;	// so sRTF receives the string
	StreamOut(SF_TEXT, es);			// Call CRichEditCtrl::StreamOut to get the string.
#endif

	return sText;
}

void CRichEditCtrlEx::SetText(CString sText)
{
	// Put the RTF string sRTF into the rich edit control.

	// Read the text in
	EDITSTREAM es;
	es.dwError = 0;
	es.pfnCallback = CBStreamIn;
#ifdef _UNICODE
	CString cs;
	es.dwCookie = (DWORD_PTR) &cs;
#else
	es.dwCookie = (DWORD_PTR) &sText;
#endif
	StreamIn(SF_TEXT, es);	// Do it.

#ifdef _UNICODE
	SETTEXTEX stex;
    stex.flags = ST_SELECTION | ST_KEEPUNDO;
    stex.codepage = 1200;  // Unicode code page(set SETTEXTEX documentation)
    SendMessage(EM_SETTEXTEX, (WPARAM)&stex, (LPARAM)sText.GetBuffer(sText.GetLength())); 
	sText.ReleaseBuffer();
#endif
}

/*
	Callback function to stream an RTF string into the rich edit control.
*/
DWORD CALLBACK CRichEditCtrlEx::CBStreamIn(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb)
{
	// We insert the rich text here.

/*	
	This function taken from CodeGuru.com
	http://www.codeguru.com/richedit/rtf_string_streamin.shtml
	Zafir Anjum
*/

	CString *pstr = (CString *) dwCookie;

	if (pstr->GetLength() < cb)
	{
		*pcb = pstr->GetLength();
		memcpy(pbBuff, (LPCTSTR) *pstr, *pcb);
		pstr->Empty();
	}
	else
	{
		*pcb = cb;
		memcpy(pbBuff, (LPCTSTR) *pstr, *pcb);
		*pstr = pstr->Right(pstr->GetLength() - cb);
	}
	///

	return 0;
}

/*
	Callback function to stream the RTF string out of the rich edit control.
*/
DWORD CALLBACK CRichEditCtrlEx::CBStreamOut(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb)
{
	// Address of our string var is in psEntry
	CString *psEntry = (CString*) dwCookie;
	

	CString tmpEntry = "";
	tmpEntry = (CString) pbBuff;

	// And write it!!!
	*psEntry += tmpEntry.Left(cb);

	return 0;
}

CHARFORMAT CRichEditCtrlEx::GetCharFormat(DWORD dwMask)
{
	CHARFORMAT cf;
	cf.cbSize = sizeof(CHARFORMAT);

	cf.dwMask = dwMask;

	GetSelectionCharFormat(cf);

	return cf;
}

void CRichEditCtrlEx::SetFontName(CString sFontName)
{
	CHARFORMAT cf = GetCharFormat();

	// Set the font name.
	for (int i = 0; i <= sFontName.GetLength()-1; i++)
		cf.szFaceName[i] = (char)sFontName[i];


	cf.dwMask = CFM_FACE;

	SetSelectionCharFormat(cf);
}

void CRichEditCtrlEx::SetFontSize(int nPointSize)
{
	CHARFORMAT cf = GetCharFormat();

	nPointSize *= 20;	// convert from to twips
	cf.yHeight = nPointSize;
	
	cf.dwMask = CFM_SIZE;

	SetSelectionCharFormat(cf);
}

int CRichEditCtrlEx::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CRichEditCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	// TODO: Add your specialized creation code here
	
	return 0;
}

BOOL CRichEditCtrlEx::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext) 
{
	return CWnd::Create(_T("RichEdit50W"), lpszWindowName, dwStyle, rect, pParentWnd, nID, pContext);

}
