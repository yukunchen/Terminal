/*++
Copyright (c) Microsoft Corporation

Module Name:
- KeyEventHelpers.hpp

Abstract:
- Some helpers for key events that cannot otherwise be a part of the KeyEvent 
    class, because of their dependency on information about ExtendedEditKeys, 
    which is stored on the host itself.

Author:
- Austin Diviness (AustDi) 18-Aug-2017
--*/
#pragma once
#include "../types/inc/IInputEvent.hpp"

bool IsPauseKey(_In_ const KeyEvent& KeyEvent);
const ExtKeySubst* const ParseEditKeyInfo(_In_ KeyEvent& KeyEvent);
const ExtKeySubst* const GetKeySubst(_In_ const KeyEvent& KeyEvent);
bool IsCommandLineEditingKey(_In_ const KeyEvent& KeyEvent);
bool IsCommandLinePopupKey(_In_ const KeyEvent& KeyEvent);
