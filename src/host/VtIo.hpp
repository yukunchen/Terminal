/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#pragma once
// #include "precomp.h"

#include "..\renderer\vt\vtrenderer.hpp"
#include "VtInputThread.hpp"

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
    HRESULT Initialize(const std::wstring& InPipeName, const std::wstring& OutPipeName);
    bool IsUsingVt();

    HRESULT Start();

private:
    bool _usingVt;

    // wil::unique_hfile _hInputFile;
    // wil::unique_hfile _hOutputFile;
    
    Microsoft::Console::VtInputThread* _pVtInputThread;
    Microsoft::Console::Render::VtEngine* _pVtRenderEngine;
};