/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#pragma once

HRESULT ConsoleCreateIoThreadLegacy(_In_ HANDLE Server);

HRESULT UseVtPipe(_In_ const std::wstring& InPipeName, _In_ const std::wstring& OutPipeName, _In_ const std::wstring& VtMode);