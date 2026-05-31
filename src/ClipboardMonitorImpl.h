#pragma once

#include "..\Shared\IClipboardMonitor.h"
#include "ClipboardEventDispatcher.h"
#include "CopyThread.h"

class CClipboardMonitorImpl : public IClipboardMonitor
{
public:
    CClipboardMonitorImpl();
    virtual ~CClipboardMonitorImpl();

    // IClipboardMonitor
    bool Create(HWND hClipHandler, IClipboardConfig* pConfig) override;
    void Destroy() override;

    EventToken Subscribe(IClipEventListener* listener) override;
    void Unsubscribe(EventToken token) override;

    std::vector<ClipEntry> QueryClips(const ClipQueryFilter& filter) override;
    std::vector<ClipEntry> GetTopPasted(size_t count) override;
    size_t CountClips(const ClipQueryFilter& filter) override;

    void NotifyPaste(ClipId id) override;
    void NotifyPasteBatch(const std::vector<ClipId>& ids) override;

    bool IsClipboardViewerConnected() override;
    bool GetConnectCV() override;
    void SetConnectCV(bool bConnect) override;

    void NotifyClipCaptured(ClipId clipId, const CString& description, int formatCount) override;

private:
    HWND m_hClipHandler;
    IClipboardConfig* m_pConfig;
    CCopyThread m_copyThread;
    CClipboardEventDispatcher m_dispatcher;
    bool m_created;

    CClipboardMonitorImpl(const CClipboardMonitorImpl&) = delete;
    CClipboardMonitorImpl& operator=(const CClipboardMonitorImpl&) = delete;
};
