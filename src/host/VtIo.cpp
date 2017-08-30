/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "VtIo.hpp"
#include "..\interactivity\inc\ServiceLocator.hpp"
#include "..\renderer\vt\vtrenderer.hpp"
#include "..\renderer\base\renderer.hpp"

using namespace Microsoft::Console::VirtualTerminal;

VtIo::VtIo()
{
    _usingVt = false;
}

VtIo::~VtIo()
{
    
}

HRESULT VtIo::Initialize(const std::wstring& InPipeName, const std::wstring& OutPipeName)
{
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
    _pVtRenderEngine = new VtEngine(_hOutputFile.release());

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