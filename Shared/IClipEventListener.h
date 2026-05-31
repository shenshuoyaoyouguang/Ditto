#pragma once

#include "ClipboardTypes.h"

class IClipEventListener
{
public:
    virtual ~IClipEventListener() = default;
    virtual void OnClipEvent(const ClipEventInfo& info) = 0;
};
