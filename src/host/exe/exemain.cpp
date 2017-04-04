/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "..\server\Entrypoints.h"

// Routine Description:
// - Main entry point for EXE version of console launching.
//   This can be used as a debugging/diagnostics tool as well as a method of testing the console without
//   replacing the system binary.
// Arguments:
// - hInstance, hPrevInstance - Unused pointers to the module instances. See wWinMain definitions @ MSDN for more details.
// - pwszCmdLine - If passed, the command line arguments specify which process to start up and the arguments to give to it.
//               - If not passed, we will run cmd.exe with no arguments on your behalf.
// - nCmdShow - Unused variable specifying window show/hide state for Win32 mode applications.
// Return value:
// - [[noreturn]] - This function will not return. It will kill the thread we were called from and the console server threads will take over.
int CALLBACK wWinMain(
    _In_ HINSTANCE /*hInstance*/,
    _In_ HINSTANCE /*hPrevInstance*/,
    _In_ PWSTR pwszCmdLine,
    _In_ int /*nCmdShow*/)
{
    Entrypoints::StartConsoleForCmdLine(pwszCmdLine);
}
