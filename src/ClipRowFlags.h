#pragma once

// Per-row flag/symbol parsing extracted from CQListCtrl::OnCustomdrawList
// (src/QListCtrl.cpp) so it can be unit-tested outside the MFC paint cycle.
// Same seam pattern as src/ThemeColorMath.h: header-only, pure C++17, no
// MFC/CString/windows.h -- only <string>/<vector>, so tests/ can compile it
// with plain cl/g++.
//
// Logic sources (mirrored exactly, call sites unchanged):
//   SplitClipRowText  -- QListCtrl.cpp:505-512, first-'|' split of the row
//                        text into the symbol prefix and the display text.
//   ParseClipRowFlags -- QListCtrl.cpp:576-585 (<pasted> bar), 601-608
//                        (drawInGroupIcon vs theApp.m_GroupID) and 617-647
//                        (per-flag icon drawing). All flag matches use
//                        CString::Find semantics, i.e. case-SENSITIVE
//                        substring search. NOTE: the paint also gates the
//                        pasted bar on m_showIfClipWasPasted -- that gating
//                        stays at the call site; wasPasted is the raw flag.
//                        <qpastetext> sets a flag although the paint draws
//                        nothing for it (empty branch at :640-642).
//   ComputeHighlightSpans / ClipRowFindNoCaseAndInsert -- Misc.cpp:1209-1308
//                        (FindNoCaseAndInsert) as used at QListCtrl.cpp:652-657:
//                        case-INsensitive search (Misc lowers both sides via
//                        ICU ToLowerStringEx + MakeLower; here mirrored with
//                        per-character towlower/tolower, a 1:1 mapping, so
//                        lowered offsets equal original offsets), non-
//                        overlapping matches, 101-replacement cap (Misc
//                        increments replaceCount then breaks on > 100, so it
//                        performs 101 insertions), the
//                        first-hit line-window truncation, and the trailing
//                        \r\n/\r/\n -> \x01\x05\x02 rewrite.
// NOTE (ICU divergence boundary): production lowering is ICU string-level
// (Misc.cpp:1225-1227 ToLowerStringEx + MakeLower); this seam lowers per
// code unit with towlower/tolower (C locale: ASCII-only in practice). Inputs
// that can differ: non-ASCII case mappings (e.g. Greek final sigma U+03C2,
// Turkish dotted/dotless I U+0130/U+0131 and locale-sensitive mappings),
// 1-to-N case mappings (e.g. German sharp-s U+00DF -> "ss" style expansions),
// and any mapping where ICU changes string length so lowered offsets no
// longer equal original offsets. Full ICU parity would need an ICU
// dependency in the test seam -- out of scope. Defined seam behavior:
// ASCII case-insensitivity only; non-ASCII code units pass through the
// C-locale mapping unchanged.

#include <cctype>
#include <cwctype>
#include <string>
#include <utility>
#include <vector>

// ---------------------------------------------------------------------------
// small helpers (case conversion, CString::Replace equivalent)
// ---------------------------------------------------------------------------

inline char ClipRowToLowerChar(char ch)
{
	return static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
}

inline wchar_t ClipRowToLowerChar(wchar_t ch)
{
	return static_cast<wchar_t>(std::towlower(static_cast<wint_t>(ch)));
}

template <typename CharT>
inline std::basic_string<CharT> ClipRowToLower(const std::basic_string<CharT>& s)
{
	std::basic_string<CharT> out(s);
	for (CharT& ch : out)
		ch = ClipRowToLowerChar(ch);
	return out;
}

// CString::Replace(old, nu) equivalent: replaces all non-overlapping
// occurrences left to right, returns the replacement count.
template <typename CharT>
inline int ClipRowReplaceAll(std::basic_string<CharT>& str,
	const std::basic_string<CharT>& oldToken,
	const std::basic_string<CharT>& newToken)
{
	if (oldToken.empty())
		return 0;
	int count = 0;
	typename std::basic_string<CharT>::size_type pos = 0;
	while ((pos = str.find(oldToken, pos)) != std::basic_string<CharT>::npos)
	{
		str.replace(pos, oldToken.size(), newToken);
		pos += newToken.size();
		count++;
	}
	return count;
}

// ---------------------------------------------------------------------------
// 1. symbol pipe-split (QListCtrl.cpp:505-512)
// ---------------------------------------------------------------------------

template <typename CharT>
struct ClipRowSplitResult
{
	std::basic_string<CharT> symbols; // Left(nSymEnd), empty when no '|'
	std::basic_string<CharT> text;    // Mid(nSymEnd + 1), whole row when no '|'
};

// Splits at the FIRST '|' exactly like:
//   nSymEnd = csText.Find('|'); if (nSymEnd >= 0) { symbols = Left; text = Mid; }
template <typename CharT>
inline ClipRowSplitResult<CharT> SplitClipRowText(const std::basic_string<CharT>& row)
{
	ClipRowSplitResult<CharT> result;
	typename std::basic_string<CharT>::size_type pipe = row.find(CharT('|'));
	if (pipe != std::basic_string<CharT>::npos)
	{
		result.symbols = row.substr(0, pipe);
		result.text = row.substr(pipe + 1);
	}
	else
	{
		result.symbols.clear();
		result.text = row;
	}
	return result;
}

// ---------------------------------------------------------------------------
// 2. group/sticky/ingroup flag parsing (QListCtrl.cpp:576-647)
// ---------------------------------------------------------------------------

struct ClipRowFlags
{
	bool wasPasted = false;        // "<pasted>"      (:576-585 bar)
	bool isGroup = false;          // "<group>"       (:619)
	bool neverAutoDelete = false;  // "<noautodelete>"(:624)
	bool hasShortcut = false;      // "<shortcut>"    (:629)
	bool inGroup = false;          // "<ingroup>" raw flag (:634-635)
	bool drawInGroupIcon = true;   // false when currentGroupId > 0 and inGroup (:601-608)
	bool inGroupIconVisible = false; // drawInGroupIcon && inGroup -- the icon actually drawn
	bool hasQuickPasteText = false;// "<qpastetext>"  (:640, paint draws nothing)
	bool isSticky = false;         // "<sticky>"      (:643)
};

// currentGroupId mirrors theApp.m_GroupID at :603.
// NOTE: token constants are function-local static const per CharT
// instantiation (C++11 magic statics: constructed once, thread-safe), so the
// 7 find calls below allocate nothing per row. The single wsSymbols copy at
// the QListCtrl.cpp call site stays as-is: one small copy/row is negligible
// next to the GDI icon/text costs of the paint path (no view re-architecture).
template <typename CharT>
inline ClipRowFlags ParseClipRowFlags(const std::basic_string<CharT>& symbols, long currentGroupId)
{
	typedef std::basic_string<CharT> Str;
	static const Str kPasted = Str{ static_cast<CharT>('<'), static_cast<CharT>('p'), static_cast<CharT>('a'), static_cast<CharT>('s'), static_cast<CharT>('t'), static_cast<CharT>('e'), static_cast<CharT>('d'), static_cast<CharT>('>') };
	static const Str kGroup = Str{ static_cast<CharT>('<'), static_cast<CharT>('g'), static_cast<CharT>('r'), static_cast<CharT>('o'), static_cast<CharT>('u'), static_cast<CharT>('p'), static_cast<CharT>('>') };
	static const Str kNoAutoDelete = Str{ static_cast<CharT>('<'), static_cast<CharT>('n'), static_cast<CharT>('o'), static_cast<CharT>('a'), static_cast<CharT>('u'), static_cast<CharT>('t'), static_cast<CharT>('o'), static_cast<CharT>('d'), static_cast<CharT>('e'), static_cast<CharT>('l'), static_cast<CharT>('e'), static_cast<CharT>('t'), static_cast<CharT>('e'), static_cast<CharT>('>') };
	static const Str kShortcut = Str{ static_cast<CharT>('<'), static_cast<CharT>('s'), static_cast<CharT>('h'), static_cast<CharT>('o'), static_cast<CharT>('r'), static_cast<CharT>('t'), static_cast<CharT>('c'), static_cast<CharT>('u'), static_cast<CharT>('t'), static_cast<CharT>('>') };
	static const Str kInGroup = Str{ static_cast<CharT>('<'), static_cast<CharT>('i'), static_cast<CharT>('n'), static_cast<CharT>('g'), static_cast<CharT>('r'), static_cast<CharT>('o'), static_cast<CharT>('u'), static_cast<CharT>('p'), static_cast<CharT>('>') };
	static const Str kQuickPasteText = Str{ static_cast<CharT>('<'), static_cast<CharT>('q'), static_cast<CharT>('p'), static_cast<CharT>('a'), static_cast<CharT>('s'), static_cast<CharT>('t'), static_cast<CharT>('e'), static_cast<CharT>('t'), static_cast<CharT>('e'), static_cast<CharT>('x'), static_cast<CharT>('t'), static_cast<CharT>('>') };
	static const Str kSticky = Str{ static_cast<CharT>('<'), static_cast<CharT>('s'), static_cast<CharT>('t'), static_cast<CharT>('i'), static_cast<CharT>('c'), static_cast<CharT>('k'), static_cast<CharT>('y'), static_cast<CharT>('>') };
	auto contains = [&symbols](const Str& token) -> bool
	{
		return symbols.find(token) != Str::npos;
	};

	ClipRowFlags flags;
	flags.wasPasted = contains(kPasted);
	flags.isGroup = contains(kGroup);
	flags.neverAutoDelete = contains(kNoAutoDelete);
	flags.hasShortcut = contains(kShortcut);
	flags.inGroup = contains(kInGroup);
	flags.hasQuickPasteText = contains(kQuickPasteText);
	flags.isSticky = contains(kSticky);

	// :603-608 -- inside a group the "in group" flag icon is suppressed.
	flags.drawInGroupIcon = !(currentGroupId > 0 && flags.inGroup);
	flags.inGroupIconVisible = flags.drawInGroupIcon && flags.inGroup;
	return flags;
}

// ---------------------------------------------------------------------------
// 3. search-highlight spans + FindNoCaseAndInsert port (Misc.cpp:1209-1308,
//    used at QListCtrl.cpp:652-657)
// ---------------------------------------------------------------------------

template <typename CharT>
struct ClipRowHighlightSpan
{
	typename std::basic_string<CharT>::size_type start = 0; // offset in original text
	typename std::basic_string<CharT>::size_type length = 0;
};

// Case-insensitive, non-overlapping match offsets in original-text
// coordinates -- the search half of FindNoCaseAndInsert's first loop
// (mainLow.Find(findLow, startFindPos), startFindPos += oldLen, 101-hit
// parity: Misc increments replaceCount then breaks on > 100, so production
// performs 101 insertions; default maxHits is 101 to match).
template <typename CharT>
inline std::vector<ClipRowHighlightSpan<CharT>> ComputeHighlightSpans(
	const std::basic_string<CharT>& text,
	const std::basic_string<CharT>& search,
	int maxHits = 101)
{
	typedef std::basic_string<CharT> Str;
	std::vector<ClipRowHighlightSpan<CharT>> spans;
	if (search.empty())
		return spans;

	Str mainLow = ClipRowToLower(text);
	Str findLow = ClipRowToLower(search);
	typename Str::size_type startFindPos = 0;
	while (static_cast<int>(spans.size()) < maxHits)
	{
		typename Str::size_type foundPos = mainLow.find(findLow, startFindPos);
		if (foundPos == Str::npos)
			break;
		ClipRowHighlightSpan<CharT> span;
		span.start = foundPos;
		span.length = search.size();
		spans.push_back(span);
		startFindPos = foundPos + findLow.size();
	}
	return spans;
}

// Exact port of Misc.cpp FindNoCaseAndInsert operating on std::string/
// std::wstring instead of CString: inserts pre/post markers around every
// case-insensitive non-overlapping hit (101-hit parity: breaks once
// replaceCount > 100, i.e. 101 insertions), then
// applies the first-hit line-window truncation (linesPerRow) and finally
// rewrites \r\n/\r/\n to \x01\x05\x02 when at least one hit occurred.
// Returns the hit count; empty search leaves the string untouched and
// returns 0.
template <typename CharT>
inline int ClipRowFindNoCaseAndInsert(std::basic_string<CharT>& mainStr,
	const std::basic_string<CharT>& findStr,
	const std::basic_string<CharT>& preInsert,
	const std::basic_string<CharT>& postInsert,
	int linesPerRow)
{
	typedef std::basic_string<CharT> Str;
	typedef typename Str::size_type Sz;
	static const Sz NPOS = Str::npos;

	int replaceCount = 0;

	// Prevent infinite loop when user tries to replace nothing.
	if (!findStr.empty())
	{
		Sz oldLen = findStr.size();

		Sz startFindPos = 0;
		Sz newPos = 0;
		Sz insertedLength = 0;

		Sz firstFindPos = 0;

		Str mainLow = ClipRowToLower(mainStr);
		Str findLow = ClipRowToLower(findStr);

		Sz preLength = preInsert.size();
		Sz postLength = postInsert.size();

		while (true)
		{
			Sz foundPos = mainLow.find(findLow, startFindPos);
			if (foundPos == NPOS)
				break;

			if (replaceCount == 0)
			{
				firstFindPos = foundPos + preLength;
			}

			newPos = foundPos + insertedLength;

			mainStr.insert(newPos, preInsert);
			mainStr.insert(newPos + preLength + oldLen, postInsert);

			startFindPos = foundPos + oldLen;

			insertedLength += preLength + postLength;

			replaceCount++;

			// safety check, make sure we don't look forever
			if (replaceCount > 100)
				break;
		}

		startFindPos = 0;
		int line = 0;
		Sz prevLinePos = 0;
		Sz prevPrevLinePos = 0;

		const Str newline(1, CharT('\n'));
		while (true)
		{
			Sz foundPos = mainStr.find(newline, startFindPos);
			if (foundPos == NPOS)
				break;

			if (firstFindPos < foundPos)
			{
				if (line > linesPerRow - 1)
				{
					Sz lineStart = prevLinePos;
					if (linesPerRow > 1)
					{
						lineStart = prevPrevLinePos;
					}

					Str ellipsis;
					ellipsis.push_back(CharT('.'));
					ellipsis.push_back(CharT('.'));
					ellipsis.push_back(CharT('.'));
					ellipsis.push_back(CharT(' '));
					mainStr = ellipsis + mainStr.substr(lineStart + 1);
				}

				break;
			}

			startFindPos = foundPos + 1;
			prevPrevLinePos = prevLinePos;
			prevLinePos = foundPos;

			line++;

			// safety check, make sure we don't look forever
			if (line > 1000)
				break;
		}

		if (replaceCount > 0)
		{
			// use unprintable characters so it doesn't find copied html to convert
			Str marker;
			marker.push_back(CharT('\x01'));
			marker.push_back(CharT('\x05'));
			marker.push_back(CharT('\x02'));
			Str crlf;
			crlf.push_back(CharT('\r'));
			crlf.push_back(CharT('\n'));
			Str cr(1, CharT('\r'));
			Str lf(1, CharT('\n'));
			ClipRowReplaceAll(mainStr, crlf, marker);
			ClipRowReplaceAll(mainStr, cr, marker);
			ClipRowReplaceAll(mainStr, lf, marker);
		}
	}

	return replaceCount;
}
