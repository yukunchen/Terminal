/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include <precomp.h>

#include "echoDispatch.hpp"

using namespace Microsoft::Console::VirtualTerminal;

void EchoDispatch::Print(_In_ wchar_t const wchPrintable)
{
    wprintf(L"Print: %c (0x%x)\r\n", wchPrintable, wchPrintable);
}

void EchoDispatch::PrintString(_In_reads_(cch) wchar_t* const rgwch, _In_ size_t const cch)
{
    wprintf(L"PrintString: \"%s\" (%d chars)\r\n", rgwch, cch);
}

void EchoDispatch::Execute(_In_ wchar_t const wchControl)
{
    wprintf(L"Execute: 0x%x\r\n", wchControl);
}