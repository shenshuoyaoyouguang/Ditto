#pragma once

#include <memory>
#include "..\Shared\IClipboardMonitor.h"

std::unique_ptr<IClipboardMonitor> CreateClipboardMonitor();
