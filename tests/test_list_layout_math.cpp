// Unit tests for src/ListLayoutMath.h -- pure pill/layout maths, no MFC.
//
// Expected values are HAND-COMPUTED from the source expressions
// (QListCtrl.cpp:561-563, QPasteWnd.cpp:690-830, CDPI::Scale in DPI.h:22).
// Scale(x, dpi) mirrors MulDiv(x, dpi, 96), i.e. (x*dpi + 48)/96 with
// integer division for the non-negative layout values used here. Each
// expectation below shows the arithmetic; none was copied from program
// output.
//
// Build & run (local, no MFC needed):
//   g++ -std=c++17 -Wall -Wextra -I src tests/test_list_layout_math.cpp -o dist/test_list_layout_math.exe
//   dist/test_list_layout_math.exe

#include "../src/ListLayoutMath.h"

#include <cstdio>

using namespace ListLayoutMath;

static int g_failures = 0;

#define CHECK_EQ(actual, expected) do { \
	int a_ = (actual); int e_ = (expected); \
	if (a_ != e_) { \
		g_failures++; \
		std::printf("FAIL %s:%ld  %s = %d, expected %d\n", __FILE__, (long)__LINE__, #actual, a_, e_); \
	} \
} while (0)

#define CHECK_RECT(actual, l, t, r, b) do { \
	IntRect rc_ = (actual); \
	if (!(rc_ == IntRect{ (l), (t), (r), (b) })) { \
		g_failures++; \
		std::printf("FAIL %s:%ld  %s = {%d,%d,%d,%d}, expected {%d,%d,%d,%d}\n", \
			__FILE__, (long)__LINE__, #actual, \
			rc_.left, rc_.top, rc_.right, rc_.bottom, (l), (t), (r), (b)); \
	} \
} while (0)

// DPI probe points: 100% = 96, 150% = 144, 200% = 192.
static void test_scale_value()
{
	// 1: Scale(2)@96 = (2*96+48)/96 = 240/96 = 2 (identity at 100%).
	CHECK_EQ(ScaleValue(2, 96), 2);
	// 2: Scale(2)@144 = (2*144+48)/96 = 336/96 = 3 (pill x inset at 150%).
	CHECK_EQ(ScaleValue(2, 144), 3);
	// 3: Scale(4)@144 = (4*144+48)/96 = 624/96 = 6 (pill radius at 150%).
	CHECK_EQ(ScaleValue(4, 144), 6);
	// 4: Scale(210)@144 = (210*144+48)/96 = 30288/96 = 315 (sidebar at 150%).
	CHECK_EQ(ScaleValue(210, 144), 315);
	// 5: Scale(1)@144 = (144+48)/96 = 192/96 = 2: MulDiv rounds half up,
	//    so two separately scaled 1px values sum to 4 while Scale(2) is 3.
	//    This is why batching must precompute each scaled constant (T8).
	CHECK_EQ(ScaleValue(1, 144), 2);
	CHECK_EQ(ScaleValue(1, 144) + ScaleValue(1, 144), 4);
	CHECK_EQ(ScaleValue(2, 144), 3);
	// 6: Scale(33-5)=Scale(28)@144 = (28*144+48)/96 = 4080/96 = 42:
	//    the subtraction happens BEFORE scaling (QPasteWnd.cpp:826).
	CHECK_EQ(SearchRowYOffset(144), 42);
	// 200% is exact doubling: Scale(210)@192 = (210*192+48)/96 = 40368/96 = 420.
	CHECK_EQ(ScaleValue(210, 192), 420);
	CHECK_EQ(ScaleValue(28, 192), 56);
}

static void test_pill_rect()
{
	IntRect item{ 0, 0, 400, 22 };
	// 100%: inset Scale(2)@96 = 2 -> {0+2, 0, 400-2, 22}; radius Scale(4)@96 = 4.
	CHECK_RECT(SelectedPillRect(item, 96), 2, 0, 398, 22);
	CHECK_EQ(PillCornerRadius(96), 4);
	// 150%: inset (2*144+48)/96 = 336/96 = 3 -> {3, 0, 397, 22}; radius 6.
	CHECK_RECT(SelectedPillRect(item, 144), 3, 0, 397, 22);
	CHECK_EQ(PillCornerRadius(144), 6);
	// 200%: inset (2*192+48)/96 = 432/96 = 4 -> {4, 0, 396, 22}; radius 8.
	CHECK_RECT(SelectedPillRect(item, 192), 4, 0, 396, 22);
	CHECK_EQ(PillCornerRadius(192), 8);
	// Top/bottom are untouched by DeflateRect(x, 0, x, 0) at any DPI.
	IntRect tall{ 10, 5, 300, 49 };
	CHECK_RECT(SelectedPillRect(tall, 144), 13, 5, 297, 49);
}

static void test_hover_rect()
{
	// Hover only swaps the colour (QListCtrl.cpp:547-548); the fill covers
	// the full row rect, so the rect is identity at every DPI.
	IntRect item{ 0, 44, 400, 66 };
	CHECK_RECT(HoverRect(item), 0, 44, 400, 66);
}

static void test_content_layout()
{
	// 100%, cx=800, no sidebar, no group: contentX=0, contentWidth=800,
	// chips=Scale(34)@96=34, listTop=0+34=34, bottom=Scale(33)@96=33.
	ContentLayout plain = ComputeContentLayout(800, false, false, 96);
	CHECK_EQ(plain.contentX, 0);
	CHECK_EQ(plain.contentWidth, 800);
	CHECK_EQ(plain.chipsHeight, 34);
	CHECK_EQ(plain.listTop, 34);
	CHECK_EQ(plain.bottomOffset, 33);
	// 100% with sidebar: contentX=Scale(210)@96=210, width=800-210=590,
	// chips hidden (0), listTop=0.
	ContentLayout side = ComputeContentLayout(800, true, false, 96);
	CHECK_EQ(side.contentX, 210);
	CHECK_EQ(side.contentWidth, 590);
	CHECK_EQ(side.chipsHeight, 0);
	CHECK_EQ(side.listTop, 0);
	// 150% with sidebar, cx=800: contentX=(210*144+48)/96=315,
	// width=800-315=485.
	ContentLayout side150 = ComputeContentLayout(800, true, false, 144);
	CHECK_EQ(side150.contentX, 315);
	CHECK_EQ(side150.contentWidth, 485);
	CHECK_EQ(side150.chipsHeight, 0);
	CHECK_EQ(side150.listTop, 0);
	// bottom offset at 150%: (33*144+48)/96 = 4800/96 = 50.
	CHECK_EQ(side150.bottomOffset, 50);
	// 200% with sidebar: contentX=420, width=380, bottom=(33*192+48)/96=66.
	ContentLayout side200 = ComputeContentLayout(800, true, false, 192);
	CHECK_EQ(side200.contentX, 420);
	CHECK_EQ(side200.contentWidth, 380);
	CHECK_EQ(side200.bottomOffset, 66);
	// Group-title mode adds Scale(20) on top: 20@96=20, chips still apply
	// without the sidebar -> listTop=20+34=54.
	ContentLayout group = ComputeContentLayout(800, false, true, 96);
	CHECK_EQ(group.listTop, 54);
	// Group + sidebar at 150%: topOfListBox=(20*144+48)/96=2928/96=30,
	// chips=0 -> listTop=30.
	ContentLayout group150 = ComputeContentLayout(800, true, true, 144);
	CHECK_EQ(group150.listTop, 30);
}

static void test_search_row()
{
	// 100%, cx=800, cy=600: searchX=34, searchW=800-70=730,
	// searchY=600-Scale(28)=600-28=572, h=25 -> {34,572,764,597}.
	SearchRowRects r96 = ComputeSearchRow(800, 600, 96);
	CHECK_RECT(r96.search, 34, 572, 764, 597);
	// sysMenu: x=800-30=770, y=600-28=572, 24x24 -> {770,572,794,596}.
	CHECK_RECT(r96.sysMenu, 770, 572, 794, 596);
	// groupsBtn: x=4, same y/size -> {4,572,28,596}.
	CHECK_RECT(r96.groupsBtn, 4, 572, 28, 596);
	// 150%: searchX=(34*144+48)/96=4944/96=51; searchW=800-105=695
	// (Scale(70)@144=(70*144+48)/96=10128/96=105); searchY=600-42=558;
	// h=(25*144+48)/96=3648/96=38 -> {51,558,746,596}.
	SearchRowRects r144 = ComputeSearchRow(800, 600, 144);
	CHECK_RECT(r144.search, 51, 558, 746, 596);
	// sysMenu x=800-45=755 (Scale(30)@144=45), 36x36 -> {755,558,791,594}.
	CHECK_RECT(r144.sysMenu, 755, 558, 791, 594);
	// groupsBtn x=(4*144+48)/96=624/96=6 -> {6,558,42,594}.
	CHECK_RECT(r144.groupsBtn, 6, 558, 42, 594);
	// 200%: searchX=68, searchW=800-140=660, y=600-56=544, h=50.
	SearchRowRects r192 = ComputeSearchRow(800, 600, 192);
	CHECK_RECT(r192.search, 68, 544, 728, 594);
	CHECK_RECT(r192.sysMenu, 740, 544, 788, 592);
	CHECK_RECT(r192.groupsBtn, 8, 544, 56, 592);
}

static void test_group_header_and_list_rect()
{
	// Group header, 100%, cx=800, sidebar: groupLeft=210;
	// back={(210+2),2,(210+2+16),(2+16)}={212,2,228,18};
	// title x=210+24=234, w=800-210-20=570 -> {234,2,804,18}.
	GroupHeaderRects g = ComputeGroupHeader(800, true, 96);
	CHECK_EQ(g.groupLeft, 210);
	CHECK_RECT(g.backBtn, 212, 2, 228, 18);
	CHECK_RECT(g.title, 234, 2, 804, 18);
	// No sidebar: groupLeft=0 -> back={2,2,18,18};
	// title x=0+24=24, w=800-0-20=780 -> right=24+780=804.
	GroupHeaderRects g0 = ComputeGroupHeader(800, false, 96);
	CHECK_EQ(g0.groupLeft, 0);
	CHECK_RECT(g0.backBtn, 2, 2, 18, 18);
	CHECK_RECT(g0.title, 24, 2, 804, 18);
	// List rect, 100%, no sidebar: contentX=0, listTop=34, width=800,
	// cy=600, bottom=33, extra=0:
	// h=600-33-34+0+1=534 -> {0,34,800,568}.
	IntRect list = ComputeListRect(0, 34, 800, 600, 33, 0);
	CHECK_RECT(list, 0, 34, 800, 568);
	// No-results rect, same geometry: border=Scale(10)@96=10 ->
	// x=0+10=10, y=34+10=44, w=800-10=790 -> right=800,
	// h=600-33-34+1-10=524 -> bottom=44+524=568 -> {10,44,800,568}.
	IntRect nr = ComputeNoResultsRect(0, 800, 34, 600, 33, 96);
	CHECK_RECT(nr, 10, 44, 800, 568);
}

int main()
{
	test_scale_value();
	test_pill_rect();
	test_hover_rect();
	test_content_layout();
	test_search_row();
	test_group_header_and_list_rect();

	if (g_failures > 0)
	{
		std::printf("%d failure(s)\n", g_failures);
		return 1;
	}
	std::printf("all list layout tests passed\n");
	return 0;
}
