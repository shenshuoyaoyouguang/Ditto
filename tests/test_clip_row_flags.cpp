// Unit tests for src/ClipRowFlags.h -- per-row flag/symbol parsing extracted
// from CQListCtrl::OnCustomdrawList, no MFC.
//
// Expected values are hand-derived from the source logic (QListCtrl.cpp:505-512
// first-'|' split, :576-647 flag checks, Misc.cpp:1209-1308 case-insensitive
// insert loop), never from the implementation. Offsets below were computed by
// hand; see the comments for the character-by-character lays.
//
// Build & run (local, no MFC needed):
//   g++ -std=c++17 -Wall -Wextra -I src tests/test_clip_row_flags.cpp -o test_clip_row_flags.exe
//   .\test_clip_row_flags.exe
// Delete the exe afterwards to keep the repo root clean.

#include "../src/ClipRowFlags.h"

#include <cstdio>
#include <string>

static int g_failures = 0;

#define CHECK_TRUE(cond) do { \
	if (!(cond)) { \
		g_failures++; \
		std::printf("FAIL %s:%ld  %s\n", __FILE__, (long)__LINE__, #cond); \
	} \
} while (0)

static void test_split_no_pipe()
{
	// No '|' -> symbols empty, text is the whole row.
	auto r = SplitClipRowText(std::wstring(L"hello"));
	CHECK_TRUE(r.symbols == L"");
	CHECK_TRUE(r.text == L"hello");
}

static void test_split_empty()
{
	auto r = SplitClipRowText(std::wstring(L""));
	CHECK_TRUE(r.symbols == L"");
	CHECK_TRUE(r.text == L"");
}

static void test_split_basic()
{
	// "<group>|hello": pipe at 7, symbols = first 7 chars, text = rest.
	auto r = SplitClipRowText(std::wstring(L"<group>|hello"));
	CHECK_TRUE(r.symbols == L"<group>");
	CHECK_TRUE(r.text == L"hello");
}

static void test_split_leading_pipe()
{
	// "|text": pipe at 0, symbols empty, text is everything after it.
	auto r = SplitClipRowText(std::wstring(L"|text"));
	CHECK_TRUE(r.symbols == L"");
	CHECK_TRUE(r.text == L"text");
}

static void test_split_multi_pipe()
{
	// Split is at the FIRST '|' only (CString::Find): "a|b|c" -> "a" + "b|c".
	auto r = SplitClipRowText(std::wstring(L"a|b|c"));
	CHECK_TRUE(r.symbols == L"a");
	CHECK_TRUE(r.text == L"b|c");
}

static void test_split_trailing_pipe()
{
	// "sym|" -> symbols "sym", empty text.
	auto r = SplitClipRowText(std::wstring(L"sym|"));
	CHECK_TRUE(r.symbols == L"sym");
	CHECK_TRUE(r.text == L"");
}

static void test_split_narrow()
{
	// std::string instantiation behaves the same.
	auto r = SplitClipRowText(std::string("a|b|c"));
	CHECK_TRUE(r.symbols == "a");
	CHECK_TRUE(r.text == "b|c");
}

static void test_flags_empty()
{
	ClipRowFlags f = ParseClipRowFlags(std::wstring(L""), 0);
	CHECK_TRUE(!f.wasPasted);
	CHECK_TRUE(!f.isGroup);
	CHECK_TRUE(!f.neverAutoDelete);
	CHECK_TRUE(!f.hasShortcut);
	CHECK_TRUE(!f.inGroup);
	CHECK_TRUE(f.drawInGroupIcon);
	CHECK_TRUE(!f.inGroupIconVisible);
	CHECK_TRUE(!f.hasQuickPasteText);
	CHECK_TRUE(!f.isSticky);
}

static void test_flags_each_alone()
{
	CHECK_TRUE(ParseClipRowFlags(std::wstring(L"<pasted>"), 0).wasPasted);
	CHECK_TRUE(ParseClipRowFlags(std::wstring(L"<group>"), 0).isGroup);
	CHECK_TRUE(ParseClipRowFlags(std::wstring(L"<noautodelete>"), 0).neverAutoDelete);
	CHECK_TRUE(ParseClipRowFlags(std::wstring(L"<shortcut>"), 0).hasShortcut);
	CHECK_TRUE(ParseClipRowFlags(std::wstring(L"<ingroup>"), 0).inGroup);
	CHECK_TRUE(ParseClipRowFlags(std::wstring(L"<qpastetext>"), 0).hasQuickPasteText);
	CHECK_TRUE(ParseClipRowFlags(std::wstring(L"<sticky>"), 0).isSticky);
	// A lone <group> must not light up the other flags.
	ClipRowFlags g = ParseClipRowFlags(std::wstring(L"<group>"), 0);
	CHECK_TRUE(!g.isSticky && !g.wasPasted && !g.inGroup);
}

static void test_flags_combo()
{
	// All flags at once; substring search finds each token anywhere.
	ClipRowFlags f = ParseClipRowFlags(
		std::wstring(L"<group><noautodelete><shortcut><ingroup><qpastetext><sticky><pasted>"), 0);
	CHECK_TRUE(f.wasPasted);
	CHECK_TRUE(f.isGroup);
	CHECK_TRUE(f.neverAutoDelete);
	CHECK_TRUE(f.hasShortcut);
	CHECK_TRUE(f.inGroup);
	CHECK_TRUE(f.drawInGroupIcon);
	CHECK_TRUE(f.inGroupIconVisible);
	CHECK_TRUE(f.hasQuickPasteText);
	CHECK_TRUE(f.isSticky);
}

static void test_flags_case_sensitive()
{
	// CString::Find is case-sensitive: upper-case tokens must NOT match.
	ClipRowFlags f = ParseClipRowFlags(std::wstring(L"<GROUP><Sticky><PASTED>"), 0);
	CHECK_TRUE(!f.isGroup);
	CHECK_TRUE(!f.isSticky);
	CHECK_TRUE(!f.wasPasted);
}

static void test_flags_ingroup_visibility()
{
	// Outside any group (groupId 0) the in-group icon shows.
	ClipRowFlags a = ParseClipRowFlags(std::wstring(L"<ingroup>"), 0);
	CHECK_TRUE(a.inGroup);
	CHECK_TRUE(a.drawInGroupIcon);
	CHECK_TRUE(a.inGroupIconVisible);

	// Inside a group (theApp.m_GroupID > 0) the icon is suppressed (:603-608),
	// but the raw flag is still detected.
	ClipRowFlags b = ParseClipRowFlags(std::wstring(L"<ingroup>"), 5);
	CHECK_TRUE(b.inGroup);
	CHECK_TRUE(!b.drawInGroupIcon);
	CHECK_TRUE(!b.inGroupIconVisible);

	// Inside a group without the flag: nothing to suppress, nothing to draw.
	ClipRowFlags c = ParseClipRowFlags(std::wstring(L"<sticky>"), 5);
	CHECK_TRUE(!c.inGroup);
	CHECK_TRUE(c.drawInGroupIcon);
	CHECK_TRUE(!c.inGroupIconVisible);
}

static void test_spans_case_insensitive()
{
	// Hand lay: H(0)e(1)l(2)l(3)o(4) (5)H(6)E(7)L(8)L(9)O(10) (11)h(12)...:
	// "hello" lowered matches at 0, 6 and 12, each length 5.
	auto spans = ComputeHighlightSpans(std::wstring(L"Hello HELLO hello"), std::wstring(L"hello"));
	CHECK_TRUE(spans.size() == 3);
	if (spans.size() == 3)
	{
		CHECK_TRUE(spans[0].start == 0 && spans[0].length == 5);
		CHECK_TRUE(spans[1].start == 6 && spans[1].length == 5);
		CHECK_TRUE(spans[2].start == 12 && spans[2].length == 5);
	}
}

static void test_spans_no_match()
{
	auto spans = ComputeHighlightSpans(std::wstring(L"hello"), std::wstring(L"xyz"));
	CHECK_TRUE(spans.empty());
}

static void test_spans_empty_search()
{
	// Mirrors the findStr != "" guard: empty search never matches.
	auto spans = ComputeHighlightSpans(std::wstring(L"hello"), std::wstring(L""));
	CHECK_TRUE(spans.empty());
}

static void test_spans_non_overlapping()
{
	// "aaa" vs "aa": hit at 0, then startFindPos = 0 + 2 = 2, and
	// "aa" is not found at 2 -> exactly one span {0, 2}.
	auto spans = ComputeHighlightSpans(std::wstring(L"aaa"), std::wstring(L"aa"));
	CHECK_TRUE(spans.size() == 1);
	if (spans.size() == 1)
	{
		CHECK_TRUE(spans[0].start == 0 && spans[0].length == 2);
	}
}

static void test_spans_adjacent()
{
	// "abab" vs "ab": hits at 0 and 2.
	auto spans = ComputeHighlightSpans(std::wstring(L"abab"), std::wstring(L"ab"));
	CHECK_TRUE(spans.size() == 2);
	if (spans.size() == 2)
	{
		CHECK_TRUE(spans[0].start == 0 && spans[0].length == 2);
		CHECK_TRUE(spans[1].start == 2 && spans[1].length == 2);
	}
}

static void test_insert_two_hits()
{
	// Hand trace with pre="<" post=">": hit at lowered 0 -> insert at
	// newPos 0 and 0+1+2=3 => "<ab> ab"; insertedLength=2. Next hit at
	// lowered 3 -> newPos 5, insert => "<ab> <ab>..." => "<ab> <ab>".
	std::wstring s(L"ab ab");
	int n = ClipRowFindNoCaseAndInsert(s, std::wstring(L"ab"),
		std::wstring(L"<"), std::wstring(L">"), 10);
	CHECK_TRUE(n == 2);
	CHECK_TRUE(s == L"<ab> <ab>");
}

static void test_insert_preserves_case()
{
	// Matching is case-insensitive but the inserted markers wrap the
	// ORIGINAL text: "aB" searched as "ab" -> "<aB>".
	std::wstring s(L"aB");
	int n = ClipRowFindNoCaseAndInsert(s, std::wstring(L"ab"),
		std::wstring(L"<"), std::wstring(L">"), 10);
	CHECK_TRUE(n == 1);
	CHECK_TRUE(s == L"<aB>");
}

static void test_insert_no_match()
{
	std::wstring s(L"hello");
	int n = ClipRowFindNoCaseAndInsert(s, std::wstring(L"xyz"),
		std::wstring(L"<"), std::wstring(L">"), 10);
	CHECK_TRUE(n == 0);
	CHECK_TRUE(s == L"hello");
}

static void test_insert_empty_search()
{
	std::wstring s(L"hello");
	int n = ClipRowFindNoCaseAndInsert(s, std::wstring(L""),
		std::wstring(L"<"), std::wstring(L">"), 10);
	CHECK_TRUE(n == 0);
	CHECK_TRUE(s == L"hello");
}

static void test_insert_rewrites_newline()
{
	// "a\nb" vs "a": hit at 0 -> "<a>\nb". firstFindPos = 0+1 = 1;
	// newline scan: \n at 3, 1 < 3 but line(0) > 10-1=9 is false -> break.
	// replaceCount > 0 so \n -> \x01\x05\x02: "<a>\x01\x05\x02b".
	std::wstring s(L"a\nb");
	int n = ClipRowFindNoCaseAndInsert(s, std::wstring(L"a"),
		std::wstring(L"<"), std::wstring(L">"), 10);
	CHECK_TRUE(n == 1);
	CHECK_TRUE(s == std::wstring(L"<a>\x01\x05\x02" L"b"));
}

static void test_insert_rewrites_crlf()
{
	// Hand trace "x\r\nab" vs "ab": hit at lowered 3 -> insert pre at 3,
	// post at 3+1+2=6 => "x\r\n<ab>". Newline scan (only '\n'): \n at 2,
	// firstFindPos = 3+1 = 4 < 2? No -> line=1, no more -> break.
	// Rewrite: "\r\n" first => "x\x01\x05\x02<ab>", then lone \r/\n: none.
	std::wstring s(L"x\r\nab");
	int n = ClipRowFindNoCaseAndInsert(s, std::wstring(L"ab"),
		std::wstring(L"<"), std::wstring(L">"), 10);
	CHECK_TRUE(n == 1);
	CHECK_TRUE(s == std::wstring(L"x\x01\x05\x02<ab>"));
}

static void test_insert_truncates_to_match_line()
{
	// Hand trace, pre="<" post=">", linesPerRow=1, text "aa\nbb\ncc ab\ndd":
	// lowered offsets a(0)a(1)\n(2)b(3)b(4)\n(5)c(6)c(7) (8)a(9)b(10)\n(11):
	// hit at 9, firstFindPos = 9+1 = 10. Insert => "aa\nbb\ncc <ab>\ndd".
	// Newline scan of the MODIFIED string: \n at 2 (10<2? no, line=1),
	// \n at 5 (10<5? no, line=2), \n at 13 (10<13? yes, line 2 > 1-1=0
	// => truncate, linesPerRow==1 so lineStart=prevLinePos=5):
	//   "... " + Mid(6) = "... cc <ab>\ndd".
	// Then \n rewrite => "... cc <ab>\x01\x05\x02dd".
	std::wstring s(L"aa\nbb\ncc ab\ndd");
	int n = ClipRowFindNoCaseAndInsert(s, std::wstring(L"ab"),
		std::wstring(L"<"), std::wstring(L">"), 1);
	CHECK_TRUE(n == 1);
	CHECK_TRUE(s == std::wstring(L"... cc <ab>\x01\x05\x02" L"dd"));
}

static void test_insert_no_truncate_with_tall_window()
{
	// Same input with linesPerRow=3: at the \n at 13, line(2) > 3-1=2 is
	// false -> break, no truncation; only the \n rewrite applies:
	// "aa\x01\x05\x02bb\x01\x05\x02cc <ab>\x01\x05\x02dd".
	std::wstring s(L"aa\nbb\ncc ab\ndd");
	int n = ClipRowFindNoCaseAndInsert(s, std::wstring(L"ab"),
		std::wstring(L"<"), std::wstring(L">"), 3);
	CHECK_TRUE(n == 1);
	CHECK_TRUE(s == std::wstring(L"aa\x01\x05\x02" L"bb\x01\x05\x02" L"cc <ab>\x01\x05\x02" L"dd"));
}

static void test_spans_cap_matches_production_101()
{
	// Parity with Misc.cpp:1233-1257: replaceCount is incremented THEN the
	// loop breaks on > 100, so production performs 101 insertions. Default
	// maxHits must therefore be 101, not 100.
	std::wstring text(200, L'a'); // 200 x 'a', search "a" -> 200 candidates
	auto spans = ComputeHighlightSpans(text, std::wstring(L"a"));
	CHECK_TRUE(spans.size() == 101);
	if (spans.size() == 101)
	{
		CHECK_TRUE(spans[0].start == 0);
		CHECK_TRUE(spans[100].start == 100);
	}
	// Explicit cap still honored when the caller passes one.
	auto capped = ComputeHighlightSpans(text, std::wstring(L"a"), 100);
	CHECK_TRUE(capped.size() == 100);
}

static void test_insert_cap_matches_production_101()
{
	// Same 101 bound through the insert path: 200 hits -> 101 insertions.
	std::wstring s(200, L'a');
	int n = ClipRowFindNoCaseAndInsert(s, std::wstring(L"a"),
		std::wstring(L"<"), std::wstring(L">"), 1000);
	CHECK_TRUE(n == 101);
}

static void test_lower_ascii_case_insensitive()
{
	// Defined seam behavior: ASCII case-insensitivity.
	auto spans = ComputeHighlightSpans(std::wstring(L"AbC"), std::wstring(L"aBc"));
	CHECK_TRUE(spans.size() == 1);
	if (spans.size() == 1)
	{
		CHECK_TRUE(spans[0].start == 0 && spans[0].length == 3);
	}
}

static void test_lower_non_ascii_passthrough()
{
	// Defined seam behavior: non-ASCII code units pass through the C-locale
	// mapping unchanged (no ICU). Latin-1 E-acute upper vs lower must NOT
	// match in the seam even though ICU production lowering could equate
	// such non-ASCII case pairs.
	CHECK_TRUE(ClipRowToLower(std::wstring(L"\x00C9")) == std::wstring(L"\x00C9"));
	auto spans = ComputeHighlightSpans(std::wstring(L"\x00C9"), std::wstring(L"\x00E9"));
	CHECK_TRUE(spans.empty());
	// Greek final sigma passes through unchanged as well.
	CHECK_TRUE(ClipRowToLower(std::wstring(L"\x03C2")) == std::wstring(L"\x03C2"));
}

static void test_insert_narrow()
{
	std::string s("ab ab");
	int n = ClipRowFindNoCaseAndInsert(s, std::string("ab"),
		std::string("<"), std::string(">"), 10);
	CHECK_TRUE(n == 2);
	CHECK_TRUE(s == "<ab> <ab>");
}

int main()
{
	test_split_no_pipe();
	test_split_empty();
	test_split_basic();
	test_split_leading_pipe();
	test_split_multi_pipe();
	test_split_trailing_pipe();
	test_split_narrow();
	test_flags_empty();
	test_flags_each_alone();
	test_flags_combo();
	test_flags_case_sensitive();
	test_flags_ingroup_visibility();
	test_spans_case_insensitive();
	test_spans_no_match();
	test_spans_empty_search();
	test_spans_non_overlapping();
	test_spans_adjacent();
	test_insert_two_hits();
	test_insert_preserves_case();
	test_insert_no_match();
	test_insert_empty_search();
	test_insert_rewrites_newline();
	test_insert_rewrites_crlf();
	test_insert_truncates_to_match_line();
	test_insert_no_truncate_with_tall_window();
	test_spans_cap_matches_production_101();
	test_insert_cap_matches_production_101();
	test_lower_ascii_case_insensitive();
	test_lower_non_ascii_passthrough();
	test_insert_narrow();

	if (g_failures > 0)
	{
		std::printf("%d failure(s)\n", g_failures);
		return 1;
	}
	std::printf("all clip row flag tests passed\n");
	return 0;
}
