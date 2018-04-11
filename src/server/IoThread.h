/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#pragma once

class ConsoleArguments;

[[nodiscard]]
HRESULT ConsoleCreateIoThreadLegacy(_In_ HANDLE Server, const ConsoleArguments* const args);
