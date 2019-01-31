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

#include <winrt/Windows.system.h>
#include <winrt/windows.ui.xaml.hosting.h>
#include <windows.ui.xaml.hosting.desktopwindowxamlsource.h>
#include <winrt/windows.ui.core.h>
#include <winrt/windows.ui.xaml.controls.h>
#include <winrt/Windows.ui.xaml.media.h>
#include <winrt/Windows.ui.xaml.input.h>

#include <windows.ui.xaml.media.dxinterop.h>

////////////////////////////////////////////////////////////////////////////////
#include <algorithm>
#include <atomic>
#include <deque>
#include <list>
#include <memory>
#include <map>
#include <mutex>
#include <new>
#include <optional>
#include <queue>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <utility>
#include <vector>
#include <unordered_map>
#include <iterator>
#include <math.h>
// WIL

#include <wil/Common.h>
#include <wil/Result.h>
#include <wil/resource.h>
#include <wil/wistd_memory.h>

// GSL
// Block GSL Multi Span include because it both has C++17 deprecated iterators
// and uses the C-namespaced "max" which conflicts with Windows definitions.
#define GSL_MULTI_SPAN_H
#include <gsl/gsl>
