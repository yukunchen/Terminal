/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#pragma once

#include "..\inc\VtIoModes.hpp"
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

    HRESULT Initialize(_In_ const std::wstring& InPipeName, _In_ const std::wstring& OutPipeName, _In_ const std::wstring& VtMode);
    bool IsUsingVt() const;

    HRESULT StartIfNeeded();

    static HRESULT ParseIoMode(_In_ const std::wstring& VtMode, _Out_ VtIoMode& ioMode);
    
private:
    bool _usingVt;
    VtIoMode _IoMode;

    std::unique_ptr<Microsoft::Console::Render::VtEngine> _pVtRenderEngine;
    std::unique_ptr<Microsoft::Console::VtInputThread> _pVtInputThread;

#ifdef UNIT_TESTING
    friend class VtIoTests;
#endif
};