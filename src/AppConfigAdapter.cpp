#include "stdafx.h"
#include "AppConfigAdapter.h"
#include "Options.h"

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
    return includeApps.IsEmpty();
}

bool CAppConfigAdapter::IsAppExcluded(HWND hWnd) const
{
    CString excludeApps = CGetSetOptions::GetCopyAppExclude();
    return !excludeApps.IsEmpty();
}

std::shared_ptr<std::vector<CLIPFORMAT>> CAppConfigAdapter::GetSupportedTypes() const
{
    auto pTypes = std::make_shared<std::vector<CLIPFORMAT>>();
    pTypes->push_back(CF_UNICODETEXT);
    pTypes->push_back(CF_TEXT);
    pTypes->push_back(CF_HDROP);
    pTypes->push_back(CF_DIB);
    pTypes->push_back(CF_ENHMETAFILE);
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
