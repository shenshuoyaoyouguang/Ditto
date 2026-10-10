#pragma once

// Pure SQL/filter assembly extracted from CQPasteWnd::FillList and its
// TypeFilterSql helper (src/QPasteWnd.cpp:1516-1759). No MFC/CString/
// windows.h dependencies: only std::wstring, so tests/
// test_list_query_builder.cpp can compile this header outside the main
// project -- the same seam pattern as src/ThemeColorMath.h.
//
// EXTRACTION ONLY. Every string literal below is copied character-identical
// from the shipped code (including lowercase "where"/"order by" and the
// double spaces the CString::Format "%s" substitutions leave when
// IsDistinct/dataJoin are empty). Later tasks will rewrite FillList to call
// these helpers; until then the call sites are untouched and these
// functions must NOT change any query shape (the T1 baseline proves the
// main COUNT/page are index SEEKs while LIKE '%x%' is a SCAN, so even a
// whitespace change would invalidate the pinned plans).

#include <string>
#include <vector>

namespace ListQuery {

// Chip ids mirror enum ChipFilter in src/ChipBar.h (CHIP_ALL = 0 ..).
// The header is NOT included here because it pulls in MFC (CWnd/DPI.h);
// the values are re-declared so the builder stays dependency-free.
enum Chip
{
	ChipAll = 0,
	ChipText = 1,
	ChipImage = 2,
	ChipFile = 3,
	ChipLink = 4,
};

// QPasteWnd.cpp:1516-1535. WHY each predicate carries "Main.bIsGroup = 0":
// chips filter clips, never groups, and the EXISTS subqueries hit
// Data(lParentID, strClipBoardFormat) which the Data_ParentId_Format index
// covers. WHY CF_TEXT sits next to CF_UNICODETEXT: Ditto stores
// ASCII-only clips under the ANSI CF_TEXT format, so testing only
// CF_UNICODETEXT silently dropped them. WHY 'PNG' sits next to CF_DIB:
// Ditto registers its own PNG clipboard format for image clips stored as
// png rather than as a DIB.
inline std::wstring TypeFilterSql(int typeFilter)
{
	switch (typeFilter)
	{
	case ChipText:
		return L"(Main.bIsGroup = 0 AND EXISTS (SELECT 1 FROM Data D WHERE D.lParentID = Main.lID AND D.strClipBoardFormat IN ('CF_UNICODETEXT', 'CF_TEXT')))";
	case ChipImage:
		return L"(Main.bIsGroup = 0 AND EXISTS (SELECT 1 FROM Data D WHERE D.lParentID = Main.lID AND D.strClipBoardFormat IN ('CF_DIB', 'PNG')))";
	case ChipFile:
		return L"(Main.bIsGroup = 0 AND EXISTS (SELECT 1 FROM Data D WHERE D.lParentID = Main.lID AND D.strClipBoardFormat = 'CF_HDROP'))";
	case ChipLink:
		return L"(Main.bIsGroup = 0 AND (Main.mText LIKE 'http://%' OR Main.mText LIKE 'https://%' OR Main.mText LIKE 'www.%'))";
	}

	return L"";
}

// QPasteWnd.cpp:1555. Starred view: non-group clips kept from auto-delete.
inline std::wstring StarredFilterSql()
{
	return L"Main.bIsGroup = 0 AND Main.lDontAutoDelete > 0";
}

// QPasteWnd.cpp:1560-1563 (starred + top-level sort). WHY this exact key
// order: it matches the Main_TopLevel covering index
// (stickyClipOrder DESC, bIsGroup ASC, clipOrder DESC) so the paged SELECT
// seeks instead of sorting.
inline std::wstring MainSortSql()
{
	return L"Main.stickyClipOrder DESC, Main.bIsGroup ASC, Main.clipOrder DESC";
}

// QPasteWnd.cpp:1595-1597 (group-view sort). WHY the group key order: it
// matches Main_InGroup2 (lParentId, stickyClipGroupOrder DESC, bIsGroup,
// clipGroupOrder DESC).
inline std::wstring GroupSortSql()
{
	return L"Main.stickyClipGroupOrder DESC, Main.bIsGroup ASC, Main.clipGroupOrder DESC";
}

// QPasteWnd.cpp:1574-1589. Top-level (m_GroupID < 0) filter. WHY three
// shapes: ShowAll+groups keeps group rows alongside clips (MULTI-INDEX OR
// plan), ShowAll without groups keeps clips only, and the default keeps
// only top-level rows of each kind (lParentID = -1 on both arms).
inline std::wstring TopLevelFilterSql(bool showAllClipsInMainList, bool showGroupsInMainList)
{
	if (showAllClipsInMainList)
	{
		if (showGroupsInMainList)
		{
			// Found to be slower on large databases (kept verbatim).
			return L"((Main.bIsGroup = 1 AND Main.lParentID = -1) OR Main.bIsGroup = 0)";
		}
		return L"(Main.bIsGroup = 0)";
	}
	return L"((Main.bIsGroup = 1 AND Main.lParentID = -1) OR (Main.bIsGroup = 0 AND Main.lParentID = -1))";
}

// QPasteWnd.cpp:1601-1604. Group view filter: CString strFilter.Format(
// _T("Main.lParentID = %d"), m_GroupID); strParentFilter gets the same
// value so later search convergence can AND it back in.
inline std::wstring GroupFilterSql(int groupId)
{
	return L"Main.lParentID = " + std::to_wstring(groupId);
}

struct BaseView
{
	std::wstring filter; // strFilter going into the search/type-filter stages
	std::wstring parentFilter; // strParentFilter, non-empty only for group views
	std::wstring sort; // csSort
};

// QPasteWnd.cpp:1553-1608. Picks the starred / top-level / group branch.
// groupId is theApp.m_GroupID (< 0 = top level). Mirrors the shipped
// if/else chain exactly: starred wins over everything, then groupId < 0
// selects the top-level arm, otherwise the group arm (whose inner
// "groupId >= 0" guard is always true once the outer else is taken).
inline BaseView BaseViewSql(bool showStarred, int groupId, bool showAllClipsInMainList, bool showGroupsInMainList)
{
	BaseView view;
	if (showStarred)
	{
		view.sort = MainSortSql();
		view.filter = StarredFilterSql();
	}
	else if (groupId < 0)
	{
		view.sort = MainSortSql();
		view.filter = TopLevelFilterSql(showAllClipsInMainList, showGroupsInMainList);
	}
	else
	{
		view.sort = GroupSortSql();
		view.filter = GroupFilterSql(groupId);
		view.parentFilter = view.filter;
	}
	return view;
}

// QPasteWnd.cpp:1619-1623. Empty search box: the stored search predicate
// (m_strSQLSearch) keeps the starred filter but is cleared for every other
// view, because the base strFilter already constrains those queries.
inline std::wstring EmptySearchStoredSql(bool showStarred, const std::wstring& baseFilter)
{
	return showStarred ? baseFilter : L"";
}

// Search-option gating, QPasteWnd.cpp:1634/1643-1645/1659-1661. Pure
// predicates over the CGetSetOptions flags so the header never touches the
// options layer. WHY description defaults on: "If other are off then
// always search the description".
inline bool ShouldSearchDescription(bool searchDescription, bool searchFullText, bool searchQuickPaste)
{
	return searchDescription || (!searchFullText && !searchQuickPaste);
}

inline bool HasQuickPastePrefix(const std::wstring& search)
{
	return search.compare(0, 3, L"/q ") == 0 || search.compare(0, 3, L"\\q ") == 0;
}

inline bool HasFullTextPrefix(const std::wstring& search)
{
	return search.compare(0, 3, L"/f ") == 0 || search.compare(0, 3, L"\\f ") == 0;
}

inline bool ShouldSearchQuickPaste(const std::wstring& search, bool searchQuickPasteOpt)
{
	return HasQuickPastePrefix(search) || searchQuickPasteOpt;
}

inline bool ShouldSearchFullText(const std::wstring& search, bool searchFullTextOpt)
{
	return HasFullTextPrefix(search) || searchFullTextOpt;
}

// QPasteWnd.cpp:1652/1668. csSQLSearch = csSQLSearch.Mid(3): drops the
// "/q " (or backslash) prefix before CFormatSQL::Parse. WHY the caller
// order matters: description Parse runs on the RAW text (before the
// quick-paste strip), while full-text Parse runs on the text AFTER the
// quick-paste strip, so the helpers must be applied in that order.
inline std::wstring StripSearchPrefix3(const std::wstring& search)
{
	return search.size() >= 3 ? search.substr(3) : L"";
}

// QPasteWnd.cpp search path builds LIKE predicates by concatenating the raw
// term between quotes (CFormatSQL::AddToSQL, src/FormatSQL.cpp:131-165).
// These helpers separate the TERM from the TEMPLATE so a future bind-API
// cutover can stop inlining terms entirely:
//
// - EscapeLikeTerm: quote-doubling + LIKE-metachar escaping, mirroring
//   CFormatSQL::Parse line 32 ("'" -> "''") and AddToSQL lines 142-165
//   ("%" -> "\%", ESCAPE '\'). Order matters: backslash first, otherwise
//   the escape character itself gets re-escaped.
// - LikePredicate: escaped-literal shape; identical rows to CFormatSQL
//   output for the same term (always carries ESCAPE '\', which is a no-op
//   when the term has no metachars, so no branch like AddToSQL:156-165).
// - ParamQuery/LikeParamPredicate: '?' template
//   (column LIKE ('%' || ? || '%') ESCAPE '\') with NO quotes around the
//   term; the escaped term travels in params, never concatenated.
// DEFERRED (measured, not assumed): executing the '?' shape needs a bind
// API, but the FillList worker path (CQPasteWndThread::OnLoadItems /
// OnSetListCount) only takes raw SQL via CppSQLite3::execQuery/execScalar,
// which have no bind overload, and those files are out of scope — so
// FillList keeps the escaped-literal shape. The template + params struct
// is unit-tested here for the cutover, not wired to execution.
inline std::wstring EscapeLikeTerm(const std::wstring& term)
{
	std::wstring out;
	out.reserve(term.size() + 4);
	for (wchar_t c : term)
	{
		if (c == L'\\')
			out += L"\\\\";
		else if (c == L'%')
			out += L"\\%";
		else if (c == L'_')
			out += L"\\_";
		else if (c == L'\'')
			out += L"''";
		else
			out += c;
	}
	return out;
}

inline std::wstring LikePredicate(const std::wstring& column, const std::wstring& term)
{
	return column + L" LIKE '%" + EscapeLikeTerm(term) + L"%' ESCAPE '\\'";
}

struct ParamQuery
{
	std::wstring sql; // '?' template, no term inlined
	std::vector<std::wstring> params; // raw terms, bound (never concatenated)
};

inline ParamQuery LikeParamPredicate(const std::wstring& column, const std::wstring& term)
{
	ParamQuery query;
	query.sql = column + L" LIKE ('%' || ? || '%') ESCAPE '\\'";
	query.params.push_back(EscapeLikeTerm(term));
	return query;
}

// QPasteWnd.cpp:1663. Full-text search joins the Data table for the
// ooData blob column.
inline std::wstring FullTextDataJoinSql()
{
	return L"INNER JOIN Data on Data.lParentID = Main.lID";
}

// QPasteWnd.cpp:1675. fullTextSql.Insert(1, "Data.strClipBoardFormat =
// 'CF_UNICODETEXT' AND "): position 1 is right after the opening paren of
// CFormatSQL::GetSQLString() ("(" + where + ")"), constraining the blob
// LIKE to unicode-text rows.
inline std::wstring ApplyFullTextFormatPrefix(const std::wstring& fullTextSql)
{
	if (fullTextSql.empty())
		return fullTextSql;
	std::wstring out = fullTextSql;
	out.insert(1, L"Data.strClipBoardFormat = 'CF_UNICODETEXT' AND ");
	return out;
}

// QPasteWnd.cpp:1678-1686. DISTINCT decision: joining Data fans out to one
// row per Data record, so when full-text runs TOGETHER with a description
// or quick-paste predicate the SELECT must de-duplicate (at the cost of the
// TEMP B-TREE FOR DISTINCT step in the baseline plan). Full-text alone
// needs no DISTINCT. joinedData=false selects the EXISTS rewrite below
// (no JOIN fan-out, so DISTINCT is never needed); it defaults to true so
// existing callers keep the shipped JOIN shape.
inline std::wstring DistinctForSearch(bool fullTextActive, const std::wstring& descriptionSql, const std::wstring& quickPasteSql, bool joinedData = true)
{
	if (!joinedData)
		return L"";
	if (fullTextActive && (!descriptionSql.empty() || !quickPasteSql.empty()))
		return L"DISTINCT";
	return L"";
}

// Full-text WITHOUT the JOIN: wraps the format-guarded CFormatSQL fragment
// (ApplyFullTextFormatPrefix output, parens included) in a correlated
// EXISTS subquery so the query stays on FROM Main with dataJoin empty.
// WHY it is faster (measured §4 b' oracle): the JOIN shape drives from
// SCAN Data + SEARCH Main(rowid) + TEMP B-TREE FOR DISTINCT (+ TEMP B-TREE
// FOR ORDER BY on the page), while EXISTS keeps SCAN Main and probes Data
// per row via SEARCH Data USING INDEX Data_ParentId_Format (lParentID=?)
// with no TEMP B-TREE at all. COUNT 0.216 -> 0.156 ms, page 0.271 ->
// 0.174 ms warm medians on the 248-row copy; row-ID/order diffed identical
// (123/123 full list, COUNT 123 = 123).
// The leading-wildcard LIKE itself still SCANs (no b-tree can serve
// '%x%'); fixing that needs FTS, which is explicitly out of scope.
// NOTE the orphan-Main gap is NOT closed here but by HasDataChildGuardSql
// below: the old INNER JOIN excluded a Main with zero Data children even
// when its mText matched, while a bare (desc OR dataExists) would include
// it. The guard restores exactly the old rowset (see ConvergeSearchFilter).
inline std::wstring FullTextExistsPredicate(const std::wstring& fullTextSql)
{
	if (fullTextSql.empty())
		return fullTextSql;
	return L"(EXISTS (SELECT 1 FROM Data WHERE Data.lParentID = Main.lID AND " + fullTextSql + L"))";
}

// Orphan-Main guard: reproduces the INNER JOIN's implicit "Main has >= 1
// Data row" side condition that FullTextExistsPredicate alone drops.
// Appended by ConvergeSearchFilter when requireDataRow is set (i.e. the
// FillList full-text branch ran, which is exactly when the shipped code put
// INNER JOIN Data in dataJoin). A second correlated EXISTS keeps the plan
// shape: SEARCH Data USING INDEX Data_ParentId_Format (lParentID=?), no
// TEMP B-TREE — verified against the copy, numbers in the task report.
inline std::wstring HasDataChildGuardSql()
{
	return L"(EXISTS (SELECT 1 FROM Data WHERE Data.lParentID = Main.lID))";
}

// Index note (why DatabaseUtilities.cpp is untouched): the T1 baseline
// proves the unfiltered COUNT/PAGE already SEEK via Main_InGroup2 and
// Main_TopLevelParentID, the group sort SEEKs via Main_InGroup2, the
// starred filter SEEKs via Main_IsGroup (lDontAutoDelete is a residual on
// a 0-row path — an index there has no measured proof), and the EXISTS
// inner probe already SEARCHes Data_ParentId_Format. No covering-index DDL
// is added because no measured plan shows an avoidable SCAN/sort that an
// index could serve: the remaining SCANs are leading-wildcard LIKEs.

// QPasteWnd.cpp:1689-1730. Converges the CFormatSQL fragments (each
// already wrapped in parens by CFormatSQL::GetSQLString) into one OR group,
// then ANDs the group-view parent filter and the starred filter back in.
// Empty fragments contribute nothing and add no " OR " separators.
// requireDataRow=false keeps the shipped text byte-identical; =true appends
// HasDataChildGuardSql() right after the OR group (before parent/starred),
// closing the orphan-Main gap described above. The flag defaults to false
// so all non-full-text callers are untouched.
inline std::wstring ConvergeSearchFilter(const std::wstring& descriptionSql,
	const std::wstring& quickPasteSql,
	const std::wstring& fullTextSql,
	const std::wstring& parentFilter,
	bool showStarred,
	bool requireDataRow = false)
{
	std::wstring filter = L"(";
	if (!descriptionSql.empty())
		filter += descriptionSql;
	if (!quickPasteSql.empty())
	{
		if (!descriptionSql.empty())
			filter += L" OR ";
		filter += quickPasteSql;
	}
	if (!fullTextSql.empty())
	{
		if (!descriptionSql.empty() || !quickPasteSql.empty())
			filter += L" OR ";
		filter += fullTextSql;
	}
	filter += L")";

	if (requireDataRow)
	{
		filter += L" AND ";
		filter += HasDataChildGuardSql();
	}

	if (!parentFilter.empty())
	{
		filter += L" AND ";
		filter += parentFilter;
	}

	if (showStarred)
	{
		filter += L" AND (";
		filter += StarredFilterSql();
		filter += L")";
	}

	return filter;
}

// QPasteWnd.cpp:1741-1751. The chip type filter applies in EVERY view on
// top of the group/starred/search filters built above.
inline std::wstring ApplyTypeFilter(const std::wstring& filter, int typeFilter)
{
	if (typeFilter == ChipAll)
		return filter;
	const std::wstring typeSql = TypeFilterSql(typeFilter);
	if (typeSql.empty())
		return filter;
	if (filter.empty())
		return typeSql;
	return filter + L" AND " + typeSql;
}

// QPasteWnd.cpp:1754. countSql.Format("SELECT COUNT(%s Main.lID) FROM
// Main %s where %s", ...). The concatenation keeps the format's exact
// spacing: empty IsDistinct leaves "COUNT( Main.lID)", empty dataJoin
// leaves "FROM Main  where" (two spaces).
inline std::wstring BuildCountSql(const std::wstring& distinct, const std::wstring& dataJoin, const std::wstring& filter)
{
	return L"SELECT COUNT(" + distinct + L" Main.lID) FROM Main " + dataJoin + L" where " + filter;
}

// QPasteWnd.cpp:1756-1759. The 13-column paged SELECT handed to the worker
// thread (which appends LIMIT/OFFSET in QPasteWndThread.cpp:126-128).
inline std::wstring BuildPageSql(const std::wstring& distinct,
	const std::wstring& dataJoin,
	const std::wstring& filter,
	const std::wstring& sort)
{
	return L"SELECT " + distinct +
		L" Main.lID, Main.mText, Main.lParentID, Main.lDontAutoDelete, "
		L"Main.lShortCut, Main.bIsGroup, Main.QuickPasteText, Main.clipOrder, Main.clipGroupOrder, "
		L"Main.stickyClipOrder, Main.stickyClipGroupOrder, Main.lDate, Main.lastPasteDate FROM Main " +
		dataJoin + L" where " + filter + L" order by " + sort;
}

// SendToFriend batch (CQPasteWnd::SendToFriendbyPos, src/QPasteWnd.cpp:2834).
// Replaces the N x (SELECT mText ... WHERE lID = ? + CClip::LoadFormats
// Data range per ID, src/Clip.cpp:1668-1672) loop with one Main IN-list
// query + one Data IN-list query; the caller iterates its ID list in order
// so send order is unchanged, and per-clip format order is kept by
// (lParentID, Data.lID desc), matching LoadFormats' lID-desc order within
// each clip. IDs are app-owned integers (selection item data), so inline
// decimal formatting is injection-free; empty input yields "" and the
// caller keeps its count <= 0 early-return (no "IN ()" is ever executed).
inline std::wstring JoinIdList(const std::vector<int>& ids)
{
	std::wstring out;
	for (size_t i = 0; i < ids.size(); i++)
	{
		if (i > 0)
			out += L",";
		out += std::to_wstring(ids[i]);
	}
	return out;
}

inline std::wstring BuildMainBatchSql(const std::vector<int>& ids)
{
	if (ids.empty())
		return L"";
	return L"SELECT lID, mText FROM Main WHERE lID IN (" + JoinIdList(ids) + L")";
}

inline std::wstring BuildDataBatchSql(const std::vector<int>& ids)
{
	if (ids.empty())
		return L"";
	return L"SELECT lID, lParentID, strClipBoardFormat, ooData FROM Data WHERE lParentID IN (" +
		JoinIdList(ids) + L") ORDER BY lParentID, Data.lID desc";
}

// GoToEntry rank (CQPasteWnd::OnMenuGoToEntry, src/QPasteWnd.cpp:7160).
// RANK() (not ROW_NUMBER()) so tied sort keys share one index, exactly
// like the old COUNT(*)-of-strictly-before fallback query: RANK - 1 equals
// that COUNT for every row, ties included. Ties are real -- the sort key
// (stickyClipOrder, bIsGroup, clipOrder) can repeat (e.g. migrated clips
// sharing clipOrder) -- so ROW_NUMBER()'s distinct positions would scroll
// to the wrong row after a tie. Verified by a full old-vs-new sweep on
// the baseline DB copy, including a synthetic full-key tie pair.
// The ORDER BY reuses MainSortSql() so the window key can never drift from
// the list sort. Returns no row when the target is excluded by the filter;
// the caller then falls back to the COUNT shape, preserving the old scroll
// position in that case too.
inline std::wstring BuildGoToRankSql(const std::wstring& filter, int targetId)
{
	return L"SELECT rank FROM (SELECT Main.lID, RANK() OVER (ORDER BY " +
		MainSortSql() + L") - 1 AS rank FROM Main where " + filter +
		L") WHERE lID = " + std::to_wstring(targetId);
}

} // namespace ListQuery
