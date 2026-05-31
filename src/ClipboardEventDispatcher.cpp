#include "stdafx.h"
#include "ClipboardEventDispatcher.h"

EventToken CClipboardEventDispatcher::Subscribe(IClipEventListener* listener)
{
    if (listener == nullptr)
        return InvalidEventToken;

    CSingleLock lock(&m_cs, TRUE);
    m_listeners.push_back(listener);
    return reinterpret_cast<EventToken>(listener);
}

void CClipboardEventDispatcher::Unsubscribe(EventToken token)
{
    if (token == InvalidEventToken)
        return;

    CSingleLock lock(&m_cs, TRUE);
    IClipEventListener* target = reinterpret_cast<IClipEventListener*>(token);
    for (auto it = m_listeners.begin(); it != m_listeners.end(); ++it)
    {
        if (*it == target)
        {
            m_listeners.erase(it);
            return;
        }
    }
}

void CClipboardEventDispatcher::FireEvent(const ClipEventInfo& info)
{
    CSingleLock lock(&m_cs, TRUE);
    for (auto* listener : m_listeners)
    {
        if (listener)
            listener->OnClipEvent(info);
    }
}
