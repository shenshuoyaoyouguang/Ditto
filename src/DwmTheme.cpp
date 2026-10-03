#include "stdafx.h"
#include "DwmTheme.h"

#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

// attribute ids, in case the sdk in use predates them
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif
#ifndef DWMWA_SYSTEMBACKDROP_TYPE
#define DWMWA_SYSTEMBACKDROP_TYPE 38
#endif
#ifndef DWMWCP_DONOTROUND
#define DWMWCP_DONOTROUND 1
#define DWMWCP_ROUND 2
#endif

static HRESULT SetDwordAttribute(HWND hwnd, DWORD attribute, DWORD value)
{
	return DwmSetWindowAttribute(hwnd, attribute, &value, sizeof(value));
}

bool DwmTheme::ApplyRoundedCorners(HWND hwnd, bool enable)
{
	if (hwnd == NULL)
		return false;

	return SUCCEEDED(SetDwordAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE,
		enable ? DWMWCP_ROUND : DWMWCP_DONOTROUND));
}

bool DwmTheme::ApplyDarkCaption(HWND hwnd, bool dark)
{
	if (hwnd == NULL)
		return false;

	DWORD value = dark ? 1 : 0;

	// 20 is the documented id; early windows 10 builds only honor 19
	if (SUCCEEDED(SetDwordAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, value)))
		return true;

	return SUCCEEDED(SetDwordAttribute(hwnd, 19, value));
}

bool DwmTheme::CanUseBackdrop()
{
	static int cached = -1;
	if (cached < 0)
	{
		cached = 0;

		typedef LONG(WINAPI *RtlGetVersionFn)(PRTL_OSVERSIONINFOW);
		HMODULE ntdll = GetModuleHandle(_T("ntdll.dll"));
		if (ntdll != NULL)
		{
			RtlGetVersionFn getVersion = (RtlGetVersionFn)GetProcAddress(ntdll, "RtlGetVersion");
			if (getVersion != NULL)
			{
				RTL_OSVERSIONINFOW info;
				ZeroMemory(&info, sizeof(info));
				info.dwOSVersionInfoSize = sizeof(info);
				if (getVersion(&info) == 0 &&
					info.dwMajorVersion >= 10 &&
					info.dwBuildNumber >= 22621)
				{
					cached = 1;
				}
			}
		}
	}

	return cached != 0;
}

bool DwmTheme::ApplyBackdrop(HWND hwnd, Backdrop backdrop)
{
	if (hwnd == NULL || !CanUseBackdrop())
		return false;

	return SUCCEEDED(SetDwordAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE, (DWORD)backdrop));
}

bool DwmTheme::ExtendFrame(HWND hwnd)
{
	if (hwnd == NULL)
		return false;

	MARGINS margins = { -1, -1, -1, -1 };
	return SUCCEEDED(DwmExtendFrameIntoClientArea(hwnd, &margins));
}
