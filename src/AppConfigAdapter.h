#pragma once

#include "..\Shared\IClipboardConfig.h"

class CAppConfigAdapter : public IClipboardConfig
{
public:
    CAppConfigAdapter();
    virtual ~CAppConfigAdapter() = default;

    bool GetCopyOnChange() const override;
    bool IsAppIncluded(HWND hWnd) const override;
    bool IsAppExcluded(HWND hWnd) const override;
    std::shared_ptr<std::vector<CLIPFORMAT>> GetSupportedTypes() const override;
    int GetDebounceIntervalMs() const override;

    void SetCopyOnChange(bool bEnabled);

private:
    bool m_bCopyOnChange;
};
