//
// pch.h
// Header for platform projection include files
//

#pragma once

#include <windows.h>
#include <unknwn.h>
#include <restrictederrorinfo.h>
#include <hstring.h>
#include "winrt/Windows.Foundation.h"
#include "winrt/Windows.ApplicationModel.Activation.h"
#include "winrt/Windows.UI.ViewManagement.h"
#include "winrt/Windows.UI.Xaml.h"
#include "winrt/Windows.UI.Xaml.Controls.h"
#include "winrt/Windows.UI.Xaml.Controls.Primitives.h"
#include "winrt/Windows.UI.Xaml.Data.h"
#include "winrt/Windows.UI.Xaml.Interop.h"
#include "winrt/Windows.UI.Xaml.Markup.h"
#include "winrt/Windows.UI.Xaml.Navigation.h"

// Win2d
// Make sure imports are here, and not closer to the consumer
// If you try and x:Name a win2d control who's defined in a header that isn't
//      included here, the compiler will complain that 'Microsoft' isn't a
//      recognized namespace in *.xaml.g.h
#include "winrt/Microsoft.Graphics.Canvas.h"
#include "winrt/Microsoft.Graphics.Canvas.UI.h"
#include "winrt/Microsoft.Graphics.Canvas.UI.Xaml.h"
#include "winrt/Microsoft.Graphics.Canvas.Text.h"

// shamelessly taken from libraryincludes, w/o WRL
// TODO remote WRL from library includes, I believe only DX needs it
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

// IntSafe
#define ENABLE_INTSAFE_SIGNED_FUNCTIONS
#include <intsafe.h>

