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
    ~VtIo();
    HRESULT Initialize(const std::wstring& InPipeName, const std::wstring& OutPipeName, const std::wstring& VtMode);
    bool IsUsingVt();

    HRESULT Start();

    static HRESULT ParseIoMode(const std::wstring& VtMode, VtIoMode* const IoMode);
    
private:
    bool _usingVt;
    VtIoMode _IoMode;
    // Microsoft::Console::VtInputThread* _pVtInputThread;
    // Microsoft::Console::Render::VtEngine* _pVtRenderEngine;
};