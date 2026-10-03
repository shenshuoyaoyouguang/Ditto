// FileTransferProgressDlg.cpp : implementation file
//

#include "stdafx.h"
#include "cp_main.h"
#include "FileTransferProgressDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFileTransferProgressDlg dialog


CFileTransferProgressDlg::CFileTransferProgressDlg(CWnd* pParent /*=NULL*/)
	: CFluentDialog(CFileTransferProgressDlg::IDD, pParent)
{
	m_bCancelled = false;
}

void CFileTransferProgressDlg::DoDataExchange(CDataExchange* pDX)
{
	CFluentDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFileTransferProgressDlg)
	DDX_Control(pDX, IDC_FILE_COPY, m_FileCopy);
	DDX_Control(pDX, IDCANCEL, m_m_CancelButton);
	DDX_Control(pDX, IDC_PROGRESS_FILE, m_ProgressSingleFile);
	DDX_Control(pDX, IDC_PROGRESS_ALL_FILES, m_ProgressAllFiles);
	DDX_Control(pDX, IDC_STATIC_1, m_Message);
	DDX_Control(pDX, IDC_STATIC_2, m_CopyingFile);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFileTransferProgressDlg, CFluentDialog)
	//{{AFX_MSG_MAP(CFileTransferProgressDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFileTransferProgressDlg message handlers

void CFileTransferProgressDlg::PostNcDestroy() 
{
	CFluentDialog::PostNcDestroy();

	delete this;
}

BOOL CFileTransferProgressDlg::OnInitDialog()
{
	CFluentDialog::OnInitDialog();

	// the old avi animation is replaced by a themed progress bar at the
	// same spot; the animate control from the template stays hidden
	m_FileCopy.ShowWindow(SW_HIDE);

	// m_progressBar already carries the single-file progress (same 0-100 value
	// from SetSingleFilePos), so the template's IDC_PROGRESS_FILE would be a
	// second, identical bar right next to it. Hide it too.
	m_ProgressSingleFile.ShowWindow(SW_HIDE);

	CRect rcAvi;
	m_FileCopy.GetWindowRect(rcAvi);
	ScreenToClient(rcAvi);

	CDPI dpi;
	dpi.SetHwnd(m_hWnd);
	rcAvi.DeflateRect(0, dpi.Scale(4), 0, dpi.Scale(4));

	m_progressBar.Create(WS_CHILD | WS_VISIBLE, rcAvi, this, 0x380);

	SetNumFiles(0);

	::SetWindowPos(m_hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE|SWP_SHOWWINDOW);

	return TRUE;
}

void CFileTransferProgressDlg::OnCancel() 
{
	m_bCancelled = true;
}

void CFileTransferProgressDlg::SetMessage(CString &cs)
{
	m_Message.SetWindowText(cs);
}

CString CFileTransferProgressDlg::GetMessage()
{
	CString cs;
	m_Message.GetWindowText(cs);
	return cs;
}

void CFileTransferProgressDlg::SetFileMessage(CString &cs)
{
	m_CopyingFile.SetWindowText(cs);
}

void CFileTransferProgressDlg::SetNumFiles(int nFiles)
{
	m_ProgressAllFiles.SetRange32(0, nFiles);
	m_ProgressAllFiles.SetStep(1);
	m_ProgressAllFiles.SetPos(0);

	m_ProgressSingleFile.SetRange32(0, 100);
	m_ProgressSingleFile.SetStep(1);
	m_ProgressSingleFile.SetPos(0);

	// the fluent bar must own its range explicitly -- it defaults to 0..100, and
	// OnPaint derives the fill fraction from GetRange/GetPos, so a changed range
	// would silently misplace the fill
	m_progressBar.SetRange32(0, 100);
	m_progressBar.SetStep(1);
	m_progressBar.SetPos(0);

}


void CFileTransferProgressDlg::StepAllFiles()
{
	m_ProgressAllFiles.StepIt();
}

void CFileTransferProgressDlg::SetSingleFilePos(int nPos)
{
	m_ProgressSingleFile.SetPos(nPos);
	m_progressBar.SetPos(nPos);
}

void CFileTransferProgressDlg::PumpMessages()
{
	int nLoops = 0;
	MSG msg;
    while(::PeekMessage(&msg, m_hWnd, 0, 0, PM_REMOVE)) 
    {
        TranslateMessage(&msg);
		DispatchMessage(&msg);

		nLoops++;
		if(nLoops > 100)
			break;
    }
}
