//
//    Copyright (C) Microsoft.  All rights reserved.
//

#pragma once

#define DEFINE_CONSOLEV2_PROPERTIES
#define INC_OLE2


// This includes a lot of common headers needed by both the host and the propsheet
// including: windows.h, winuser, ntstatus, assert, and the DDK
#include "..\CommonIncludes.h"

#include <windowsx.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <winbase.h>
#include <winconp.h>
#include <wingdi.h>

// -- WARNING -- LOAD BEARING CODE --
// This define ABSOLUTELY MUST be included (and equal to 1, or more specifically != 0)
// prior to the import of Common Controls.
// Failure to do so will result in a state where property sheet pages load without complete theming,
// suddenly start disappearing and closing themselves (while throwing no error in the debugger)
// or otherwise failing to load the correct version of ComCtl or the string resources you expect.
// For more details, see https://msdn.microsoft.com/en-us/library/windows/desktop/bb773175(v=vs.85).aspx
// DO NOT REMOVE.
#define ISOLATION_AWARE_ENABLED 1
// -- END WARNING
#include <commctrl.h>

#include "globals.h"
               
#include "console.h"    
#include "menu.h"
#include "dialogs.h"

#include <strsafe.h>
#include <intsafe.h>
#include <wchar.h>
#include <shellapi.h>

#include "strid.h"
#include "..\propslib\conpropsp.hpp"

// This is currently bubbling up the source tree to our branch
#ifndef WM_DPICHANGED_BEFOREPARENT
#define WM_DPICHANGED_BEFOREPARENT      0x02E2
#endif

