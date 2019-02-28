#include "pch.h"
#include "Utils.h"

// using namespace ::Microsoft::Terminal::Core;
using namespace winrt::Microsoft::Terminal::TerminalControl;

//void ::Microsoft::Terminal::TerminalControl::SetFromControlSettings(TerminalSettings terminalSettings,
//                      Settings& coreSettings)
//{
//    // TODO Color Table
//    coreSettings.DefaultForeground(terminalSettings.DefaultForeground());
//    coreSettings.DefaultBackground(terminalSettings.DefaultBackground());
//
//    for (int i = 0; i < coreSettings.GetColorTable().size(); i++)
//    {
//        coreSettings.SetColorTableEntry(i, terminalSettings.GetColorTableEntry(i));
//    }
//
//
//    coreSettings.HistorySize(terminalSettings.HistorySize());
//    coreSettings.InitialRows(terminalSettings.InitialRows());
//    coreSettings.InitialCols(terminalSettings.InitialCols());
//    coreSettings.SnapOnInput(terminalSettings.SnapOnInput());
//}

// void ::Microsoft::Terminal::TerminalControl::SetFromCoreSettings(Settings& coreSettings,
//                          TerminalSettings terminalSettings)
// {
//     // TODO Color Table
//     terminalSettings.DefaultForeground(coreSettings.DefaultForeground());
//     terminalSettings.DefaultBackground(coreSettings.DefaultBackground());
//     terminalSettings.HistorySize(coreSettings.HistorySize());
//     terminalSettings.InitialRows(coreSettings.InitialRows());
//     terminalSettings.InitialCols(coreSettings.InitialCols());
//     terminalSettings.SnapOnInput(coreSettings.SnapOnInput());
// }
