/*++
Copyright (c) Microsoft Corporation

Module Name:
- menu.h

Abstract:
- This module contains the definitions for console system menu

Author:
- Therese Stowell (ThereseS) Feb-3-1992 (swiped from Win3.1)

Revision History:
--*/

#pragma once

#include "resource.h"

void InitSystemMenu();
void InitializeMenu();

void UpdateWinText();
void PropertiesDlgShow(_In_ HWND const hwnd, _In_ BOOL const Defaults);

void GetConsoleState(_Out_ CONSOLE_STATE_INFO * const pStateInfo);
