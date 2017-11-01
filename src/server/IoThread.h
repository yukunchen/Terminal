/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#pragma once

class ConsoleArguments;

HRESULT ConsoleCreateIoThreadLegacy(_In_ HANDLE Server, _In_ const ConsoleArguments* const args);
