#pragma once

// DwmTheme: thin, fail-silent wrappers around the dwm attributes used by the
// fluent redesign (plan section 4.1). Every call degrades gracefully on os
// builds without the attribute - callers must not treat false as an error.

namespace DwmTheme
{
	// mirror of DWMSBT_* (DWMWA_SYSTEMBACKDROP_TYPE values)
	enum Backdrop
	{
		Backdrop_None = 1,
		Backdrop_Mica = 2,
		Backdrop_Acrylic = 3,
		Backdrop_MicaAlt = 4,
	};

	// 8px window corner rounding (win11; false on win10)
	bool ApplyRoundedCorners(HWND hwnd, bool enable);

	// dark/light title bar; the pre-20h1 attribute id 19 is tried as fallback
	bool ApplyDarkCaption(HWND hwnd, bool dark);

	// mica/acrylic backdrop (win11 22H2+); note mica additionally requires
	// DwmExtendFrameIntoClientArea + transparent client painting by the caller
	bool ApplyBackdrop(HWND hwnd, Backdrop backdrop);

	// true when the os build supports DWMWA_SYSTEMBACKDROP_TYPE (>= 22621)
	bool CanUseBackdrop();
}
