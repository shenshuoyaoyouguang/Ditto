#pragma once

#include <memory>
#include <vector>
#include "ClipboardTypes.h"
#include "IClipboardConfig.h"
#include "IClipEventListener.h"

class IClipboardMonitor
{
public:
    virtual ~IClipboardMonitor() = default;

    virtual bool Create(HWND hClipHandler, IClipboardConfig* pConfig) = 0;
    virtual void Destroy() = 0;

    virtual EventToken Subscribe(IClipEventListener* listener) = 0;
    virtual void Unsubscribe(EventToken token) = 0;

    virtual std::vector<ClipEntry> QueryClips(const ClipQueryFilter& filter) = 0;
    virtual std::vector<ClipEntry> GetTopPasted(size_t count) = 0;
    virtual size_t CountClips(const ClipQueryFilter& filter) = 0;

    virtual void NotifyPaste(ClipId id) = 0;
    virtual void NotifyPasteBatch(const std::vector<ClipId>& ids) = 0;

    virtual bool IsClipboardViewerConnected() = 0;
    virtual bool GetConnectCV() = 0;
    virtual void SetConnectCV(bool bConnect) = 0;

    virtual void NotifyClipCaptured(ClipId clipId, const CString& description, int formatCount) = 0;
};
