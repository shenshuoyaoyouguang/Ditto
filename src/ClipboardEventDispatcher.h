#pragma once

#include <vector>
#include <afxmt.h>
#include "..\Shared\IClipEventListener.h"

class CClipboardEventDispatcher
{
public:
    CClipboardEventDispatcher() = default;
    ~CClipboardEventDispatcher() = default;

    EventToken Subscribe(IClipEventListener* listener);
    void Unsubscribe(EventToken token);
    void FireEvent(const ClipEventInfo& info);

private:
    CCriticalSection m_cs;
    std::vector<IClipEventListener*> m_listeners;
};
