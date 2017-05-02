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
#include "..\..\host\renderData.hpp"
#include "..\..\renderer\base\renderer.hpp"
#include "..\..\renderer\wddmcon\wddmconrenderer.hpp"

#include "..\inc\ServiceLocator.hpp"

using namespace Microsoft::Console::Interactivity::OneCore;

NTSTATUS InitializeBgfx()
{
    NTSTATUS Status;

    Globals * const Globals = ServiceLocator::LocateGlobals();
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
                Globals->pRenderEngine = pBgfxEngine;
            }
        }
    }

    return Status;
}

NTSTATUS InitializeWddmCon()
{
    NTSTATUS Status;

    Globals * const Globals = ServiceLocator::LocateGlobals();

    WddmConEngine * const pWddmConEngine = new WddmConEngine();
    Status = NT_TESTNULL(pWddmConEngine);

    if (NT_SUCCESS(Status))
    {
        Globals->pRenderEngine = pWddmConEngine;
    }

    return Status;
}

DWORD ConsoleInputThreadProcOneCore(LPVOID lpParam)
{
    UNREFERENCED_PARAMETER(lpParam);

    NTSTATUS Status;

    Globals * const Globals = ServiceLocator::LocateGlobals();

    ConIoSrvComm * const Server = ServiceLocator::LocateInputServices<ConIoSrvComm>();
    Status = Server->Connect();

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
                    // Create and set the render data.
                    Globals->pRenderData = new RenderData();
                    Status = NT_TESTNULL(Globals->pRenderData);

                    if (NT_SUCCESS(Status))
                    {
                        // Create and set the renderer.
                        Renderer* pNewRenderer = nullptr;
                        Renderer::s_CreateInstance(Globals->pRenderData,
                                                   Globals->pRenderEngine,
                                                   &pNewRenderer);
                        Status = NT_TESTNULL(pNewRenderer);

                        if (NT_SUCCESS(Status))
                        {
                            Globals->pRender = pNewRenderer;

                            // Start listening for input (returns on failure only).
                            Status = Server->ServiceInputPipe();
                        }
                    }
                }
            }
        }
        else
        {
            // Nothing to do input-wise, but we must let the rest of the console
            // continue.
            Server->SignalInputEventIfNecessary();
        }
    }

    // General error case.
    if (!NT_SUCCESS(Status))
    {
        // If anything went wrong, there is no point in continuing.
        TerminateProcess(GetCurrentProcess(), Status);
    }

    return 0;
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
