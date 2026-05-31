#include "stdafx.h"
#include "AppConfigAdapter.h"
#include "CP_Main.h"
#include "Options.h"
#include "Misc.h"
#include "..\Shared\Tokenizer.h"
#include "WildCardMatch.h"
#include "sqlite\CppSQLite3.h"
#include "..\Shared\DittoDefines.h"

CAppConfigAdapter::CAppConfigAdapter()
    : m_bCopyOnChange(true)
{
}

bool CAppConfigAdapter::GetCopyOnChange() const
{
    return m_bCopyOnChange;
}

bool CAppConfigAdapter::IsAppIncluded(HWND hWnd) const
{
    CString includeApps = CGetSetOptions::GetCopyAppInclude();
    if (includeApps.IsEmpty())
        return true; // 空列表 = 全包含

    CString appName = GetProcessName(hWnd).MakeLower();
    if (appName.IsEmpty())
        return true; // 无法获取进程名时保守放行

    CTokenizer token(includeApps, CGetSetOptions::GetCopyAppSeparator());
    CString line;
    while (token.Next(line))
    {
        if (!line.IsEmpty() && CWildCardMatch::WildMatch(line.Trim(), appName, _T("")))
            return true;
    }
    return false;
}

bool CAppConfigAdapter::IsAppExcluded(HWND hWnd) const
{
    CString excludeApps = CGetSetOptions::GetCopyAppExclude();
    if (excludeApps.IsEmpty())
        return false; // 空列表 = 无排除

    CString appName = GetProcessName(hWnd).MakeLower();
    if (appName.IsEmpty())
        return false; // 无法获取进程名时保守放行

    CTokenizer token(excludeApps, CGetSetOptions::GetCopyAppSeparator());
    CString line;
    while (token.Next(line))
    {
        if (!line.IsEmpty() && CWildCardMatch::WildMatch(line.Trim(), appName, _T("")))
            return true;
    }
    return false;
}

std::shared_ptr<std::vector<CLIPFORMAT>> CAppConfigAdapter::GetSupportedTypes() const
{
    auto pTypes = std::make_shared<std::vector<CLIPFORMAT>>();
    // 从 DB 加载已配置的类型（与原始 LoadTypesFromDB 逻辑一致）
    try
    {
        CppSQLite3Query q = theApp.m_db.execQuery(_T("SELECT TypeText FROM Types ORDER BY lID ASC"));
        while (!q.eof())
        {
            CLIPFORMAT cf = CDittoAddinHelpers::GetFormatID(q.fieldValue(_T("TypeText")));
            if (cf != 0)
                pTypes->push_back(cf);
            q.nextRow();
        }
    }
    catch (CppSQLite3Exception&)
    {
        // DB 不可用时使用默认类型
        pTypes->push_back(CF_UNICODETEXT);
        pTypes->push_back(CF_TEXT);
        pTypes->push_back(CF_HDROP);
        pTypes->push_back(CF_DIB);
    }
    return pTypes;
}

int CAppConfigAdapter::GetDebounceIntervalMs() const
{
    return CGetSetOptions::m_lProcessDrawClipboardDelay;
}

void CAppConfigAdapter::SetCopyOnChange(bool bEnabled)
{
    m_bCopyOnChange = bEnabled;
}
