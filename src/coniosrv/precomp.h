/*++
Copyright (c) Microsoft Corporation

Module Name:
- precomp.h

Abstract:
- Contains external headers that do not change often to include in the
  precompilation phase of the console input server.

Author:
- HeGatta Mar.16.2017
--*/

#pragma once

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntcsrdll.h>
#include <ntcsrsrv.h>

#include <windows.h>

#include <Softpub.h>
#include <wincrypt.h>
#include <wintrust.h>
#include <mscat.h>
#include <siginfop.h>

#include <initguid.h>
#include <ntddkbd.h>
#include <kbd.h>

#include <condrv.h>

#include <cfgmgr32.h>
#include <intsafe.h>
#include <strsafe.h>

#pragma hdrstop