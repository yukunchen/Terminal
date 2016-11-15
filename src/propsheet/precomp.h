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

