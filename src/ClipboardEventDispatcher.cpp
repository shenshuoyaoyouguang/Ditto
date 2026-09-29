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
    // 在锁内复制快照，释放锁后再调用回调，避免重入死锁
    std::vector<IClipEventListener*> snapshot;
    {
        CSingleLock lock(&m_cs, TRUE);
        snapshot = m_listeners;
    }
    for (auto* listener : snapshot)
    {
        if (listener)
            listener->OnClipEvent(info);
    }
}
