#pragma once
#include <winrt/Microsoft.Terminal.TerminalControl.h>
#include "../../cascadia/TerminalCore/Settings.h"

namespace Microsoft::Terminal::TerminalControl
{
    void SetFromControlSettings(winrt::Microsoft::Terminal::TerminalControl::TerminalSettings terminalSettings,
                          ::Microsoft::Terminal::Core::Settings& coreSettings);

    // void SetFromCoreSettings(::Microsoft::Terminal::Core::Settings& coreSettings,
    //                          winrt::Microsoft::Terminal::TerminalControl::TerminalSettings terminalSettings);
}
