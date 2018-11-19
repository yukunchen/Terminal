#pragma once

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
