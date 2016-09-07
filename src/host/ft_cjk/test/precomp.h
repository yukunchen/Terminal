//
//    Copyright (C) Microsoft.  All rights reserved.
//

#pragma once

#include "windows.h"
#include "wincon.h"
#include "windowsx.h"
#include "msctfp.h"

#include "WexTestClass.h"

using namespace WEX::Common;
using namespace WEX::Logging;
using namespace WEX::TestExecution;

// The System OS should be Japanese with codepage 932.

#define JAPANESE_CP 932u
#define TIMEOUT 500

#define PATH_TO_TOOL L"Conhost.CJK.Tests.Tool.exe"

#include "Helpers.hpp"

#include <conio.h>
