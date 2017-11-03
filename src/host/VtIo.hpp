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

class ConsoleArguments;

class Microsoft::Console::VirtualTerminal::VtIo
{
public:
    VtIo();

    HRESULT Initialize(_In_ const ConsoleArguments* const pArgs);
    
    bool IsUsingVt() const;

    HRESULT StartIfNeeded();

    static HRESULT ParseIoMode(_In_ const std::wstring& VtMode, _Out_ VtIoMode& ioMode);
    
private:
    bool _usingVt;
    VtIoMode _IoMode;

    HRESULT _Initialize(_In_ const HANDLE InHandle, _In_ const HANDLE OutHandle, _In_ const std::wstring& VtMode);
    HRESULT _Initialize(_In_ const std::wstring& InPipeName, _In_ const std::wstring& OutPipeName, _In_ const std::wstring& VtMode);

    std::unique_ptr<Microsoft::Console::Render::VtEngine> _pVtRenderEngine;
    std::unique_ptr<Microsoft::Console::VtInputThread> _pVtInputThread;

#ifdef UNIT_TESTING
    friend class VtIoTests;
#endif
};
