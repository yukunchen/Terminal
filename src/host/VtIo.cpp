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
    _hInputFile.reset(INVALID_HANDLE_VALUE);
    _hOutputFile.reset(INVALID_HANDLE_VALUE);
}

VtIo::~VtIo()
{
    
}

HRESULT VtIo::Initialize(const std::wstring& InPipeName, const std::wstring& OutPipeName)
{

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

    // TODO: make the i/o HANDLEs into unique_handle's and release() here
    // The VtIo doesn't need to know the handles for itself.
    // maybe keep the strings around though.
    _pVtInputThread = new VtInputThread(_hInputFile.get());
    _pVtRenderEngine = new VtEngine(_hOutputFile.get());

    return E_NOTIMPL;
}

bool VtIo::IsUsingVt()
{
    return (_hInputFile.get() != INVALID_HANDLE_VALUE) && (_hOutputFile.get() != INVALID_HANDLE_VALUE);
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