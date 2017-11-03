/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "ConsoleInputThread.hpp"
#include "BgfxEngine.hpp"
#include "ConIoSrvComm.hpp"
#include "ConsoleWindow.hpp"

#include "ConIoSrv.h"

#include "..\..\host\input.h"
#include "..\..\renderer\base\renderer.hpp"
#include "..\..\renderer\wddmcon\wddmconrenderer.hpp"

#include "..\inc\ServiceLocator.hpp"

using namespace Microsoft::Console::Interactivity::OneCore;

NTSTATUS InitializeBgfx()
{
    NTSTATUS Status;

    Globals * const Globals = ServiceLocator::LocateGlobals();
    assert(Globals->pRender != nullptr);
    IWindowMetrics * const Metrics = ServiceLocator::LocateWindowMetrics();
    const ConIoSrvComm * const Server = ServiceLocator::LocateInputServices<ConIoSrvComm>();

    // Fetch the display size from the console driver.
    const RECT DisplaySize = Metrics->GetMaxClientRectInPixels();
    Status = GetLastError();

    if (NT_SUCCESS(Status))
    {
        // Same with the font size.
        CD_IO_FONT_SIZE FontSize = { 0 };
        Status = Server->RequestGetFontSize(&FontSize);

        if (NT_SUCCESS(Status))
        {
            // Create and set the render engine.
            BgfxEngine* const pBgfxEngine = new BgfxEngine(Server->GetSharedViewBase(),
                                                           DisplaySize.bottom / FontSize.Height,
                                                           DisplaySize.right / FontSize.Width,
                                                           FontSize.Width,
                                                           FontSize.Height);
            Status = NT_TESTNULL(pBgfxEngine);

            if (NT_SUCCESS(Status))
            {
                try
                {
                    Globals->pRender->AddRenderEngine(pBgfxEngine);
                catch(...)
                {
                    Status = NTSTATUS_FROM_HRESULT(wil::ResultFromCaughtException());
                }
            }
        }
    }

    return Status;
}

NTSTATUS InitializeWddmCon()
{
    NTSTATUS Status;

    Globals * const Globals = ServiceLocator::LocateGlobals();
    assert(Globals->pRender != nullptr);

    WddmConEngine * const pWddmConEngine = new WddmConEngine();
    Status = NT_TESTNULL(pWddmConEngine);

    if (NT_SUCCESS(Status))
    {
        try
        {
            Globals->pRender->AddRenderEngine(pWddmConEngine);
        catch(...)
        {
            Status = NTSTATUS_FROM_HRESULT(wil::ResultFromCaughtException());
        }
    }

    return Status;
}

DWORD ConsoleInputThreadProcOneCore(LPVOID lpParam)
{
    UNREFERENCED_PARAMETER(lpParam);
    
    Globals * const Globals = ServiceLocator::LocateGlobals();
    ConIoSrvComm * const Server = ServiceLocator::LocateInputServices<ConIoSrvComm>();
    
    NTSTATUS Status = Server->Connect();

    if (NT_SUCCESS(Status))
    {
        USHORT DisplayMode;
        Status = Server->RequestGetDisplayMode(&DisplayMode);

        if (NT_SUCCESS(Status) && DisplayMode != CIS_DISPLAY_MODE_NONE)
        {
            // Create and set the console window.
            ConsoleWindow * const wnd = new ConsoleWindow();
            Status = NT_TESTNULL(wnd);

            if (NT_SUCCESS(Status))
            {
                ServiceLocator::SetConsoleWindowInstance(wnd);

                // The console's renderer should be created before we get here.
                ASSERT(Globals->pRender != nullptr);

                switch (DisplayMode)
                {
                case CIS_DISPLAY_MODE_BGFX:
                    Status = InitializeBgfx();
                    break;

                case CIS_DISPLAY_MODE_DIRECTX:
                    Status = InitializeWddmCon();
                    break;
                }

                if (NT_SUCCESS(Status))
                {
                    Globals->ntstatusConsoleInputInitStatus = Status;
                    Globals->hConsoleInputInitEvent.SetEvent();

                    // Start listening for input (returns on failure only).
                    // This will never return.
                    Server->ServiceInputPipe();
                }
            }
        }
        else
        {
            // Nothing to do input-wise, but we must let the rest of the console
            // continue.
            Server->CleanupForHeadless(Status);
        }
    }
    else
    {
        // If we get an access denied and couldn't connect to the coniosrv in CSRSS.exe.
        // that's OK. We're likely inside an AppContainer in a TAEF /runas:uap test.
        // We don't want AppContainered things to have access to the hardware devices directly
        // like coniosrv in CSRSS offers, so we "succeeded" and will let the IO thread know it
        // can continue.
        if (STATUS_ACCESS_DENIED == Status)
        {
            Status = STATUS_SUCCESS;
        }
        
        // Notify IO thread of our status.
        Server->CleanupForHeadless(Status);
    }

    return Status;
}

// Routine Description:
// - Starts the OneCore-specific console input thread.
HANDLE ConsoleInputThread::Start()
{
    HANDLE hThread = nullptr;
    DWORD dwThreadId = (DWORD)-1;

    hThread = CreateThread(nullptr,
                           0,
                           (LPTHREAD_START_ROUTINE)ConsoleInputThreadProcOneCore,
                           _pConIoSrvComm,
                           0,
                           &dwThreadId);
    if (hThread)
    {
        _hThread = hThread;
        _dwThreadId = dwThreadId;
    }

    return hThread;
}

ConIoSrvComm *ConsoleInputThread::GetConIoSrvComm()
{
    return _pConIoSrvComm;
}
