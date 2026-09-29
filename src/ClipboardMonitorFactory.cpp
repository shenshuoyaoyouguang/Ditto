#include "stdafx.h"
#include "ClipboardMonitorFactory.h"
#include "ClipboardMonitorImpl.h"

std::unique_ptr<IClipboardMonitor> CreateClipboardMonitor()
{
    return std::make_unique<CClipboardMonitorImpl>();
}
