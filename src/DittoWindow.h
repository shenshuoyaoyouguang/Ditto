#pragma once

#include "GdipButton.h"
#include "GdiImageDrawer.h"
#include "DPI.h"

#define BUTTON_CLOSE 1
#define BUTTON_CHEVRON 2
#define BUTTON_MINIMIZE 3
#define BUTTON_MAXIMIZE 4

class CDittoWindow
{
public:
	CDittoWindow(void);
	~CDittoWindow(void);

	void DoNcPaint(CWnd *pWnd);
	void DrawChevronBtn(CWindowDC &dc, CWnd *pWnd);
	void DrawCloseBtn(CWindowDC &dc, CWnd *pWnd);
	void DrawMaximizeBtn(CWindowDC &dc, CWnd *pWnd);
	void DrawMinimizeBtn(CWindowDC &dc, CWnd *pWnd);


	void DoCreate(CWnd *pWnd);
	void DoNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS FAR* lpncsp);
	UINT DoNcHitTest(CWnd *pWnd, CPoint point);
	long DoNcLButtonUp(CWnd *pWnd, UINT nHitTest, CPoint point);
	int DoNcLButtonDown(CWnd *pWnd, UINT nHitTest, CPoint point);
	void DoNcMouseMove(CWnd *pWnd, UINT nHitTest, CPoint point) ;
	// Clears the caption button hover state. The hosts must forward
	// WM_NCMOUSELEAVE here: without it the highlight set by the last
	// WM_NCMOUSEMOVE stays stuck when the cursor moves off the buttons into the
	// client area or leaves the window (no further NCMOUSEMOVE is sent).
	void DoNcMouseLeave(CWnd *pWnd);
	bool DoPreTranslateMessage(MSG* pMsg);
	void SetCaptionOn(CWnd *pWnd, int nPos, bool bOnstartup, int captionSize, int captionFontSize);
	bool SetCaptionColors(COLORREF left, COLORREF right, COLORREF border);
	void SetCaptionTextColor(COLORREF color);
	void MinMaxWindow(CWnd *pWnd, long lOption);
	void SetTitleTextHeight(CWnd *pWnd);
	int IndexToPos(int index, bool horizontal);
	void OnDpiChanged(CWnd *pWnd, int dpi);
	
	bool m_bDrawClose;
	bool m_sendWMClose;
	bool m_bDrawChevron;
	bool m_bDrawMaximize;
	bool m_bDrawMinimize;

	CRect m_crCloseBT;
	CRect m_crChevronBT;
	CRect m_crMaximizeBT;
	CRect m_crMinimizeBT;
	CRect m_crWindowIconBT;

	CFont m_VertFont;
	CFont m_HorFont;

	bool m_bMinimized;

	bool m_bMouseDownOnChevron;
	bool m_bMouseOverChevron;
	bool m_bMouseDownOnClose;
	bool m_bMouseOverClose;
	bool m_bMouseDownOnMinimize;
	bool m_bMouseOverMinimize;
	bool m_bMouseDownOnMaximize;
	bool m_bMouseOverMaximize;

	COLORREF m_CaptionColorLeft;
	COLORREF m_CaptionColorRight;
	COLORREF m_CaptionTextColor;
	COLORREF m_border;
	
	CGdiImageDrawer m_closeButton;
	CGdiImageDrawer m_chevronRightButton;
	CGdiImageDrawer m_chevronLeftButton;
	CGdiImageDrawer m_maximizeButton;
	CGdiImageDrawer m_minimizeButton;
	//CGdiImageDrawer m_windowIcon;

	CString m_customWindowTitle;
	bool m_useCustomWindowTitle;

	int m_captionBorderWidth;
	int m_captionFontSize;

	int m_captionPosition;
	int m_borderSize;

	int m_titleTextHeight;

	bool m_buttonDownOnCaption;

	CRect m_crFullSizeWindow;
	COleDateTime m_TimeMinimized;
	COleDateTime m_TimeMaximized;

protected:
	void DrawCaptionButtonBackground(CWindowDC &dc, const CRect &rcButton, bool bMouseOver, bool bMouseDown, bool bCloseButton);

	// True when the last DWM call reported the attribute was accepted, i.e. this
	// OS build knows DWMWA_USE_IMMERSIVE_DARK_MODE. It is NOT whether dark is
	// currently applied -- the call succeeds for both values.
	bool m_bDwmDarkSupported;
	// The mode we last asked DWM for. Comparing this against the theme is what
	// makes a runtime theme switch actually re-issue the call.
	bool m_bDwmDarkApplied;
	bool m_bNcMouseTracking;

public:
	CDPI m_dpi;
};
