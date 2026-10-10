// Unit tests for src/ListQueryBuilder.h -- FillList SQL/filter assembly, no MFC.
//
// Expected strings are copied character-identical from the shipped code
// (src/QPasteWnd.cpp:1516-1759, src/FormatSQL.h:20 for the CFormatSQL paren
// wrap) and independently hand-reviewed against those lines -- never taken
// from the header under test. Any mismatch means the extraction drifted
// from the shipped query shapes and must be fixed in the header, not here.
//
// Build & run (local, no MFC needed):
//   g++ -std=c++17 -Wall -Wextra -I src tests/test_list_query_builder.cpp -o test_list_query_builder.exe
//   .\test_list_query_builder.exe
// Delete the exe afterwards to keep the repo root clean.
// or with MSVC:
//   cl /nologo /std:c++17 /W4 /EHsc /I src tests\test_list_query_builder.cpp /Fe:test_list_query_builder.exe
//   .\test_list_query_builder.exe

#include "../src/ListQueryBuilder.h"

#include <cstdio>
#include <string>

static int g_failures = 0;

#define CHECK_TRUE(cond) do { \
	if (!(cond)) { \
		g_failures++; \
		std::printf("FAIL %s:%ld  %s\n", __FILE__, (long)__LINE__, #cond); \
	} \
} while (0)

#define CHECK_WSTR_EQ(actual, expected) do { \
	const std::wstring a_ = (actual); \
	const std::wstring e_ = (expected); \
	if (a_ != e_) { \
		g_failures++; \
		std::printf("FAIL %s:%ld  %s\n  actual:   [%ls]\n  expected: [%ls]\n", \
			__FILE__, (long)__LINE__, #actual, a_.c_str(), e_.c_str()); \
	} \
} while (0)

using namespace ListQuery;

static void test_type_filter_sql()
{
	// QPasteWnd.cpp:1516-1535, literals copied verbatim.
	CHECK_WSTR_EQ(TypeFilterSql(ChipAll), L"");
	CHECK_WSTR_EQ(TypeFilterSql(ChipText),
		L"(Main.bIsGroup = 0 AND EXISTS (SELECT 1 FROM Data D WHERE D.lParentID = Main.lID AND D.strClipBoardFormat IN ('CF_UNICODETEXT', 'CF_TEXT')))");
	CHECK_WSTR_EQ(TypeFilterSql(ChipImage),
		L"(Main.bIsGroup = 0 AND EXISTS (SELECT 1 FROM Data D WHERE D.lParentID = Main.lID AND D.strClipBoardFormat IN ('CF_DIB', 'PNG')))");
	CHECK_WSTR_EQ(TypeFilterSql(ChipFile),
		L"(Main.bIsGroup = 0 AND EXISTS (SELECT 1 FROM Data D WHERE D.lParentID = Main.lID AND D.strClipBoardFormat = 'CF_HDROP'))");
	CHECK_WSTR_EQ(TypeFilterSql(ChipLink),
		L"(Main.bIsGroup = 0 AND (Main.mText LIKE 'http://%' OR Main.mText LIKE 'https://%' OR Main.mText LIKE 'www.%'))");
	CHECK_WSTR_EQ(TypeFilterSql(99), L"");
}

static void test_default_main_filter()
{
	// QPasteWnd.cpp:1588 default arm (m_bShowAllClipsInMainList=false).
	BaseView v = BaseViewSql(false, -1, false, false);
	CHECK_WSTR_EQ(v.filter,
		L"((Main.bIsGroup = 1 AND Main.lParentID = -1) OR (Main.bIsGroup = 0 AND Main.lParentID = -1))");
	CHECK_WSTR_EQ(v.sort,
		L"Main.stickyClipOrder DESC, Main.bIsGroup ASC, Main.clipOrder DESC");
	CHECK_TRUE(v.parentFilter.empty());
}

static void test_show_all_variants()
{
	// QPasteWnd.cpp:1579 ShowAll+groups (slower on large DBs, kept verbatim).
	BaseView g = BaseViewSql(false, -1, true, true);
	CHECK_WSTR_EQ(g.filter,
		L"((Main.bIsGroup = 1 AND Main.lParentID = -1) OR Main.bIsGroup = 0)");
	// QPasteWnd.cpp:1583 ShowAll without groups.
	BaseView n = BaseViewSql(false, -1, true, false);
	CHECK_WSTR_EQ(n.filter, L"(Main.bIsGroup = 0)");
	CHECK_WSTR_EQ(n.sort, MainSortSql());
}

static void test_starred_view()
{
	// QPasteWnd.cpp:1555 + 1559-1566.
	BaseView v = BaseViewSql(true, -1, false, false);
	CHECK_WSTR_EQ(v.filter, L"Main.bIsGroup = 0 AND Main.lDontAutoDelete > 0");
	CHECK_WSTR_EQ(v.sort, MainSortSql());
	CHECK_TRUE(v.parentFilter.empty());
	// Starred wins even when a group is selected.
	BaseView inGroup = BaseViewSql(true, 7, false, false);
	CHECK_WSTR_EQ(inGroup.filter, v.filter);
}

static void test_group_view()
{
	// QPasteWnd.cpp:1595-1604 with m_GroupID = 7.
	BaseView v = BaseViewSql(false, 7, false, false);
	CHECK_WSTR_EQ(v.filter, L"Main.lParentID = 7");
	CHECK_WSTR_EQ(v.parentFilter, L"Main.lParentID = 7");
	CHECK_WSTR_EQ(v.sort,
		L"Main.stickyClipGroupOrder DESC, Main.bIsGroup ASC, Main.clipGroupOrder DESC");
}

static void test_empty_search_stored_filter()
{
	// QPasteWnd.cpp:1619-1623.
	CHECK_WSTR_EQ(EmptySearchStoredSql(true, L"Main.bIsGroup = 0 AND Main.lDontAutoDelete > 0"),
		L"Main.bIsGroup = 0 AND Main.lDontAutoDelete > 0");
	CHECK_WSTR_EQ(EmptySearchStoredSql(false,
		L"((Main.bIsGroup = 1 AND Main.lParentID = -1) OR (Main.bIsGroup = 0 AND Main.lParentID = -1))"), L"");
}

static void test_search_option_gating()
{
	// QPasteWnd.cpp:1634-1635: description defaults on when the others are off.
	CHECK_TRUE(ShouldSearchDescription(true, false, false));
	CHECK_TRUE(ShouldSearchDescription(false, false, false));
	CHECK_TRUE(!ShouldSearchDescription(false, true, false));
	CHECK_TRUE(!ShouldSearchDescription(false, false, true));
	// QPasteWnd.cpp:1643-1645, 1659-1661: /q and /f prefixes force the mode.
	CHECK_TRUE(ShouldSearchQuickPaste(L"/q foo", false));
	CHECK_TRUE(ShouldSearchQuickPaste(L"\\q foo", false));
	CHECK_TRUE(ShouldSearchQuickPaste(L"foo", true));
	CHECK_TRUE(!ShouldSearchQuickPaste(L"foo", false));
	CHECK_TRUE(ShouldSearchFullText(L"/f foo", false));
	CHECK_TRUE(ShouldSearchFullText(L"\\f foo", false));
	CHECK_TRUE(ShouldSearchFullText(L"foo", true));
	CHECK_TRUE(!ShouldSearchFullText(L"foo", false));
	// QPasteWnd.cpp:1652, 1668: Mid(3) strips the 3-char prefix.
	CHECK_WSTR_EQ(StripSearchPrefix3(L"/q foo"), L"foo");
	CHECK_WSTR_EQ(StripSearchPrefix3(L"\\f bar"), L"bar");
}

static void test_like_term_helpers()
{
	// Hand-derived from src/FormatSQL.cpp:32 (quote doubling) and :142-165
	// (backslash-escaping of LIKE metachars with ESCAPE '\').
	CHECK_WSTR_EQ(EscapeLikeTerm(L"hello"), L"hello");
	CHECK_WSTR_EQ(EscapeLikeTerm(L"a'b"), L"a''b");
	CHECK_WSTR_EQ(EscapeLikeTerm(L"100%"), L"100\\%");
	CHECK_WSTR_EQ(EscapeLikeTerm(L"a_b"), L"a\\_b");
	CHECK_WSTR_EQ(EscapeLikeTerm(L"a\\b"), L"a\\\\b");
	CHECK_WSTR_EQ(EscapeLikeTerm(L""), L"");
	// Combined metachars: % -> \%, _ -> \_, ' -> '', \ -> \\.
	CHECK_WSTR_EQ(EscapeLikeTerm(L"%_'\\"), L"\\%\\_''\\\\");

	// Escaped-literal predicate: term inlined only after escaping.
	CHECK_WSTR_EQ(LikePredicate(L"Main.mText", L"e"),
		L"Main.mText LIKE '%e%' ESCAPE '\\'");
	CHECK_WSTR_EQ(LikePredicate(L"Main.mText", L"a'b%"),
		L"Main.mText LIKE '%a''b\\%%' ESCAPE '\\'");

	// Parameterized template: no quotes around the term; the ESCAPED term
	// travels in params (EscapeLikeTerm rules: % -> \%, _ -> \_, ' -> '',
	// \ -> \\) so metacharacters stay literal under the template's
	// ESCAPE '\'. Hand-derived: "a'b%" -> "a''b\%" (quote doubled, % escaped).
	ParamQuery q = LikeParamPredicate(L"Main.mText", L"a'b%");
	CHECK_WSTR_EQ(q.sql, L"Main.mText LIKE ('%' || ? || '%') ESCAPE '\\'");
	CHECK_TRUE(q.params.size() == 1);
	CHECK_WSTR_EQ(q.params[0], L"a''b\\%");
	// Literal-percent term: "100%" binds as "100\%" so it matches a real
	// percent sign instead of acting as a wildcard suffix.
	ParamQuery pct = LikeParamPredicate(L"Main.mText", L"100%");
	CHECK_TRUE(pct.params.size() == 1);
	CHECK_WSTR_EQ(pct.params[0], L"100\\%");
}

static void test_description_convergence()
{
	// CFormatSQL::GetSQLString wraps the LIKE in parens (FormatSQL.h:20);
	// QPasteWnd.cpp:1689-1717 wraps the converged group in parens again.
	const std::wstring desc = L"(Main.mText LIKE '%hello%')";
	CHECK_WSTR_EQ(ConvergeSearchFilter(desc, L"", L"", L"", false),
		L"((Main.mText LIKE '%hello%'))");

	// Description + quick-paste OR together (QPasteWnd.cpp:1696-1704).
	const std::wstring qp = L"(Main.QuickPasteText LIKE '%hello%')";
	CHECK_WSTR_EQ(ConvergeSearchFilter(desc, qp, L"", L"", false),
		L"((Main.mText LIKE '%hello%') OR (Main.QuickPasteText LIKE '%hello%'))");
}

static void test_full_text_join_and_distinct()
{
	// QPasteWnd.cpp:1663.
	CHECK_WSTR_EQ(FullTextDataJoinSql(), L"INNER JOIN Data on Data.lParentID = Main.lID");

	// QPasteWnd.cpp:1675 Insert(1, ...) lands after the opening paren.
	const std::wstring raw = L"(Data.ooData LIKE '%e%')";
	CHECK_WSTR_EQ(ApplyFullTextFormatPrefix(raw),
		L"(Data.strClipBoardFormat = 'CF_UNICODETEXT' AND Data.ooData LIKE '%e%')");
	CHECK_WSTR_EQ(ApplyFullTextFormatPrefix(L""), L"");

	// QPasteWnd.cpp:1678-1686: DISTINCT only when full-text runs WITH
	// another predicate.
	CHECK_WSTR_EQ(DistinctForSearch(true, L"(a)", L""), L"DISTINCT");
	CHECK_WSTR_EQ(DistinctForSearch(true, L"", L"(b)"), L"DISTINCT");
	CHECK_WSTR_EQ(DistinctForSearch(true, L"", L""), L"");
	CHECK_WSTR_EQ(DistinctForSearch(false, L"(a)", L""), L"");

	// Full three-way OR convergence (QPasteWnd.cpp:1706-1715).
	const std::wstring desc = L"(Main.mText LIKE '%e%')";
	const std::wstring ft = ApplyFullTextFormatPrefix(raw);
	CHECK_WSTR_EQ(ConvergeSearchFilter(desc, L"", ft, L"", false),
		L"((Main.mText LIKE '%e%') OR (Data.strClipBoardFormat = 'CF_UNICODETEXT' AND Data.ooData LIKE '%e%'))");
}

static void test_search_convergence_with_view_filters()
{
	// QPasteWnd.cpp:1719-1730: group parent filter ANDed back in.
	CHECK_WSTR_EQ(ConvergeSearchFilter(L"(Main.mText LIKE '%x%')", L"", L"", L"Main.lParentID = 7", false),
		L"((Main.mText LIKE '%x%')) AND Main.lParentID = 7");
	// Starred search ANDs the starred predicate too.
	CHECK_WSTR_EQ(ConvergeSearchFilter(L"(Main.mText LIKE '%x%')", L"", L"", L"", true),
		L"((Main.mText LIKE '%x%')) AND (Main.bIsGroup = 0 AND Main.lDontAutoDelete > 0)");
}

static void test_apply_type_filter()
{
	// QPasteWnd.cpp:1741-1751: chips stack on top of every view via AND.
	const std::wstring base =
		L"((Main.bIsGroup = 1 AND Main.lParentID = -1) OR (Main.bIsGroup = 0 AND Main.lParentID = -1))";
	CHECK_WSTR_EQ(ApplyTypeFilter(base, ChipAll), base);
	CHECK_WSTR_EQ(ApplyTypeFilter(base, ChipText),
		base + L" AND (Main.bIsGroup = 0 AND EXISTS (SELECT 1 FROM Data D WHERE D.lParentID = Main.lID AND D.strClipBoardFormat IN ('CF_UNICODETEXT', 'CF_TEXT')))");
	// Empty base takes the chip predicate bare (no leading AND).
	CHECK_WSTR_EQ(ApplyTypeFilter(L"", ChipFile),
		L"(Main.bIsGroup = 0 AND EXISTS (SELECT 1 FROM Data D WHERE D.lParentID = Main.lID AND D.strClipBoardFormat = 'CF_HDROP'))");
}

static void test_full_text_exists_rewrite()
{
	// Hand-derived: the EXISTS rewrite of the b' JOIN+DISTINCT shape.
	// Input fragment is the ApplyFullTextFormatPrefix output (parens incl).
	const std::wstring raw = L"(Data.ooData LIKE '%e%')";
	const std::wstring guarded = ApplyFullTextFormatPrefix(raw);
	CHECK_WSTR_EQ(FullTextExistsPredicate(guarded),
		L"(EXISTS (SELECT 1 FROM Data WHERE Data.lParentID = Main.lID AND "
		L"(Data.strClipBoardFormat = 'CF_UNICODETEXT' AND Data.ooData LIKE '%e%')))");
	CHECK_WSTR_EQ(FullTextExistsPredicate(L""), L"");

	// No JOIN fan-out, so DISTINCT is never needed on the EXISTS path;
	// the default (joinedData=true) keeps the shipped JOIN decision.
	CHECK_WSTR_EQ(DistinctForSearch(true, L"(a)", L"", false), L"");
	CHECK_WSTR_EQ(DistinctForSearch(true, L"", L"(b)", false), L"");
	CHECK_WSTR_EQ(DistinctForSearch(true, L"(a)", L""), L"DISTINCT");

	// COUNT vs PAGE split stays clean: COUNT(Main.lID), no DISTINCT, no JOIN.
	const std::wstring desc = L"(Main.mText LIKE '%e%')";
	const std::wstring exists = FullTextExistsPredicate(guarded);
	const std::wstring filter = ConvergeSearchFilter(desc, L"", exists, L"", false);
	CHECK_WSTR_EQ(filter,
		L"((Main.mText LIKE '%e%') OR (EXISTS (SELECT 1 FROM Data WHERE "
		L"Data.lParentID = Main.lID AND (Data.strClipBoardFormat = 'CF_UNICODETEXT' AND Data.ooData LIKE '%e%'))))");
	CHECK_WSTR_EQ(BuildCountSql(L"", L"", filter),
		L"SELECT COUNT( Main.lID) FROM Main  where ((Main.mText LIKE '%e%') OR (EXISTS (SELECT 1 FROM Data WHERE "
		L"Data.lParentID = Main.lID AND (Data.strClipBoardFormat = 'CF_UNICODETEXT' AND Data.ooData LIKE '%e%'))))");
	const std::wstring page = BuildPageSql(L"", L"", filter, MainSortSql());
	CHECK_TRUE(page.rfind(L"SELECT  Main.lID", 0) == 0);
	CHECK_TRUE(page.find(L"DISTINCT") == std::wstring::npos);
	CHECK_TRUE(page.find(L"JOIN") == std::wstring::npos);
	CHECK_TRUE(page.find(L"EXISTS (SELECT 1 FROM Data WHERE Data.lParentID = Main.lID") != std::wstring::npos);
	CHECK_TRUE(page.find(L"order by Main.stickyClipOrder DESC, Main.bIsGroup ASC, Main.clipOrder DESC") != std::wstring::npos);
}

static void test_data_child_guard()
{
	// Hand-derived guard literal: any Main row must own >= 1 Data row,
	// reproducing the old INNER JOIN's implicit side condition.
	CHECK_WSTR_EQ(HasDataChildGuardSql(),
		L"(EXISTS (SELECT 1 FROM Data WHERE Data.lParentID = Main.lID))");

	// Guarded convergence: guard lands right after the OR group, before
	// parent/starred filters.
	const std::wstring desc = L"(Main.mText LIKE '%e%')";
	const std::wstring exists =
		L"(EXISTS (SELECT 1 FROM Data WHERE Data.lParentID = Main.lID AND "
		L"(Data.strClipBoardFormat = 'CF_UNICODETEXT' AND Data.ooData LIKE '%e%')))";
	CHECK_WSTR_EQ(ConvergeSearchFilter(desc, L"", exists, L"", false, true),
		L"((Main.mText LIKE '%e%') OR (EXISTS (SELECT 1 FROM Data WHERE "
		L"Data.lParentID = Main.lID AND (Data.strClipBoardFormat = 'CF_UNICODETEXT' AND Data.ooData LIKE '%e%')))) "
		L"AND (EXISTS (SELECT 1 FROM Data WHERE Data.lParentID = Main.lID))");
	// Guard + parent + starred ordering.
	CHECK_WSTR_EQ(ConvergeSearchFilter(desc, L"", exists, L"Main.lParentID = 7", true, true),
		L"((Main.mText LIKE '%e%') OR (EXISTS (SELECT 1 FROM Data WHERE "
		L"Data.lParentID = Main.lID AND (Data.strClipBoardFormat = 'CF_UNICODETEXT' AND Data.ooData LIKE '%e%')))) "
		L"AND (EXISTS (SELECT 1 FROM Data WHERE Data.lParentID = Main.lID)) "
		L"AND Main.lParentID = 7 "
		L"AND (Main.bIsGroup = 0 AND Main.lDontAutoDelete > 0)");
	// Flag off (default) keeps the unguarded text byte-identical.
	CHECK_WSTR_EQ(ConvergeSearchFilter(desc, L"", exists, L"", false, false),
		ConvergeSearchFilter(desc, L"", exists, L"", false));
}

// The pre-rewrite QPasteWnd.cpp:1689-1730 block, replicated line-for-line
// (CString CString::Format spells out as wstring ops) as the identity oracle:
// any drift between this and ConvergeSearchFilter(flag=false) fails below.
static std::wstring OldConverge(const std::wstring& descriptionSql,
	const std::wstring& quickPasteSql,
	const std::wstring& fullTextSql,
	const std::wstring& parentFilter,
	bool showStarred)
{
	std::wstring strFilter = L"(";
	if (descriptionSql != L"")
		strFilter += descriptionSql;
	if (quickPasteSql != L"")
	{
		if (descriptionSql != L"")
			strFilter += L" OR ";
		strFilter += quickPasteSql;
	}
	if (fullTextSql != L"")
	{
		if (descriptionSql != L"" || quickPasteSql != L"")
			strFilter += L" OR ";
		strFilter += fullTextSql;
	}
	strFilter += L")";
	if (parentFilter != L"")
	{
		strFilter += L" AND ";
		strFilter += parentFilter;
	}
	if (showStarred)
	{
		strFilter += L" AND (";
		strFilter += StarredFilterSql();
		strFilter += L")";
	}
	return strFilter;
}

// The pre-rewrite QPasteWnd.cpp:1741-1751 chip block, replicated as oracle.
static std::wstring OldApplyType(const std::wstring& filter, int typeFilter)
{
	if (typeFilter != ChipAll)
	{
		std::wstring csTypeFilter = TypeFilterSql(typeFilter);
		if (!csTypeFilter.empty())
		{
			if (filter.empty())
				return csTypeFilter;
			return filter + L" AND " + csTypeFilter;
		}
	}
	return filter;
}

static void test_converge_old_vs_new_identity()
{
	const std::wstring desc = L"(Main.mText LIKE '%x%')";
	const std::wstring qp = L"(Main.QuickPasteText LIKE '%x%')";
	const std::wstring ft = L"(Data.strClipBoardFormat = 'CF_UNICODETEXT' AND Data.ooData LIKE '%x%')";
	const std::wstring parent = L"Main.lParentID = 7";
	// Fragment matrix x parent/starred matrix; flag=false must equal OldConverge.
	const std::wstring descs[] = { L"", desc };
	const std::wstring qps[] = { L"", qp };
	const std::wstring fts[] = { L"", ft };
	for (const auto& d : descs)
		for (const auto& q : qps)
			for (const auto& f : fts)
				for (const auto& p : { std::wstring(L""), parent })
					for (bool st : { false, true })
						CHECK_WSTR_EQ(ConvergeSearchFilter(d, q, f, p, st, false), OldConverge(d, q, f, p, st));
	// Explicit starred+search and parent+search pins (readable failures).
	CHECK_WSTR_EQ(ConvergeSearchFilter(desc, L"", L"", L"", true),
		L"((Main.mText LIKE '%x%')) AND (Main.bIsGroup = 0 AND Main.lDontAutoDelete > 0)");
	CHECK_WSTR_EQ(ConvergeSearchFilter(desc, qp, ft, parent, true),
		OldConverge(desc, qp, ft, parent, true));
	// Chip block: empty base assigns bare, non-empty ANDs; all chip ids.
	const std::wstring base =
		L"((Main.bIsGroup = 1 AND Main.lParentID = -1) OR (Main.bIsGroup = 0 AND Main.lParentID = -1))";
	for (int chip : { (int)ChipAll, (int)ChipText, (int)ChipImage, (int)ChipFile, (int)ChipLink, 99 })
	{
		CHECK_WSTR_EQ(ApplyTypeFilter(base, chip), OldApplyType(base, chip));
		CHECK_WSTR_EQ(ApplyTypeFilter(L"", chip), OldApplyType(L"", chip));
	}
}

static void test_count_vs_page_default_view()
{
	// QPasteWnd.cpp:1754-1759. Note the pinned spacing: "COUNT( Main.lID)"
	// and "SELECT  Main.lID" / "FROM Main  where" carry the double spaces
	// the shipped CString::Format leaves when IsDistinct/dataJoin are empty.
	BaseView v = BaseViewSql(false, -1, false, false);
	CHECK_WSTR_EQ(BuildCountSql(L"", L"", v.filter),
		L"SELECT COUNT( Main.lID) FROM Main  where ((Main.bIsGroup = 1 AND Main.lParentID = -1) OR (Main.bIsGroup = 0 AND Main.lParentID = -1))");
	CHECK_WSTR_EQ(BuildPageSql(L"", L"", v.filter, v.sort),
		L"SELECT  Main.lID, Main.mText, Main.lParentID, Main.lDontAutoDelete, "
		L"Main.lShortCut, Main.bIsGroup, Main.QuickPasteText, Main.clipOrder, Main.clipGroupOrder, "
		L"Main.stickyClipOrder, Main.stickyClipGroupOrder, Main.lDate, Main.lastPasteDate FROM Main  "
		L"where ((Main.bIsGroup = 1 AND Main.lParentID = -1) OR (Main.bIsGroup = 0 AND Main.lParentID = -1)) "
		L"order by Main.stickyClipOrder DESC, Main.bIsGroup ASC, Main.clipOrder DESC");
}

static void test_count_vs_page_full_text_view()
{
	// Full-text + description: DISTINCT + INNER JOIN shape (baseline b').
	const std::wstring desc = L"(Main.mText LIKE '%e%')";
	const std::wstring ft = ApplyFullTextFormatPrefix(L"(Data.ooData LIKE '%e%')");
	const std::wstring distinct = DistinctForSearch(true, desc, L"");
	const std::wstring join = FullTextDataJoinSql();
	const std::wstring filter = ConvergeSearchFilter(desc, L"", ft, L"", false);
	CHECK_WSTR_EQ(distinct, L"DISTINCT");
	CHECK_WSTR_EQ(BuildCountSql(distinct, join, filter),
		L"SELECT COUNT(DISTINCT Main.lID) FROM Main INNER JOIN Data on Data.lParentID = Main.lID "
		L"where ((Main.mText LIKE '%e%') OR (Data.strClipBoardFormat = 'CF_UNICODETEXT' AND Data.ooData LIKE '%e%'))");
	const std::wstring page = BuildPageSql(distinct, join, filter, MainSortSql());
	CHECK_TRUE(page.rfind(L"SELECT DISTINCT Main.lID", 0) == 0);
	CHECK_TRUE(page.find(L"INNER JOIN Data on Data.lParentID = Main.lID") != std::wstring::npos);
	CHECK_TRUE(page.find(L"order by Main.stickyClipOrder DESC, Main.bIsGroup ASC, Main.clipOrder DESC") != std::wstring::npos);
}

static void test_send_to_friend_batch_sql()
{
	// Hand-derived from the shipped per-ID loop (QPasteWnd.cpp SendToFriendbyPos
	// + CClip::LoadFormats, Clip.cpp:1668-1672): Main side selects the same
	// mText column, Data side the same four columns with the same lID-desc
	// order, now grouped per parent so each clip keeps its format order.
	const std::vector<int> ids = { 7, 3, 42 };
	CHECK_WSTR_EQ(JoinIdList(ids), L"7,3,42");
	CHECK_WSTR_EQ(BuildMainBatchSql(ids),
		L"SELECT lID, mText FROM Main WHERE lID IN (7,3,42)");
	CHECK_WSTR_EQ(BuildDataBatchSql(ids),
		L"SELECT lID, lParentID, strClipBoardFormat, ooData FROM Data WHERE lParentID IN (7,3,42) ORDER BY lParentID, Data.lID desc");
	// Single ID: no trailing comma, still a valid IN list.
	const std::vector<int> one = { 9 };
	CHECK_WSTR_EQ(BuildMainBatchSql(one),
		L"SELECT lID, mText FROM Main WHERE lID IN (9)");
	CHECK_WSTR_EQ(BuildDataBatchSql(one),
		L"SELECT lID, lParentID, strClipBoardFormat, ooData FROM Data WHERE lParentID IN (9) ORDER BY lParentID, Data.lID desc");
	// Empty input never emits "IN ()": the caller keeps its count <= 0 guard.
	CHECK_WSTR_EQ(BuildMainBatchSql({}), L"");
	CHECK_WSTR_EQ(BuildDataBatchSql({}), L"");
	CHECK_WSTR_EQ(JoinIdList({}), L"");
}

static void test_goto_rank_sql()
{
	// Hand-derived: RANK over the main-list sort (MainSortSql) on the
	// same top-level filter the COUNT shape used, minus 1 for the 0-based
	// list position; target addressed by lID so no float key formatting.
	// RANK (not ROW_NUMBER) so tied sort keys share one index, matching
	// the old COUNT(*)-of-strictly-before fallback for every row.
	const std::wstring filter =
		L"((Main.bIsGroup = 1 AND Main.lParentID = -1) OR (Main.bIsGroup = 0 AND Main.lParentID = -1))";
	CHECK_WSTR_EQ(BuildGoToRankSql(filter, 1),
		L"SELECT rank FROM (SELECT Main.lID, RANK() OVER (ORDER BY "
		L"Main.stickyClipOrder DESC, Main.bIsGroup ASC, Main.clipOrder DESC) - 1 AS rank "
		L"FROM Main where ((Main.bIsGroup = 1 AND Main.lParentID = -1) OR (Main.bIsGroup = 0 AND Main.lParentID = -1))) "
		L"WHERE lID = 1");
	// Window key is exactly the list sort: rebuilding the expectation from
	// MainSortSql() must equal the pinned literal above.
	CHECK_WSTR_EQ(BuildGoToRankSql(L"(Main.bIsGroup = 0)", 5),
		L"SELECT rank FROM (SELECT Main.lID, RANK() OVER (ORDER BY " +
		MainSortSql() + L") - 1 AS rank FROM Main where (Main.bIsGroup = 0)) WHERE lID = 5");
}

int main()
{
	test_type_filter_sql();
	test_default_main_filter();
	test_show_all_variants();
	test_starred_view();
	test_group_view();
	test_empty_search_stored_filter();
	test_search_option_gating();
	test_like_term_helpers();
	test_description_convergence();
	test_full_text_join_and_distinct();
	test_search_convergence_with_view_filters();
	test_apply_type_filter();
	test_count_vs_page_default_view();
	test_count_vs_page_full_text_view();
	test_full_text_exists_rewrite();
	test_data_child_guard();
	test_converge_old_vs_new_identity();
	test_send_to_friend_batch_sql();
	test_goto_rank_sql();

	if (g_failures > 0)
	{
		std::printf("%d failure(s)\n", g_failures);
		return 1;
	}
	std::printf("all list query builder tests passed\n");
	return 0;
}
