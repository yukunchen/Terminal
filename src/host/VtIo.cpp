/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "VtIo.hpp"
#include "..\interactivity\inc\ServiceLocator.hpp"
// #include "..\renderer\vt\vtrenderer.hpp"
#include "..\renderer\vt\XtermEngine.hpp"
#include "..\renderer\vt\Xterm256Engine.hpp"
#include "..\renderer\vt\WinTelnetEngine.hpp"
#include "..\renderer\base\renderer.hpp"

using namespace Microsoft::Console::VirtualTerminal;

VtIo::VtIo()
{
    _usingVt = false;
}

VtIo::~VtIo()
{
    
}

HRESULT VtIo::ParseIoMode(const std::wstring& VtMode, VtIoMode* const IoMode)
{
    if (IoMode == nullptr) return E_INVALIDARG;
    if (VtMode == XTERM_256_STRING)
    {
        *IoMode = VtIoMode::XTERM_256;
    }
    else if (VtMode == XTERM_STRING)
    {
        *IoMode = VtIoMode::XTERM;
    }
    else if (VtMode == WIN_TELNET_STRING)
    {
        *IoMode = VtIoMode::WIN_TELNET;
    }
    else if (VtMode == DEFAULT_STRING)
    {
        *IoMode = VtIoMode::XTERM_256;
    }
    else
    {
        return E_INVALIDARG;
    }
    return S_OK;
}

HRESULT VtIo::Initialize(const std::wstring& InPipeName, const std::wstring& OutPipeName, const std::wstring& VtMode)
{
    CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();

    HRESULT hr = ParseIoMode(VtMode, &_IoMode);
    if (!SUCCEEDED(hr)) return hr;

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
    THROW_IF_HANDLE_INVALID(_hInputFile.get());
    
    _hOutputFile.reset(
        CreateFileW(OutPipeName.c_str(),
                    GENERIC_WRITE, 
                    0, 
                    nullptr, 
                    OPEN_EXISTING, 
                    FILE_ATTRIBUTE_NORMAL, 
                    nullptr)
    );
    THROW_IF_HANDLE_INVALID(_hOutputFile.get());

    _pVtInputThread = new VtInputThread(_hInputFile.release());

    switch(_IoMode)
    {
        case VtIoMode::XTERM_256:
            _pVtRenderEngine = new Xterm256Engine(_hOutputFile.release());
            break;
        case VtIoMode::XTERM:
            _pVtRenderEngine = new XtermEngine(_hOutputFile.release(), gci->GetColorTable(), (WORD)gci->GetColorTableSize());
            break;
        case VtIoMode::WIN_TELNET:
            _pVtRenderEngine = new WinTelnetEngine(_hOutputFile.release(), gci->GetColorTable(), (WORD)gci->GetColorTableSize());
            break;
        default:
            return E_FAIL;
    }

    _usingVt = true;
    return S_OK;
}

bool VtIo::IsUsingVt()
{
    return _usingVt;
}

HRESULT VtIo::Start()
{
    // Hmm. We only have one Renderer implementation, 
    //  but its stored as a IRenderer. 
    //  IRenderer doesn't know about IRenderEngine. 
    //todo?
    auto g = ServiceLocator::LocateGlobals();
    static_cast<Renderer*>(g->pRender)->AddRenderEngine(_pVtRenderEngine);

    _pVtInputThread->Start();

    return S_OK;
}