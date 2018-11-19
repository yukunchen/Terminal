#include "precomp.h"
#include "TerminalDispatch.hpp"
using namespace ::Microsoft::Terminal::Core;

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
