//
// pch.h
// Header for platform projection include files
//

#pragma once

#include <unknwn.h>
#include "winrt/Windows.Foundation.h"
#include "winrt/Windows.UI.Xaml.Controls.h"
#include "winrt/Windows.UI.Xaml.Controls.Primitives.h"
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
////////////////////////////////////////////////////////////////////////////////
