/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"


BOOL InitializeConsoleState()
{
    RegisterClasses(ghInstance);
    OEMCP = GetOEMCP();

    return NT_SUCCESS(InitializeDbcsMisc());
}

void UninitializeConsoleState()
{
    if (g_fHostedInFileProperties && gpStateInfo->LinkTitle != nullptr)
    {
        // If we're in the file props dialog and have an allocated title, we need to free it. Outside of the file props
        // dlg, the caller of ConsolePropertySheet() owns the lifetime.
        CoTaskMemFree(gpStateInfo->LinkTitle);
        gpStateInfo->LinkTitle = nullptr;
    }

    DestroyDbcsMisc();
    UnregisterClasses(ghInstance);
}

void UpdateApplyButton(_In_ const HWND hDlg)
{
    if (g_fHostedInFileProperties)
    {
        PropSheet_Changed(GetParent(hDlg), hDlg);
    }
}
