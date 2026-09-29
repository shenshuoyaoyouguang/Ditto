// MainTableFunctions.cpp: implementation of the CMainTableFunctions class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cp_main.h"
#include "MainTableFunctions.h"
#include "..\Shared\Tokenizer.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMainTableFunctions::CMainTableFunctions()
{

}

CMainTableFunctions::~CMainTableFunctions()
{

}

void CMainTableFunctions::LoadAcceleratorKeys(CAccels& accels, CppSQLite3DB &db)
{
	try
	{
		{
			CppSQLite3Query q = db.execQuery(_T("SELECT lID, lShortCut FROM Main WHERE lShortCut > 0"));
		
			CAccel a;
			while(q.eof() == false)
			{
				a.Cmd = q.getIntField(_T("lID"));
				a.Key = q.getIntField(_T("lShortCut"));
				a.RefId = CHotKey::PASTE_OPEN_CLIP;
			
				accels.AddAccel(a);

				q.nextRow();
			}
		}

		{
			CppSQLite3Query q2 = db.execQuery(_T("SELECT lID, MoveToGroupShortCut FROM Main WHERE MoveToGroupShortCut > 0"));

			CAccel a2;
			while(q2.eof() == false)
			{
				a2.Cmd = q2.getIntField(_T("lID"));
				a2.Key = q2.getIntField(_T("MoveToGroupShortCut"));
				a2.RefId = CHotKey::MOVE_TO_GROUP;

				accels.AddAccel(a2);

				q2.nextRow();
			}
		}
	}
	CATCH_SQLITE_EXCEPTION
}

CString CMainTableFunctions::GetRelativeTime(__int64 nTime, __int64 nNow)
{
	if (nTime <= 0)
	{
		return _T("");
	}

	if (nNow <= 0)
	{
		nNow = (__int64)CTime::GetCurrentTime().GetTime();
	}

	__int64 nDiff = nNow - nTime;
	if (nDiff < 0)
	{
		nDiff = 0;
	}

	__int64 nMinutes = nDiff / 60;
	__int64 nHours = nDiff / 3600;
	__int64 nDays = nDiff / 86400;

	if (nMinutes < 1)
	{
		return theApp.m_Language.GetString(_T("RelativeNow"), _T("just now"));
	}

	if (nHours < 1)
	{
		return StrF(_T("%d %s"), (int)nMinutes, theApp.m_Language.GetString(_T("RelativeMinutesAgo"), _T("min ago")));
	}

	if (nDays < 1)
	{
		return StrF(_T("%d %s"), (int)nHours, theApp.m_Language.GetString(_T("RelativeHoursAgo"), _T("hr ago")));
	}

	if (nDays < 7)
	{
		CString suffix = nDays == 1
			? theApp.m_Language.GetString(_T("RelativeDayAgo"), _T("day ago"))
			: theApp.m_Language.GetString(_T("RelativeDaysAgo"), _T("days ago"));
		return StrF(_T("%d %s"), (int)nDays, suffix);
	}

	CTime time((time_t)nTime);
	return time.Format(_T("%Y-%m-%d"));
}

CString CMainTableFunctions::GetDisplayText(int nMaxLines, const CString &OrigText)
{
	CString text = OrigText;
	// assign tabs to 2 spaces (rather than the default 8)
	text.Replace(_T("\t"), _T("  "));

	if(CGetSetOptions::m_bDescShowLeadingWhiteSpace)
		return text;
	// else, remove the leading indent from every line.

	// get the lines
	CString token;
	CStringArray tokens;
	CTokenizer tokenizer(text, "\r\n");
	for(int nLines=0; nLines < 100 && tokenizer.Next(token); nLines++)
	{
		tokens.Add(token);
	}

	// remove each line's indent
	TCHAR chFirst;
	CString line;
	INT_PTR count = tokens.GetSize();
	text = _T("");
	for(int i=0; i < count; i++)
	{
		line = tokens.ElementAt(i);
		chFirst = line.GetAt(0);
		if(chFirst == ' ' || chFirst == '\t')
		{
			text += _T("» "); // show indication that the line is modified
			line.TrimLeft();
			text += line;
		}
		else
		{
			text += line;
		}

		if (i != count - 1)
		{
			text += _T("\n");
		}
	}

	return text;
}
