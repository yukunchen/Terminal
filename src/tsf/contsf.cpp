//
//    Copyright (C) Microsoft.  All rights reserved.
//
#include "precomp.h"

CConsoleTSF* g_pConsoleTSF = NULL;

extern "C" BOOL ActivateTextServices(HWND hwndConsole, GetSuggestionWindowPos pfnPosition)
{
    if (!g_pConsoleTSF && hwndConsole)
    {
        g_pConsoleTSF = new CConsoleTSF(hwndConsole, pfnPosition);
        if (g_pConsoleTSF && SUCCEEDED(g_pConsoleTSF->Initialize()))
        {
            // Conhost calls this function only when the console window has focus.
            g_pConsoleTSF->SetFocus(TRUE);
        }
        else
        {
            SafeReleaseClear(g_pConsoleTSF);
        }
    }
    return g_pConsoleTSF ? TRUE : FALSE;
}

extern "C" void DeactivateTextServices()
{
    if (g_pConsoleTSF)
    {
        g_pConsoleTSF->Uninitialize();
        SafeReleaseClear(g_pConsoleTSF);
    }
}

extern "C" BOOL NotifyTextServices(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* lplResult)
{
    DebugMsg(TF_FUNC, "NotifyTextServices:: uMsg=%x, wParam=%x, lParam=%x", uMsg, wParam, lParam);
    if (lplResult)
    {
        *lplResult = 0;
    }
    if (!g_pConsoleTSF)
    {
        return FALSE;
    }

    BOOL bRet = TRUE;
    switch (uMsg)
    {
        case CONIME_SETFOCUS:
            g_pConsoleTSF->SetFocus(TRUE);
            break;

        case CONIME_KILLFOCUS:
            g_pConsoleTSF->SetFocus(FALSE);
            break;
        
        default:
            if (g_pConsoleTSF->HasFocus())
            {
                switch (uMsg)
                {
                    case WM_IME_STARTCOMPOSITION:
                        g_pConsoleTSF->OnStartIMM32Composition();
                        break;

                    case WM_IME_ENDCOMPOSITION:
                        g_pConsoleTSF->OnEndIMM32Composition();
                        break;

                    case WM_IME_COMPOSITION:
                        g_pConsoleTSF->OnUpdateIMM32Composition(wParam, lParam);
                        break;

                    case WM_IME_NOTIFY:
                        bRet = g_pConsoleTSF->OnImeNotify(wParam, lParam);
                        break;
                }
            }
            else
            {
                // Pass it to the DefProc
                bRet = FALSE;
            }
            break;
    }

    return bRet;
}


//+---------------------------------------------------------------------------
//
// ConsoleImeSendMessage
//
//----------------------------------------------------------------------------

LRESULT ConsoleImeSendMessage(COPYDATASTRUCT* pCopyData)
{
    HWND hwndConsole =  g_pConsoleTSF ? g_pConsoleTSF->GetConsoleHwnd() : NULL;
    return hwndConsole ? SendMessage(hwndConsole, WM_COPYDATA, 0, (LPARAM)pCopyData) : (LRESULT)-1;
}

//+---------------------------------------------------------------------------
//
// ConsoleImeSendMessage
//
//----------------------------------------------------------------------------

LRESULT ConsoleImeSendMessage2(ULONG_PTR dwData, DWORD cbData, __field_bcount(cbData) PVOID lpData)
{
    COPYDATASTRUCT copyData;
    copyData.dwData = dwData;
    copyData.cbData = cbData;
    copyData.lpData = lpData;
    return ConsoleImeSendMessage(&copyData);
}

