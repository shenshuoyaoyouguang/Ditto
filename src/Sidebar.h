#pragma once

#include "DPI.h"
#include "ChipBar.h"

#define NM_SIDEBAR_VIEW_CLICKED		(WM_USER + 0x141)	// wParam = view index

enum SidebarView
{
	VIEW_ALL = 0,
	VIEW_TEXT,
	VIEW_IMAGE,
	VIEW_FILE,
	VIEW_LINK,
	VIEW_STARRED,	// sidebar only; there is no starred chip
	VIEW_COUNT
};

// The sidebar rows mirror the chip filter row, and the host used to convert
// between them with arithmetic on the raw enums. Assert the correspondence so
// inserting a chip without inserting the matching sidebar row breaks the build
// instead of silently highlighting the wrong row.
static_assert((int)VIEW_ALL == (int)CHIP_ALL, "SidebarView must track ChipFilter");
static_assert((int)VIEW_TEXT == (int)CHIP_TEXT, "SidebarView must track ChipFilter");
static_assert((int)VIEW_IMAGE == (int)CHIP_IMAGE, "SidebarView must track ChipFilter");
static_assert((int)VIEW_FILE == (int)CHIP_FILE, "SidebarView must track ChipFilter");
static_assert((int)VIEW_LINK == (int)CHIP_LINK, "SidebarView must track ChipFilter");
static_assert(VIEW_COUNT == CHIP_COUNT + 1, "the starred row is sidebar only");

// Single place where a chip filter becomes a sidebar row (-1 = no row active).
inline int ChipFilterToView(int chipFilter)
{
	return (chipFilter >= (int)CHIP_ALL && chipFilter <= (int)CHIP_LINK) ? chipFilter : -1;
}

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

	// See CChipBar::InvalidateLayout -- the row cache is not invalidated by a
	// MoveWindow that turns out to be a no-op.
	void InvalidateLayout() { m_bViewsLaidOut = false; Invalidate(FALSE); }
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
