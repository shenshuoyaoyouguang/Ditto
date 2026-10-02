#pragma once

#include "DPI.h"

#define NM_SIDEBAR_VIEW_CLICKED		(WM_USER + 0x141)	// wParam = view index

enum SidebarView
{
	VIEW_ALL = 0,	// value order must match ChipFilter
	VIEW_TEXT,
	VIEW_IMAGE,
	VIEW_FILE,
	VIEW_LINK,
	VIEW_STARRED,
	VIEW_COUNT
};

// CSidebar: docked left sidebar (plan section 5.4).
// Filter view rows on top; the group tree (owned by QPasteWnd) is parented
// into the area below the "groups" caption via GetGroupsArea.
// Sends NM_SIDEBAR_VIEW_CLICKED to the parent with the view index.
class CSidebar : public CWnd
{
public:
	CSidebar();

	BOOL Create(CWnd* pParent, UINT nID);
	void SetDpiInfo(CDPI* dpi) { m_dpi = dpi; }
	void SetLabels(const CString csLabels[VIEW_COUNT]);
	void SetGroupsCaption(const CString& cs) { m_csGroupsCaption = cs; Invalidate(FALSE); }
	void SetActiveView(int index) { m_active = index; Invalidate(FALSE); }

	int ViewHitTest(CPoint point);
	void GetGroupsArea(CRect& rc);

protected:
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();
	afx_msg void OnSize(UINT nType, int cx, int cy);

	void LayoutViews();

	CDPI* m_dpi;
	CString m_labels[VIEW_COUNT];
	CRect m_views[VIEW_COUNT];
	bool m_bViewsLaidOut;
	int m_active;
	int m_hover;
	bool m_bHoverTracked;
	CString m_csGroupsCaption;

	DECLARE_MESSAGE_MAP()
};
