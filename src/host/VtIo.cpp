/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "VtIo.hpp"
#include "../interactivity/inc/ServiceLocator.hpp"

#include "../renderer/vt/XtermEngine.hpp"
#include "../renderer/vt/Xterm256Engine.hpp"
#include "../renderer/vt/WinTelnetEngine.hpp"

#include "../renderer/base/renderer.hpp"

using namespace Microsoft::Console::VirtualTerminal;
using namespace Microsoft::Console::Types;

VtIo::VtIo() :
    _usingVt(false)
{
}

// Routine Description:
//  Tries to get the VtIoMode from the given string. If it's not one of the 
//      *_STRING constants in VtIoMode.hpp, then it returns E_INVALIDARG.
// Arguments:
//  VtIoMode: A string containing the console's requested VT mode. This can be 
//      any of the strings in VtIoModes.hpp
//  pIoMode: recieves the VtIoMode that the string prepresents if it's a valid 
//      IO mode string
// Return Value:
//  S_OK if we parsed the string successfully, otherwise E_INVALIDARG indicating failure.
HRESULT VtIo::ParseIoMode(_In_ const std::wstring& VtMode, _Out_ VtIoMode& ioMode)
{
    ioMode = VtIoMode::INVALID;

    if (VtMode == XTERM_256_STRING)
    {
        ioMode = VtIoMode::XTERM_256;
    }
    else if (VtMode == XTERM_STRING)
    {
        ioMode = VtIoMode::XTERM;
    }
    else if (VtMode == WIN_TELNET_STRING)
    {
        ioMode = VtIoMode::WIN_TELNET;
    }
    else if (VtMode == XTERM_ASCII_STRING)
    {
        ioMode = VtIoMode::XTERM_ASCII;
    }
    else if (VtMode == DEFAULT_STRING)
    {
        ioMode = VtIoMode::XTERM_256;
    }
    else
    {
        return E_INVALIDARG;
    }
    return S_OK;
}

HRESULT VtIo::Initialize(const ConsoleArguments * const pArgs)
{
    // If we were already given VT handles, set up the VT IO engine to use those.
    if (pArgs->HasVtHandles())
    {
        return _Initialize(pArgs->GetVtInHandle(), pArgs->GetVtOutHandle(), pArgs->GetVtMode());
    }
    // Otherwise, if we were given VT pipe names, try to open those.
    else if (pArgs->IsUsingVtPipe())
    {
        return _Initialize(pArgs->GetVtInPipe(), pArgs->GetVtOutPipe(), pArgs->GetVtMode());
    }
    // Didn't need to initialize if we didn't have VT stuff. It's still OK, but report we did nothing.
    else
    {
        return S_FALSE;
    }
}

// Routine Description:
//  Tries to initialize this VtIo instance from the given pipe handles and 
//      VtIoMode. The pipes should have been created already (by the caller of 
//      conhost), in non-overlapped mode.
//  The VtIoMode string can be the empty string as a default value.
// Arguments:
//  InHandle: a valid file handle. The console will 
//      read VT sequences from this pipe to generate INPUT_RECORDs and other 
//      input events.
//  OutHandle: a valid file handle. The console 
//      will be "rendered" to this pipe using VT sequences
//  VtIoMode: A string containing the console's requested VT mode. This can be 
//      any of the strings in VtIoModes.hpp
// Return Value:
//  S_OK if we initialized successfully, otherwise an appropriate HRESULT
//      indicating failure.
HRESULT VtIo::_Initialize(_In_ const HANDLE InHandle, _In_ const HANDLE OutHandle, _In_ const std::wstring& VtMode)
{
    const CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();

    RETURN_IF_FAILED(ParseIoMode(VtMode, _IoMode));

    wil::unique_hfile hInputFile;
    wil::unique_hfile hOutputFile;

    hInputFile.reset(InHandle);
    RETURN_LAST_ERROR_IF(hInputFile.get() == INVALID_HANDLE_VALUE);

    hOutputFile.reset(OutHandle);
    RETURN_LAST_ERROR_IF(hOutputFile.get() == INVALID_HANDLE_VALUE);

    try
    {
        _pVtInputThread = std::make_unique<VtInputThread>(std::move(hInputFile));

        // The Screen info hasn't been created yet, but SetUpConsole did get 
        //      the launch settings already.
        Viewport initialViewport = Viewport::FromDimensions({0, 0},
                                                            gci->GetWindowSize().X,
                                                            gci->GetWindowSize().Y);
        switch (_IoMode)
        {
        case VtIoMode::XTERM_256:
            _pVtRenderEngine = std::make_unique<Xterm256Engine>(std::move(hOutputFile),
                                                                initialViewport,
                                                                gci->GetColorTable(),
                                                                static_cast<WORD>(gci->GetColorTableSize()));
            break;
        case VtIoMode::XTERM:
            _pVtRenderEngine = std::make_unique<XtermEngine>(std::move(hOutputFile),
                                                             initialViewport,
                                                             gci->GetColorTable(),
                                                             static_cast<WORD>(gci->GetColorTableSize()),
                                                             false);
            break;
        case VtIoMode::XTERM_ASCII:
            _pVtRenderEngine = std::make_unique<XtermEngine>(std::move(hOutputFile),
                                                             initialViewport,
                                                             gci->GetColorTable(),
                                                             static_cast<WORD>(gci->GetColorTableSize()),
                                                             true);
            break;
        case VtIoMode::WIN_TELNET:
            _pVtRenderEngine = std::make_unique<WinTelnetEngine>(std::move(hOutputFile),
                                                                 initialViewport,
                                                                 gci->GetColorTable(),
                                                                 static_cast<WORD>(gci->GetColorTableSize()));
            break;
        default:
            return E_FAIL;
        }
    }
    CATCH_RETURN();

    _usingVt = true;
    return S_OK;
}

// Routine Description:
//  Tries to initialize this VtIo instance from the given pipe names and 
//      VtIoMode. The pipes should have been created already (by the caller of 
//      conhost), in non-overlapped mode.
//  The VtIoMode string can be the empty string as a default value.
// Arguments:
//  InPipeName: a wstring containing the vt input pipe's name. The console will 
//      read VT sequences from this pipe to generate INPUT_RECORDs and other 
//      input events.
//  OutPipeName: a wstring containing the vt output pipe's name. The console 
//      will be "rendered" to this pipe using VT sequences
//  VtIoMode: A string containing the console's requested VT mode. This can be 
//      any of the strings in VtIoModes.hpp
// Return Value:
//  S_OK if we initialized successfully, otherwise an appropriate HRESULT
//      indicating failure.
HRESULT VtIo::_Initialize(_In_ const std::wstring& InPipeName, _In_ const std::wstring& OutPipeName, _In_ const std::wstring& VtMode)
{
    wil::unique_hfile hInputFile;
    hInputFile.reset(
        CreateFileW(InPipeName.c_str(),
                    GENERIC_READ, 
                    0, 
                    nullptr, 
                    OPEN_EXISTING, 
                    FILE_ATTRIBUTE_NORMAL, 
                    nullptr)
    );
    RETURN_LAST_ERROR_IF(hInputFile.get() == INVALID_HANDLE_VALUE);

    wil::unique_hfile hOutputFile;
    hOutputFile.reset(
        CreateFileW(OutPipeName.c_str(),
                    GENERIC_WRITE, 
                    0, 
                    nullptr, 
                    OPEN_EXISTING, 
                    FILE_ATTRIBUTE_NORMAL, 
                    nullptr)
    );
    RETURN_LAST_ERROR_IF(hOutputFile.get() == INVALID_HANDLE_VALUE);

    return _Initialize(hInputFile.release(), hOutputFile.release(), VtMode);
}

bool VtIo::IsUsingVt() const
{
    return _usingVt;
}

// Routine Description:
//  Potentially starts this VtIo's input thread and render engine.
//      If the VtIo hasn't yet been given pipes, then this function will 
//      silently do nothing. It's the responsibility of the caller to make sure 
//      that the pipes are initialized first with VtIo::Initialize
// Arguments:
//  <none>
// Return Value:
//  S_OK if we started successfully or had nothing to start, otherwise an 
//      appropriate HRESULT indicating failure.
HRESULT VtIo::StartIfNeeded()
{
    // If we haven't been set up, do nothing (because there's nothing to start)
    if (!IsUsingVt())
    {
        return S_FALSE;
    }
    // Hmm. We only have one Renderer implementation, 
    //  but its stored as a IRenderer. 
    //  IRenderer doesn't know about IRenderEngine. 
    // todo: msft:13631640
    const Globals* const g = ServiceLocator::LocateGlobals();
    try
    {
        g->pRender->AddRenderEngine(_pVtRenderEngine.get());
    }
    CATCH_RETURN();

    _pVtInputThread->Start();

    return S_OK;
}
