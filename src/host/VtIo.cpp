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

using namespace Microsoft::Console;
using namespace Microsoft::Console::VirtualTerminal;
using namespace Microsoft::Console::Types;

VtIo::VtIo() :
    _usingVt(false),
    _hasSignalThread(false),
    _lookingForCursorPosition(false),
    _IoMode(VtIoMode::INVALID)
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
[[nodiscard]]
HRESULT VtIo::ParseIoMode(const std::wstring& VtMode, _Out_ VtIoMode& ioMode)
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

[[nodiscard]]
HRESULT VtIo::Initialize(const ConsoleArguments * const pArgs)
{
    _lookingForCursorPosition = pArgs->GetInheritCursor();

    // If we were already given VT handles, set up the VT IO engine to use those.
    if (pArgs->HasVtHandles())
    {
        return _Initialize(pArgs->GetVtInHandle(), pArgs->GetVtOutHandle(), pArgs->GetVtMode(), pArgs->GetSignalHandle());
    }
    // Didn't need to initialize if we didn't have VT stuff. It's still OK, but report we did nothing.
    else
    {
        return S_FALSE;
    }
}

// Routine Description:
// - See _Initialize with 4 parameters. This one is for back-compat with old function signatures that have no signal handle.
//   It sets the signal handle to 0.
// Arguments:
//  InHandle: a valid file handle. The console will
//      read VT sequences from this pipe to generate INPUT_RECORDs and other
//      input events.
//  OutHandle: a valid file handle. The console
//      will be "rendered" to this pipe using VT sequences
//  VtIoMode: A string containing the console's requested VT mode. This can be
//      any of the strings in VtIoModes.hpp
//  SignalHandle: an optional file handle that will be used to send signals into the console.
//      This represents the ability to send signals to a *nix tty/pty.
// Return Value:
//  S_OK if we initialized successfully, otherwise an appropriate HRESULT
//      indicating failure.
[[nodiscard]]
HRESULT VtIo::_Initialize(const HANDLE InHandle, const HANDLE OutHandle, const std::wstring& VtMode)
{
    return _Initialize(InHandle, OutHandle, VtMode, INVALID_HANDLE_VALUE);
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
//  SignalHandle: an optional file handle that will be used to send signals into the console.
//      This represents the ability to send signals to a *nix tty/pty.
// Return Value:
//  S_OK if we initialized successfully, otherwise an appropriate HRESULT
//      indicating failure.
[[nodiscard]]
HRESULT VtIo::_Initialize(const HANDLE InHandle, const HANDLE OutHandle, const std::wstring& VtMode, _In_opt_ HANDLE SignalHandle)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();

    RETURN_IF_FAILED(ParseIoMode(VtMode, _IoMode));

    wil::unique_hfile hInputFile;
    wil::unique_hfile hOutputFile;

    hInputFile.reset(InHandle);
    RETURN_LAST_ERROR_IF(hInputFile.get() == INVALID_HANDLE_VALUE);

    hOutputFile.reset(OutHandle);
    RETURN_LAST_ERROR_IF(hOutputFile.get() == INVALID_HANDLE_VALUE);

    try
    {
        _pVtInputThread = std::make_unique<VtInputThread>(std::move(hInputFile), _lookingForCursorPosition);

        Viewport initialViewport = Viewport::FromDimensions({0, 0},
                                                            gci.GetWindowSize().X,
                                                            gci.GetWindowSize().Y);
        switch (_IoMode)
        {
        case VtIoMode::XTERM_256:
            _pVtRenderEngine = std::make_unique<Xterm256Engine>(std::move(hOutputFile),
                                                                gci,
                                                                initialViewport,
                                                                gci.GetColorTable(),
                                                                static_cast<WORD>(gci.GetColorTableSize()));
            break;
        case VtIoMode::XTERM:
            _pVtRenderEngine = std::make_unique<XtermEngine>(std::move(hOutputFile),
                                                             gci,
                                                             initialViewport,
                                                             gci.GetColorTable(),
                                                             static_cast<WORD>(gci.GetColorTableSize()),
                                                             false);
            break;
        case VtIoMode::XTERM_ASCII:
            _pVtRenderEngine = std::make_unique<XtermEngine>(std::move(hOutputFile),
                                                             gci,
                                                             initialViewport,
                                                             gci.GetColorTable(),
                                                             static_cast<WORD>(gci.GetColorTableSize()),
                                                             true);
            break;
        case VtIoMode::WIN_TELNET:
            _pVtRenderEngine = std::make_unique<WinTelnetEngine>(std::move(hOutputFile),
                                                                 gci,
                                                                 initialViewport,
                                                                 gci.GetColorTable(),
                                                                 static_cast<WORD>(gci.GetColorTableSize()));
            break;
        default:
            return E_FAIL;
        }
    }
    CATCH_RETURN();

    // If we were passed a signal handle, try to open it and make a signal reading thread.
    if (0 != SignalHandle && INVALID_HANDLE_VALUE != SignalHandle)
    {
        wil::unique_hfile hSignalFile(SignalHandle);
        try
        {
            _pPtySignalInputThread = std::make_unique<PtySignalInputThread>(std::move(hSignalFile));
            _hasSignalThread = true;
        }
        CATCH_RETURN();
    }

    _usingVt = true;
    return S_OK;
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
[[nodiscard]]
HRESULT VtIo::StartIfNeeded()
{
    // If we haven't been set up, do nothing (because there's nothing to start)
    if (!IsUsingVt())
    {
        return S_FALSE;
    }
    Globals& g = ServiceLocator::LocateGlobals();

    try
    {
        g.pRender->AddRenderEngine(_pVtRenderEngine.get());
        g.getConsoleInformation().GetActiveOutputBuffer().SetTerminalConnection(_pVtRenderEngine.get());
    }
    CATCH_RETURN();

    // MSFT: 15813316
    // If the terminal application wants us to inherit the cursor position,
    //  we're going to emit a VT sequence to ask for the cursor position, then
    //  read input until we get a response. Terminals who request this behavior
    //  but don't respond will hang.
    // If we get a response, the InteractDispatch will call SetCursorPosition,
    //      which will call to our VtIo::SetCursorPosition method.
    if (_lookingForCursorPosition)
    {
        LOG_IF_FAILED(_pVtRenderEngine->RequestCursor());
        while(_lookingForCursorPosition)
        {
            _pVtInputThread->DoReadInput(false);
        }
    }

    LOG_IF_FAILED(_pVtInputThread->Start());

    if (_hasSignalThread)
    {
        LOG_IF_FAILED(_pPtySignalInputThread->Start());
    }

    return S_OK;
}

// Method Description:
// - Prevent the renderer from emitting output on the next resize. This prevents
//      the host from echoing a resize to the terminal that requested it.
// Arguments:
// - <none>
// Return Value:
// - S_OK if the renderer successfully suppressed the next repaint, otherwise an
//      appropriate HRESULT indicating failure.
[[nodiscard]]
HRESULT VtIo::SuppressResizeRepaint()
{
    HRESULT hr = S_OK;
    if (_pVtRenderEngine)
    {
        hr = _pVtRenderEngine->SuppressResizeRepaint();
    }
    return hr;
}

// Method Description:
// - Attempts to set the initial cursor position, if we're looking for it.
//      If we're not trying to inherit the cursor, does nothing.
// Arguments:
// - coordCursor: The initial position of the cursor.
// Return Value:
// - S_OK if we successfully inherited the cursor or did nothing, else an
//      appropriate HRESULT
[[nodiscard]]
HRESULT VtIo::SetCursorPosition(const COORD coordCursor)
{
    HRESULT hr = S_OK;
    if (_lookingForCursorPosition)
    {
        if (_pVtRenderEngine)
        {
            hr = _pVtRenderEngine->InheritCursor(coordCursor);
        }

        _lookingForCursorPosition = false;
    }
    return hr;
}
