//
//    Copyright (C) Microsoft.  All rights reserved.
//

#pragma once

#include "windows.h"
#include "wincon.h"
#include "windowsx.h"

#include "WexTestClass.h"

using namespace WEX::Common;
using namespace WEX::Logging;
using namespace WEX::TestExecution;

#include "common.hpp"

#define WIL_SUPPORT_BITOPERATION_PASCAL_NAMES
#include <wil\Common.h>
#include <wil\Result.h>
#include <wil\Resource.h>

#include <conio.h>

#define CM_SET_KEY_STATE (WM_USER+18)
#define CM_SET_KEYBOARD_LAYOUT (WM_USER+19)
