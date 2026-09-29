#include "stdafx.h"
#include "ClipboardMonitorImpl.h"
#include "..\Shared\IClipboardConfig.h"
#include "CP_Main.h"
#include "MainFrm.h"
#include "Misc.h"
#include "Options.h"

CClipboardMonitorImpl::CClipboardMonitorImpl()
    : m_hClipHandler(nullptr)
    , m_pConfig(nullptr)
    , m_created(false)
{
}

CClipboardMonitorImpl::~CClipboardMonitorImpl()
{
    if (m_created)
        Destroy();
}

bool CClipboardMonitorImpl::Create(HWND hClipHandler, IClipboardConfig* pConfig)
{
    ASSERT(hClipHandler != nullptr);
    ASSERT(pConfig != nullptr);
    ASSERT(!m_created);
    if (m_created) return false;

    m_hClipHandler = hClipHandler;
    m_pConfig = pConfig;

    auto supportedTypes = pConfig->GetSupportedTypes();
    auto pTypes = new CClipTypes();
    if (supportedTypes)
    {
        for (auto cf : *supportedTypes)
            pTypes->Add(cf);
    }
    else
    {
        pTypes->Add(CF_UNICODETEXT);
    }

    m_copyThread.Init(CCopyConfig(m_hClipHandler, true, true, pTypes));

    VERIFY(m_copyThread.CreateThread(CREATE_SUSPENDED));
    m_copyThread.ResumeThread();

    m_created = true;
    return true;
}

void CClipboardMonitorImpl::Destroy()
{
    if (!m_created) return;
    m_copyThread.SetCopyOnChange(false);
    m_copyThread.Quit();
    m_hClipHandler = nullptr;
    m_pConfig = nullptr;
    m_created = false;
}

EventToken CClipboardMonitorImpl::Subscribe(IClipEventListener* listener)
{
    return m_dispatcher.Subscribe(listener);
}

void CClipboardMonitorImpl::Unsubscribe(EventToken token)
{
    m_dispatcher.Unsubscribe(token);
}

void CClipboardMonitorImpl::NotifyClipCaptured(ClipId clipId, const CString& description, int formatCount)
{
    if (!m_created) return;
    ClipEventInfo info;
    info.type = ClipEventType::Captured;
    info.clipId = clipId;
    info.description = description;
    info.formatCount = formatCount;
    m_dispatcher.FireEvent(info);
}

std::vector<ClipEntry> CClipboardMonitorImpl::QueryClips(const ClipQueryFilter& filter)
{
    std::vector<ClipEntry> results;
    try
    {
        CString sql, orderField, orderDir = _T("DESC");
        switch (filter.sortBy)
        {
        case ClipQueryFilter::SortBy::CopyTimeDesc:
            orderField = _T("Main.lDate"); orderDir = _T("DESC"); break;
        case ClipQueryFilter::SortBy::CopyTimeAsc:
            orderField = _T("Main.lDate"); orderDir = _T("ASC"); break;
        case ClipQueryFilter::SortBy::PasteCountDesc:
            orderField = _T("Main.paste_count"); orderDir = _T("DESC"); break;
        case ClipQueryFilter::SortBy::PasteCountAsc:
            orderField = _T("Main.paste_count"); orderDir = _T("ASC"); break;
        case ClipQueryFilter::SortBy::LastPasteTimeDesc:
            orderField = _T("Main.lastPasteDate"); orderDir = _T("DESC"); break;
        case ClipQueryFilter::SortBy::LastPasteTimeAsc:
            orderField = _T("Main.lastPasteDate"); orderDir = _T("ASC"); break;
        default:
            orderField = _T("Main.lDate"); orderDir = _T("DESC"); break;
        }

        CString where = _T("Main.bIsGroup = 0");
        if (filter.groupId.has_value())
        {
            CString g; g.Format(_T(" AND Main.lParentID = %ld"), filter.groupId.value()); where += g;
        }
        if (filter.minPasteCount.has_value())
        {
            CString m; m.Format(_T(" AND Main.paste_count >= %d"), filter.minPasteCount.value()); where += m;
        }
        if (filter.textSearch.has_value())
        {
            CString escaped = filter.textSearch.value();
            escaped.Replace(_T("\\"), _T("\\\\"));  // 先转义反斜杠
            escaped.Replace(_T("'"), _T("''"));
            escaped.Replace(_T("%"), _T("\\%"));
            escaped.Replace(_T("_"), _T("\\_"));
            CString t;
            t.Format(_T(" AND Main.mText LIKE '%%%s%%' ESCAPE '\\'"), escaped);
            where += t;
        }

        sql.Format(_T("SELECT Main.lID, Main.mText, Main.lParentID, Main.paste_count, ")
            _T("Main.lDate, Main.lastPasteDate, Main.lDontAutoDelete, Main.lShortCut ")
            _T("FROM Main WHERE %s ORDER BY %s %s LIMIT %zu OFFSET %zu"),
            where, orderField, orderDir, filter.limit, filter.offset);

        CppSQLite3Query q = theApp.m_db.execQuery(sql);
        while (!q.eof())
        {
            ClipEntry entry;
            entry.id = q.getIntField(_T("lID"));
            entry.description = q.fieldValue(_T("mText"));
            entry.groupId = q.getIntField(_T("lParentID"));
            entry.pasteCount = q.getIntField(_T("paste_count"));
            entry.copyTime = CTime(q.getInt64Field(_T("lDate")));
            entry.lastPasteTime = CTime(q.getInt64Field(_T("lastPasteDate")));
            entry.isSticky = q.getIntField(_T("lDontAutoDelete")) > 0;
            entry.formatCount = 0;
            results.push_back(entry);
            q.nextRow();
        }
    }
    CATCH_SQLITE_EXCEPTION
    return results;
}

std::vector<ClipEntry> CClipboardMonitorImpl::GetTopPasted(size_t count)
{
    ClipQueryFilter filter;
    filter.sortBy = ClipQueryFilter::SortBy::PasteCountDesc;
    filter.limit = count;
    return QueryClips(filter);
}

size_t CClipboardMonitorImpl::CountClips(const ClipQueryFilter& filter)
{
    size_t result = 0;
    try
    {
        CString where = _T("Main.bIsGroup = 0");
        if (filter.groupId.has_value())
        {
            CString g; g.Format(_T(" AND Main.lParentID = %ld"), filter.groupId.value()); where += g;
        }
        if (filter.minPasteCount.has_value())
        {
            CString m; m.Format(_T(" AND Main.paste_count >= %d"), filter.minPasteCount.value()); where += m;
        }
        if (filter.textSearch.has_value())
        {
            CString escaped = filter.textSearch.value();
            escaped.Replace(_T("'"), _T("''"));
            CString t; t.Format(_T(" AND Main.mText LIKE '%%%s%%'"), escaped); where += t;
        }
        CString sql; sql.Format(_T("SELECT COUNT(Main.lID) FROM Main WHERE %s"), where);
        result = (size_t)theApp.m_db.execScalar(sql);
    }
    CATCH_SQLITE_EXCEPTION
    return result;
}

void CClipboardMonitorImpl::NotifyPaste(ClipId id)
{
    try
    {
        __time64_t now = CTime::GetCurrentTime().GetTime();
        theApp.m_db.execDMLEx(_T("UPDATE Main SET paste_count = paste_count + 1, ")
            _T("lastPasteDate = %lld WHERE lID = %ld"),
            (long long)now, id);

        ClipEventInfo info;
        info.type = ClipEventType::Saved;
        info.clipId = id;
        m_dispatcher.FireEvent(info);
    }
    CATCH_SQLITE_EXCEPTION
}

void CClipboardMonitorImpl::NotifyPasteBatch(const std::vector<ClipId>& ids)
{
    bool inTransaction = false;
    try
    {
        theApp.m_db.execDML(_T("BEGIN TRANSACTION"));
        inTransaction = true;
        __time64_t now = CTime::GetCurrentTime().GetTime();
        for (auto id : ids)
        {
            theApp.m_db.execDMLEx(_T("UPDATE Main SET paste_count = paste_count + 1, ")
                _T("lastPasteDate = %lld WHERE lID = %ld"), (long long)now, id);
        }
        theApp.m_db.execDML(_T("COMMIT"));
        inTransaction = false;

        // 通知事件监听器
        ClipEventInfo info;
        info.type = ClipEventType::Saved;
        for (auto id : ids)
        {
            info.clipId = id;
            m_dispatcher.FireEvent(info);
        }
    }
    catch (CppSQLite3Exception& e)
    {
        if (inTransaction)
        {
            try { theApp.m_db.execDML(_T("ROLLBACK")); }
            catch (...) { }
        }
        Log(StrF(_T("SQLITE Exception %d - %s"), e.errorCode(), e.errorMessage()));
        ASSERT(FALSE);
    }
}

bool CClipboardMonitorImpl::IsClipboardViewerConnected()
{
    return m_copyThread.IsClipboardViewerConnected();
}

bool CClipboardMonitorImpl::GetConnectCV()
{
    return m_copyThread.GetConnectCV();
}

void CClipboardMonitorImpl::SetConnectCV(bool bConnect)
{
    m_copyThread.SetConnectCV(bConnect);
}
