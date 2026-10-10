#pragma once

// Pure list/main-window layout maths extracted from the paint/layout code so
// the geometry can be unit-tested without MFC. No MFC/CString/windows.h
// dependencies: plain ints/structs only, following the src/ThemeColorMath.h
// seam pattern. EXTRACTION ONLY -- call sites in QListCtrl.cpp/QPasteWnd.cpp
// are unchanged; the comments cite the source lines this mirrors.
//
// Sources:
//   - Selected pill: QListCtrl.cpp:561-563
//       rcPill.DeflateRect(m_windowDpi->Scale(2), 0, m_windowDpi->Scale(2), 0);
//       FillRoundedRect(pDC, rcPill, m_windowDpi->Scale(4), crBkgnd);
//   - Hover: QListCtrl.cpp:547-548 + 567. Hover only swaps the background
//     colour (StateHover); the fill still covers the full row rect rcItem
//     (LVIR_SELECTBOUNDS from line 495), so the hover rect is identity.
//   - Main-window layout: QPasteWnd.cpp:690-830 (MoveControls).
//   - DPI rounding: CDPI::Scale in src/DPI.h:22 is MulDiv(x, dpi, 96).
//     MulDiv rounds half-up for the non-negative layout values used here:
//       Scale(x, dpi) = (x * dpi + 48) / 96   (integer division)
//
// Like the call sites, every logical value is scaled independently
// ("Scale applied per value"): Scale(a) + Scale(b) can differ from
// Scale(a + b) at fractional DPIs, so batched callers must precompute each
// scaled constant, not scale a summed logical value. T8 consumes this.

namespace ListLayoutMath
{

struct IntRect
{
	int left;
	int top;
	int right;
	int bottom;
};

inline bool operator==(IntRect a, IntRect b)
{
	return a.left == b.left && a.top == b.top &&
		a.right == b.right && a.bottom == b.bottom;
}

// Logical (unscaled) constants straight from the call sites.
enum : int
{
	kSearchRowStart = 33,      // QPasteWnd.cpp:722
	kSearchRowBottomGap = 5,   // search y uses searchRowStart - 5 (:826)
	kSidebarWidth = 210,       // :709, :739
	kChipsHeight = 34,         // :754
	kSearchInsetLeft = 34,     // :826
	kSearchInsetWidth = 70,    // :826
	kSearchHeight = 25,        // :826
	kCornerInsetX = 30,        // :828 (system menu x inset)
	kCornerInsetY = 28,        // :828/:830 (corner buttons y inset)
	kCornerSize = 24,          // :828/:830 (corner buttons w/h)
	kGroupsBtnInsetLeft = 4,   // :830
	kGroupBarHeight = 20,      // :714 topOfListBox in group mode
	kGroupBackSize = 16,       // :711/:712 back button w/h
	kGroupBackInset = 2,       // :711 back button x/y
	kGroupTitleInset = 24,     // :712 group title x inset
	kGroupTitleRightTrim = 20, // :712 group title width trim
	kPillInsetX = 2,           // QListCtrl.cpp:562 DeflateRect x
	kPillRadius = 4,           // QListCtrl.cpp:563 FillRoundedRect radius
	kNoResultsBorder = 10      // QPasteWnd.cpp:792
};

// Mirrors CDPI::Scale (DPI.h:22): MulDiv(x, dpi, 96), half-up rounding for
// the non-negative values used in layout. Negative inputs round half away
// from zero, matching MulDiv's documented rounding.
inline int ScaleValue(int logical, int dpi)
{
	long long product = (long long)logical * dpi;
	if (product >= 0)
		return (int)((product + 48) / 96);
	return (int)((product - 48) / 96);
}

// cy - Scale(searchRowStart - 5): the search row sits one bottom-gap above
// the window bottom (QPasteWnd.cpp:826). Note the subtraction happens
// BEFORE scaling -- Scale(33 - 5) = Scale(28).
inline int SearchRowYOffset(int dpi)
{
	return ScaleValue(kSearchRowStart - kSearchRowBottomGap, dpi);
}

// Selected pill: inset 2 (scaled) on each side, full row height, corner
// radius Scale(4) (QListCtrl.cpp:561-563).
inline IntRect SelectedPillRect(IntRect item, int dpi)
{
	int inset = ScaleValue(kPillInsetX, dpi);
	return IntRect{ item.left + inset, item.top, item.right - inset, item.bottom };
}

inline int PillCornerRadius(int dpi)
{
	return ScaleValue(kPillRadius, dpi);
}

// Hover paints the full row rect (QListCtrl.cpp:547-548 colour swap only,
// filled at :567), so the rect is the item rect unchanged.
inline IntRect HoverRect(IntRect item)
{
	return item;
}

struct ContentLayout
{
	int contentX;     // 0, or Scale(210) with the sidebar (:739)
	int contentWidth; // cx - contentX (:740)
	int chipsHeight;  // Scale(34) without sidebar, 0 with it (:754)
	int listTop;      // topOfListBox + chipsHeight (:758)
	int bottomOffset; // Scale(searchRowStart) (:730)
};

// topOfListBox is Scale(20) in group-title mode (:714), else 0 (:698).
inline ContentLayout ComputeContentLayout(int cx, bool showSidebar, bool inGroupMode, int dpi)
{
	ContentLayout out;
	out.contentX = showSidebar ? ScaleValue(kSidebarWidth, dpi) : 0;
	out.contentWidth = cx - out.contentX;
	out.chipsHeight = showSidebar ? 0 : ScaleValue(kChipsHeight, dpi);
	int topOfListBox = inGroupMode ? ScaleValue(kGroupBarHeight, dpi) : 0;
	out.listTop = topOfListBox + out.chipsHeight;
	out.bottomOffset = ScaleValue(kSearchRowStart, dpi);
	return out;
}

struct SearchRowRects
{
	IntRect search;   // :826
	IntRect sysMenu;  // :828
	IntRect groupsBtn; // :830
};

inline SearchRowRects ComputeSearchRow(int cx, int cy, int dpi)
{
	SearchRowRects out;
	int searchX = ScaleValue(kSearchInsetLeft, dpi);
	int searchW = cx - ScaleValue(kSearchInsetWidth, dpi);
	int searchY = cy - SearchRowYOffset(dpi);
	int searchH = ScaleValue(kSearchHeight, dpi);
	out.search = IntRect{ searchX, searchY, searchX + searchW, searchY + searchH };

	int corner = ScaleValue(kCornerSize, dpi);
	int cornerY = cy - ScaleValue(kCornerInsetY, dpi);
	int menuX = cx - ScaleValue(kCornerInsetX, dpi);
	out.sysMenu = IntRect{ menuX, cornerY, menuX + corner, cornerY + corner };

	int groupsX = ScaleValue(kGroupsBtnInsetLeft, dpi);
	out.groupsBtn = IntRect{ groupsX, cornerY, groupsX + corner, cornerY + corner };
	return out;
}

struct GroupHeaderRects
{
	int groupLeft;   // sidebar content origin offset (:709)
	IntRect backBtn; // :711
	IntRect title;   // :712
};

inline GroupHeaderRects ComputeGroupHeader(int cx, bool showSidebar, int dpi)
{
	GroupHeaderRects out;
	out.groupLeft = showSidebar ? ScaleValue(kSidebarWidth, dpi) : 0;
	int back = ScaleValue(kGroupBackSize, dpi);
	int backInset = ScaleValue(kGroupBackInset, dpi);
	out.backBtn = IntRect{
		out.groupLeft + backInset, backInset,
		out.groupLeft + backInset + back, backInset + back };
	int titleX = out.groupLeft + ScaleValue(kGroupTitleInset, dpi);
	int titleW = cx - out.groupLeft - ScaleValue(kGroupTitleRightTrim, dpi);
	out.title = IntRect{ titleX, backInset, titleX + titleW, backInset + back };
	return out;
}

// List area: MoveWindow(contentX, listTop, contentWidth + extra,
// cy - bottomOffset - listTop + extra + 1) (:800). extraSize is the hidden
// native-scrollbar compensation (:768), 0 when the native bar shows.
inline IntRect ComputeListRect(int contentX, int listTop, int contentWidth,
	int cy, int bottomOffset, int extraSize)
{
	return IntRect{
		contentX, listTop,
		contentX + contentWidth + extraSize,
		listTop + (cy - bottomOffset - listTop + extraSize + 1) };
}

// No-results placeholder (:793): MoveWindow(contentX + border,
// listTop + border, contentWidth - border, cy - bottom - listTop + 1 - border).
inline IntRect ComputeNoResultsRect(int contentX, int contentWidth, int listTop,
	int cy, int bottomOffset, int dpi)
{
	int border = ScaleValue(kNoResultsBorder, dpi);
	return IntRect{
		contentX + border, listTop + border,
		contentX + border + (contentWidth - border),
		listTop + border + (cy - bottomOffset - listTop + 1 - border) };
}

} // namespace ListLayoutMath
