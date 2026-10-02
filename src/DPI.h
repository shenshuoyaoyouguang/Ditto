#pragma once
#include <ShellScalingAPI.h>

// Definition: relative pixel = 1 pixel at 96 DPI and scaled based on actual DPI.
class CDPI
{
public:
	CDPI(HWND hwnd = NULL) : m_Initialized(false), m_dpi(96)
	{
		m_hWnd = hwnd;
	}

	// Never store a non-positive dpi: Scale() would collapse every layout value
	// to 0 and UnScale() would divide by zero (MulDiv returns -1), which then
	// gets persisted as window sizes / font heights.
	void Update(int dpi) { m_dpi = (dpi > 0 ? dpi : 96); m_Initialized = true; }

	// Get screen DPI.
	int GetDPI() { Init(); return m_dpi; }

	// Convert between raw pixels and relative pixels.
	int Scale(int x) { Init(); return MulDiv(x, m_dpi, 96); }
	int UnScale(int x) { Init(); return MulDiv(x, 96, m_dpi); }
	
	// Invalidate any cached metrics.
	void Invalidate() { m_Initialized = false; }

	void SetHwnd(HWND hwnd) { m_hWnd = hwnd; m_Initialized = false; Init(); }

private:
	void Init()
	{
		if (m_Initialized == false)
		{
			if (m_hWnd != NULL)
			{
				HMODULE hUser32 = LoadLibrary(_T("USER32.dll"));
				if (hUser32)
				{
					//windows 10
					typedef UINT(__stdcall *GetDpiForWindow)(HWND hwnd);
					GetDpiForWindow getDpi = (GetDpiForWindow)GetProcAddress(hUser32, "GetDpiForWindow");
					if (getDpi)
					{
						int dpi = getDpi(m_hWnd);
						if (dpi > 0)
						{
							this->Update(dpi);
							FreeLibrary(hUser32);
							return;
						}
					}
					else
					{
						//windows 8
						auto monitor = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
						HMODULE shCore = LoadLibrary(_T("Shcore.dll"));
						if (shCore)
						{
							typedef HRESULT(__stdcall *GetDpiForMonitor)(HMONITOR, UINT, UINT*, UINT*);
							GetDpiForMonitor monDpi = (GetDpiForMonitor)GetProcAddress(shCore, "GetDpiForMonitor");
							if (monDpi)
							{
								UINT x = 0;
								UINT y = 0;
								if (SUCCEEDED(monDpi(monitor, MDT_EFFECTIVE_DPI, &x, &y)) && x > 0)
								{
									this->Update(x);
									FreeLibrary(shCore);
									FreeLibrary(hUser32);
									return;
								}
							}
							FreeLibrary(shCore);
						}
					}
					FreeLibrary(hUser32);
				}
			}

			// Fallback: device caps for the window (screen dc when m_hWnd is NULL).
			HDC hdc = GetDC(m_hWnd);
			if (hdc)
			{
				int dpi = GetDeviceCaps(hdc, LOGPIXELSX);
				// A window dc must be released with the same hwnd it was taken
				// from; ReleaseDC(NULL, ...) leaks it.
				ReleaseDC(m_hWnd, hdc);

				if (dpi > 0)
				{
					this->Update(dpi);
					return;
				}
			}

			// Last resort. Always mark initialized so a failing probe is not
			// retried on every Scale() call (which leaked a dc each time).
			this->Update(96);
		}
	}

private:
	bool m_Initialized;
	int m_dpi;
	HWND m_hWnd;
};
