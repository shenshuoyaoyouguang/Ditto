#pragma once

#include <cstdint>
#include <vector>
#include <optional>
#include <afx.h>

// ── 类型别名 ──
using ClipId = long;

// ── 剪贴条目摘要（供查询返回） ──
struct ClipEntry
{
    ClipId      id;
    CString     description;
    CTime       copyTime;
    CTime       lastPasteTime;
    int         pasteCount;         // 粘贴使用次数，0 表示从未被粘贴
    long        groupId;
    int         formatCount;
    bool        isSticky;

    ClipEntry()
        : id(0), pasteCount(0), groupId(0), formatCount(0), isSticky(false)
    {}
};

// ── 查询过滤器 ──
struct ClipQueryFilter
{
    enum class SortBy
    {
        CopyTimeDesc,       // 默认：按复制时间降序
        CopyTimeAsc,        // 按复制时间升序
        PasteCountDesc,     // 按粘贴次数降序
        PasteCountAsc,      // 按粘贴次数升序
        LastPasteTimeDesc,  // 按最近粘贴时间降序
        LastPasteTimeAsc,   // 按最近粘贴时间升序
    };

    std::optional<CString>  textSearch;
    std::optional<long>     groupId;
    std::optional<int>      minPasteCount;
    std::optional<CTime>    fromDate;
    std::optional<CTime>    toDate;

    SortBy  sortBy   = SortBy::CopyTimeDesc;
    size_t  offset   = 0;
    size_t  limit    = 100;

    ClipQueryFilter() = default;
};

// ── 事件类型（三阶段粒度） ──
enum class ClipEventType : uint8_t
{
    Captured,       // 剪贴板变化，数据已提取但未落库
    Saved,          // 数据已持久化到 SQLite
    SavedFailed,    // 持久化失败
};

// ── 事件荷载 ──
struct ClipEventInfo
{
    ClipEventType   type;
    ClipId          clipId;
    CString         description;
    int             formatCount;
    CString         errorMessage;   // SavedFailed 时填写

    ClipEventInfo()
        : type(ClipEventType::Saved), clipId(0), formatCount(0)
    {}
};

// ── 事件订阅 token ──
using EventToken = uint64_t;
constexpr EventToken InvalidEventToken = 0;
