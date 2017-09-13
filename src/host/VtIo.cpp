/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "VtIo.hpp"
#include "..\interactivity\inc\ServiceLocator.hpp"

#include "..\renderer\base\renderer.hpp"

using namespace Microsoft::Console::VirtualTerminal;

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
HRESULT VtIo::ParseIoMode(_In_ const std::wstring& VtMode, _Out_ VtIoMode* const pIoMode)
{
    RETURN_HR_IF_NULL(E_INVALIDARG, pIoMode);

    if (VtMode == XTERM_256_STRING)
    {
        *pIoMode = VtIoMode::XTERM_256;
    }
    else if (VtMode == XTERM_STRING)
    {
        *pIoMode = VtIoMode::XTERM;
    }
    else if (VtMode == WIN_TELNET_STRING)
    {
        *pIoMode = VtIoMode::WIN_TELNET;
    }
    else if (VtMode == DEFAULT_STRING)
    {
        *pIoMode = VtIoMode::XTERM_256;
    }
    else
    {
        return E_INVALIDARG;
    }
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
HRESULT VtIo::Initialize(_In_ const std::wstring& InPipeName, _In_ const std::wstring& OutPipeName, _In_ const std::wstring& VtMode)
{

    RETURN_IF_FAILED(ParseIoMode(VtMode, &_IoMode));

    wil::unique_hfile _hInputFile;
    wil::unique_hfile _hOutputFile;

    _hInputFile.reset(
        CreateFileW(InPipeName.c_str(),
                    GENERIC_READ, 
                    0, 
                    nullptr, 
                    OPEN_EXISTING, 
                    FILE_ATTRIBUTE_NORMAL, 
                    nullptr)
    );
    RETURN_LAST_ERROR_IF(_hInputFile.get() == INVALID_HANDLE_VALUE);
    
    _hOutputFile.reset(
        CreateFileW(OutPipeName.c_str(),
                    GENERIC_WRITE, 
                    0, 
                    nullptr, 
                    OPEN_EXISTING, 
                    FILE_ATTRIBUTE_NORMAL, 
                    nullptr)
    );
    RETURN_LAST_ERROR_IF(_hOutputFile.get() == INVALID_HANDLE_VALUE);

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
HRESULT VtIo::StartIfNeeded()
{
    // If we haven't been set up, do nothing (because there's nothing to start)
    if (!IsUsingVt())
    {
        return S_OK;
    }
    // Hmm. We only have one Renderer implementation, 
    //  but its stored as a IRenderer. 
    //  IRenderer doesn't know about IRenderEngine, so it cant have AddRenderEngine
    // auto g = ServiceLocator::LocateGlobals();
    // static_cast<Renderer*>(g->pRender)->AddRenderEngine(_pVtRenderEngine);

    // _pVtInputThread->Start();

    return S_OK;
}