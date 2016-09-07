/*++

Copyright (c) 2003, Microsoft Corporation

Module Name:

    consubs.cpp

Abstract:

    This file implements the some sub functions

Author:

Revision History:

Notes:

--*/

#include "precomp.h"

//+---------------------------------------------------------------------------
//
// CConsoleTSF::OnStartIMM32Composition()
// (WM_IME_STARTCOMPOSITION message handler)
//
//----------------------------------------------------------------------------

void CConsoleTSF::OnStartIMM32Composition()
{
    if (_pConsoleTable)
    {
        _KeyboardLayoutInfo.fInComposition = TRUE;
        ConsoleImeSendMessage2(CI_ONSTARTCOMPOSITION, 0, NULL);
    }
}

//+---------------------------------------------------------------------------
//
// CConsoleTSF::OnEndIMM32Composition()
// (WM_IME_ENDCOMPOSITION message handler)
//
//----------------------------------------------------------------------------

void CConsoleTSF::OnEndIMM32Composition()
{
    // Reset fInComposition variables.
    if (_pConsoleTable)
    {
        _KeyboardLayoutInfo.fInComposition = FALSE;
        if (_pConsoleTable->lpCompStrMem)
        {
            delete [] (BYTE*)_pConsoleTable->lpCompStrMem;
            _pConsoleTable->lpCompStrMem = NULL;
        }
        ConsoleImeSendMessage2(CI_ONENDCOMPOSITION, 0, NULL);
    }
}

//+---------------------------------------------------------------------------
//
// CConsoleTSF::OnUpdateIMM32Composition
// (WM_IME_COMPOSITION message handler)
//
//----------------------------------------------------------------------------

void CConsoleTSF::OnUpdateIMM32Composition(WPARAM CompChar, LPARAM CompFlag)
{
    DebugMsg(TF_FUNC, "CONIME: WM_IME_COMPOSITION %08x %08x", CompChar, CompFlag);
    if (!_pConsoleTable)
    {
        return;
    }

    _pConsoleTable->AddRef();

    if (CompFlag == 0) {
        DebugMsg(TF_FUNC, "                           None");
        GetCompositionStr(_hwndConsole, _pConsoleTable, CompFlag, CompChar);
    }
    else if (CompFlag & GCS_RESULTSTR) {
        DebugMsg(TF_FUNC, "                           GCS_RESULTSTR");
        GetCompositionStr(_hwndConsole, _pConsoleTable, (CompFlag & GCS_RESULTSTR), CompChar);
    }
    else if (CompFlag & GCS_COMPSTR) {
        DebugMsg(TF_FUNC, "                           GCS_COMPSTR");
        GetCompositionStr(_hwndConsole, _pConsoleTable, (CompFlag & (GCS_COMPSTR|GCS_COMPATTR)), CompChar);
    }
    else if (CompFlag & CS_INSERTCHAR) {
        DebugMsg(TF_FUNC, "                           CS_INSERTCHAR");
        GetCompositionStr(_hwndConsole, _pConsoleTable, (CompFlag & (CS_INSERTCHAR|GCS_COMPATTR)), CompChar);
    }
    else if (CompFlag & CS_NOMOVECARET) {
        DebugMsg(TF_FUNC, "                           CS_NOMOVECARET");
        GetCompositionStr(_hwndConsole, _pConsoleTable, (CompFlag & (CS_NOMOVECARET|GCS_COMPATTR)), CompChar);
    }

    _pConsoleTable->Release();
}

//+---------------------------------------------------------------------------
//
// CConsoleTSF::OnImeNotify
// (WM_IME_NOTIFY message handler).
//
//----------------------------------------------------------------------------

BOOL CConsoleTSF::OnImeNotify(WPARAM wParam, LPARAM lParam)
{
    return FALSE;
}
