/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "tracing.hpp"

TRACELOGGING_DEFINE_PROVIDER(g_hConsoleVtRendererTraceProvider,
    "Microsoft.Windows.Console.Render.VtEngine",
    // tl:{c9ba2a95-d3ca-5e19-2bd6-776a0910cb9d}
    (0xc9ba2a95, 0xd3ca, 0x5e19, 0x2b, 0xd6, 0x77, 0x6a, 0x09, 0x10, 0xcb, 0x9d),
    TraceLoggingOptionMicrosoftTelemetry());

using namespace Microsoft::Console::VirtualTerminal;

RenderTracing::RenderTracing()
{
    TraceLoggingRegister(g_hConsoleVtRendererTraceProvider);
}

RenderTracing::~RenderTracing()
{
    TraceLoggingUnregister(g_hConsoleVtRendererTraceProvider);
}

// Function Description:
// - Convert the string to only have printable characters in it. Control
//      characters are converted to hat notation, spaces are converted to "SPC"
//      (to be able to see them at the end of a string), and DEL is written as
//      "\x7f".
// Arguments:
// - inString: The string to convert
// Return Value:
// - a string with only printable characters in it.
std::string toPrintableString(const std::string& inString)
{
    std::string retval = "";
    for (size_t i = 0; i < inString.length(); i++)
    {
        unsigned char c = inString[i];
        if (c < '\x20' && c < '\x7f')
        {
            retval += "^";
            char actual = (c + 0x40);
            retval += std::string(1, actual);
        }
        else if (c == '\x7f')
        {
            retval += "\\x7f";
        }
        else if (c == '\x20')
        {
            retval += "SPC";
        }
        else
        {
            retval += std::string(1, c);
        }
    }
    return retval;
}
void RenderTracing::TraceString(const std::string& instr) const
{
    const std::string _seq = toPrintableString(instr);
    const char* const seq = _seq.c_str();
    TraceLoggingWrite(g_hConsoleVtRendererTraceProvider,
                      "VtEngine_TraceString",
                      TraceLoggingString(seq),
                      TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));
}
