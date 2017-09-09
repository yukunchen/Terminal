/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#pragma once

#include "..\inc\VtIoModes.hpp"

namespace Microsoft
{
    namespace Console
    {
        namespace VirtualTerminal
        {
            class VtIo;
        }
    }
}

class Microsoft::Console::VirtualTerminal::VtIo
{
public:
    VtIo();
    HRESULT Initialize(_In_ const std::wstring& InPipeName, _In_ const std::wstring& OutPipeName, _In_ const std::wstring& VtMode);
    bool IsUsingVt() const;

    HRESULT StartIfNeeded();

    static HRESULT ParseIoMode(_In_ const std::wstring& VtMode, _Out_ VtIoMode* const pIoMode);
    
private:
    bool _usingVt;
    VtIoMode _IoMode;
};