/********************************************************
 *                                                       *
 *   Copyright (C) Microsoft. All rights reserved.       *
 *                                                       *
 ********************************************************/

#include "precomp.h"
#include "TerminalDispatch.hpp"
using namespace ::Microsoft::Terminal::Core;
using namespace ::Microsoft::Console::VirtualTerminal;

// NOTE:
// Functions related to Set Graphics Renditions (SGR) are in
//      TerminalDispatchGraphics.cpp, not this file

TerminalDispatch::TerminalDispatch(ITerminalApi& terminalApi) :
    _terminalApi{ terminalApi }
{

}

void TerminalDispatch::Execute(const wchar_t wchControl)
{
    _terminalApi.ExecuteChar(wchControl);
}

void TerminalDispatch::Print(const wchar_t wchPrintable)
{
    _terminalApi.PrintString({ &wchPrintable, 1 });
}

void TerminalDispatch::PrintString(const wchar_t *const rgwch, const size_t cch)
{
    _terminalApi.PrintString({ rgwch, cch });
}

bool TerminalDispatch::CursorPosition(const unsigned int uiLine,
                                      const unsigned int uiColumn)
{
    short x = static_cast<short>(uiColumn - 1);
    short y = static_cast<short>(uiLine - 1);
    return _terminalApi.SetCursorPosition(x, y);
}
