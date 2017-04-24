/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "ConIoSrv.h"

#include "Display.h"
#include "Keyboard.h"
#include "Server.h"
#include "xlate.h"

PCIS_CONTEXT g_CisContext;

NTSTATUS
ServerDllInitialization(
    PCSR_SERVER_DLL pSrvDll
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Initialize the CSRSS Server DLL descriptor structure.
    //
    // N.B.: We merely use CSRSS as a privileged host; we do not expose any
    //       API's. The idea is to move this server DLL into a separate process
    //       once we figure out how to launch such a process with the required
    //       privileges and watchdog mechanism to ensure that it is relaunched
    //       if it ever dies (see below).
    //
    // TODO: MSFT: 11353646 - Move ConIoSrv from CSRSS into separate process.
    //

    pSrvDll->MaxApiNumber        = 0;
    pSrvDll->ApiDispatchTable    = NULL;
    pSrvDll->ApiServerValidTable = NULL;
#if DBG
    pSrvDll->ApiNameTable        = NULL;
#endif

    //
    // Initialize the keyboard layout.
    //

    BOOL Ret = XlateInit();
    if (Ret)
    {
        //
        // Initialize the global structures needed for the rest of the
        // initialization.
        //

        Status = CisInitializeServerGlobals();

        if (NT_SUCCESS(Status))
        {
            //
            // Initialize keyboards in the system and begin listening for
            // input. Also, register for keyboard arrival and removal
            // notifications.
            //
            // N.B.: This method merely spawns a thread to perform the
            //       initialization, which then hangs around waiting for input.
            //       The reason why the initialization is performed on a
            //       separate thread as well as the listening for input is that
            //       listening for input is done via APC callbacks and the
            //       thread that sets it up is the one that receives those
            //       callbacks.
            //

            Status = KbdInitializeKeyboardInputAsync();

            if (NT_SUCCESS(Status))
            {
                //
                // Initialize the display by connecting to the console driver
                // display object and retrieve the display size.
                //

                Status = DspInitializeDisplay();

                if (NT_SUCCESS(Status))
                {
                    //
                    // Create the ALPC API port for conhost instances to make
                    // requests, and spawn a thread to service those requests.
                    //

                    Status = CisInitializeServer();
                }
            }
        }
    }

    return Status;
}
