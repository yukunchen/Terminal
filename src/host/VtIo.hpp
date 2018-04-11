/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#pragma once

#include "..\inc\VtIoModes.hpp"
#include "..\inc\ITerminalOutputConnection.hpp"
#include "..\renderer\vt\vtrenderer.hpp"
#include "VtInputThread.hpp"
#include "PtySignalInputThread.hpp"

class ConsoleArguments;

namespace Microsoft::Console::VirtualTerminal
{
    class VtIo
    {
    public:
        VtIo();

        [[nodiscard]]
        HRESULT Initialize(const ConsoleArguments* const pArgs);

        bool IsUsingVt() const;

        [[nodiscard]]
        HRESULT StartIfNeeded();

        [[nodiscard]]
        static HRESULT ParseIoMode(const std::wstring& VtMode, _Out_ VtIoMode& ioMode);

        [[nodiscard]]
        HRESULT SuppressResizeRepaint();
        [[nodiscard]]
        HRESULT SetCursorPosition(const COORD coordCursor);

    private:
        bool _usingVt;
        bool _hasSignalThread;
        VtIoMode _IoMode;
        bool _lookingForCursorPosition;

        [[nodiscard]]
        HRESULT _Initialize(const HANDLE InHandle, const HANDLE OutHandle, const std::wstring& VtMode);
        [[nodiscard]]
        HRESULT _Initialize(const HANDLE InHandle, const HANDLE OutHandle, const std::wstring& VtMode, _In_opt_ HANDLE SignalHandle);
        [[nodiscard]]
        HRESULT _Initialize(const std::wstring& InPipeName, const std::wstring& OutPipeName, const std::wstring& VtMode);
        [[nodiscard]]
        HRESULT _Initialize(const std::wstring& InPipeName, const std::wstring& OutPipeName, const std::wstring& VtMode, _In_opt_ HANDLE SignalHandle);

        std::unique_ptr<Microsoft::Console::Render::VtEngine> _pVtRenderEngine;
        std::unique_ptr<Microsoft::Console::VtInputThread> _pVtInputThread;
        std::unique_ptr<Microsoft::Console::PtySignalInputThread> _pPtySignalInputThread;

    #ifdef UNIT_TESTING
        friend class VtIoTests;
    #endif
    };
}
