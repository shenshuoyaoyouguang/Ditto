#pragma once

#include <memory>
#include <vector>
#include <afx.h>

// ── 配置 seam ──
class IClipboardConfig
{
public:
    virtual ~IClipboardConfig() = default;
    virtual bool GetCopyOnChange() const = 0;
    virtual bool IsAppIncluded(HWND hWnd) const = 0;
    virtual bool IsAppExcluded(HWND hWnd) const = 0;
    virtual std::shared_ptr<std::vector<CLIPFORMAT>> GetSupportedTypes() const = 0;
    virtual int GetDebounceIntervalMs() const = 0;
};
