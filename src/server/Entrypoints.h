/*++
Copyright (c) Microsoft Corporation

Module Name:
- Entrypoints.h

Abstract:
- This module defines methods to get a console session started.

Author:
- Michael Niksa (MiNiksa) 14-Sept-2016

Revision History:
--*/

#pragma once

namespace Entrypoints
{
    HRESULT StartConsoleForServerHandle(_In_ HANDLE const ServerHandle);
    HRESULT StartConsoleForCmdLine(_In_ PCWSTR pwszCmdLine);
};

