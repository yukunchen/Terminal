/*++
Copyright (c) Microsoft Corporation

Module Name:
- precomp.h

Abstract:
- Contains external headers to include in the precompile phase of console build process.
- Avoid including internal project headers. Instead include them only in the classes that need them (helps with test project building).
--*/

#pragma once

// Ignore checked iterators warning from VC compiler.
#define _SCL_SECURE_NO_WARNINGS

// Block minwindef.h min/max macros to prevent <algorithm> conflict
#define NOMINMAX

// // This includes support libraries from the CRT, STL, WIL, and GSL
// #include "LibraryIncludes.h"

#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)


#include <windows.h>
#include <stdlib.h>
#include <string.h>

// This is inexplicable, but for whatever reason, cppwinrt conflicts with the
//      SDK definition of this function, so the only fix is to undef it.
// from WinBase.h
// Windows::UI::Xaml::Media::Animation::IStoryboard::GetCurrentTime
#ifdef GetCurrentTime
#undef GetCurrentTime
#endif
// Needed just for XamlIslands to work at all:
#include <winrt/Windows.system.h>
#include <winrt/windows.ui.xaml.hosting.h>
// #include <winrt/windows.ui.xaml.hosting.desktopwindowxamlsource.h>
#include <windows.ui.xaml.hosting.desktopwindowxamlsource.h>

// Additional headers for various xaml features. We need:
//  * Controls for grid
//  * Media for ScaleTransform
#include "winrt/Windows.UI.Xaml.Controls.h"
#include <winrt/Windows.ui.xaml.media.h>
