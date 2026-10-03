// DialogResizer.h: interface for the CDialogResizer class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DIALOGRESIZER_H__DA9AF3FF_C6CC_4D70_965A_4216A0EC9E75__INCLUDED_)
#define AFX_DIALOGRESIZER_H__DA9AF3FF_C6CC_4D70_965A_4216A0EC9E75__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <afxtempl.h>

#define DR_MoveLeft			 1
#define DR_MoveTop			 2
#define DR_SizeWidth		 4
#define DR_SizeHeight		 8

class CDialogResizer  
{
public:
	CDialogResizer();
	virtual ~CDialogResizer();

protected:
	class CDR_Data
	{
	public:
		CDR_Data()
		{			
			m_nFlags = 0;
		}
		HWND m_hWnd;
		int m_nFlags;
	};

public:
	void MoveControls(CSize csNewSize);

	// Re-baseline without moving anything. MoveControls is delta-based against
	// m_DlgSize, so a pass that repositions the controls out of band (the
	// proportional DPI rescale in CFluentDialog::RescaleChildren) has to move
	// the baseline too -- otherwise the next ordinary WM_SIZE recomputes the
	// delta from the pre-DPI size and re-applies the whole dpi change a second
	// time, displacing or double-growing anchored and stretched controls.
	void SetDlgSize(CSize csNewSize) { m_DlgSize = csNewSize; }

	void AddControl(int nControlID, int nFlags);
	void AddControl(HWND hWnd, int nFlags);

	void SetParent(HWND hWndParent);

protected:

	CArray< CDR_Data, CDR_Data > m_Controls;
	CSize m_DlgSize;
	HWND m_hWndParent;

protected:
};

#endif // !defined(AFX_DIALOGRESIZER_H__DA9AF3FF_C6CC_4D70_965A_4216A0EC9E75__INCLUDED_)
