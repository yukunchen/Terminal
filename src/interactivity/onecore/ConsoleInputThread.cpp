/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "ConsoleInputThread.hpp"
#include "BgfxEngine.hpp"
#include "InputServer.hpp"
#include "ConsoleWindow.hpp"

#include "..\..\host\input.h"
#include "..\..\host\renderData.hpp"
#include "..\..\renderer\base\renderer.hpp"
#include "ConIoSrv.h"

#include "..\inc\ServiceLocator.hpp"

using namespace Microsoft::Console::Interactivity::OneCore;

DWORD ConsoleInputThreadProcOneCore(LPVOID lpParam)
{
    UNREFERENCED_PARAMETER(lpParam);

    NTSTATUS Status;

    Globals *Globals = ServiceLocator::LocateGlobals();
    IWindowMetrics *Metrics = ServiceLocator::LocateWindowMetrics();
    InputServer *Server = ServiceLocator::LocateInputServices<InputServer>();

    Status = Server->Connect();

    if (NT_SUCCESS(Status))
    {
        Globals->hInstance = GetModuleHandle(L"ConhostV2.dll");
        
        // Create and set the console window.
        auto wnd = new ConsoleWindow();
        Status = NT_TESTNULL(wnd);

        if (NT_SUCCESS(Status))
        {
            ServiceLocator::SetConsoleWindowInstance(wnd);

            // Fetch the display size from the console driver.
            RECT DisplaySize = Metrics->GetMaxClientRectInPixels();
            
            // Same with the font size.
            CD_IO_FONT_SIZE FontSize = { 0 };
            Status = Server->RequestGetFontSize(&FontSize);

            if (NT_SUCCESS(Status))
            {
                // Create and set the render engine.
                BgfxEngine* pBgfxEngine = new BgfxEngine(Server->GetSharedViewBase(),
                                                         DisplaySize.bottom,
                                                         DisplaySize.right,
                                                         FontSize.Width,
                                                         FontSize.Height);
                Status = NT_TESTNULL(pBgfxEngine);

                if (NT_SUCCESS(Status))
                {
                    Globals->pRenderEngine = pBgfxEngine;

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

                            // Singal that input is ready to go.
                            Globals->hConsoleInputInitEvent.SetEvent();

                            // Start listening for input (returns on failure only).
                            Status = Server->ServiceInputPipe();
                        }
                    }
                }
            }
        }
    }

    // General error case.
    if (!NT_SUCCESS(Status))
    {
        // If we can't connect, there is no point in hanging around.
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
                           _inputServer,
                           0,
                           &dwThreadId);
    if (hThread)
    {
        _hThread = hThread;
        _dwThreadId = dwThreadId;
    }

    return hThread;
}

InputServer *ConsoleInputThread::GetInputServer()
{
    return _inputServer;
}
