/********************************************************
 *                                                       *
 *   Copyright (C) Microsoft. All rights reserved.       *
 *                                                       *
 ********************************************************/

#pragma once

#include "ConhostConnection.g.h"

namespace winrt::Microsoft::Terminal::TerminalConnection::implementation
{
    struct ConhostConnection : ConhostConnectionT<ConhostConnection>
    {
        ConhostConnection(const hstring& cmdline, uint32_t rows, uint32_t cols);

        winrt::event_token TerminalOutput(TerminalConnection::TerminalOutputEventArgs const& handler);
        void TerminalOutput(winrt::event_token const& token) noexcept;
        winrt::event_token TerminalDisconnected(TerminalConnection::TerminalDisconnectedEventArgs const& handler);
        void TerminalDisconnected(winrt::event_token const& token) noexcept;
        void Start();
        void WriteInput(hstring const& data);
        void Resize(uint32_t rows, uint32_t columns);
        void Close();

        void ShowHost(bool showConsoleWindow) noexcept;

    private:
        winrt::event<TerminalConnection::TerminalOutputEventArgs> _outputHandlers;
        winrt::event<TerminalConnection::TerminalDisconnectedEventArgs> _disconnectHandlers;

        uint32_t _initialRows;
        uint32_t _initialCols;
        hstring _commandline;

        bool _connected;
        HANDLE _inPipe;  // The pipe for writing input to
        HANDLE _outPipe; // The pipe for reading output from
        HANDLE _signalPipe;
        //HPCON _hPC;
        DWORD _outputThreadId;
        HANDLE _hOutputThread;
        PROCESS_INFORMATION _piConhost;
        bool _closing;

        bool _showConsoleWindow;

        static DWORD StaticOutputThreadProc(LPVOID lpParameter);
        DWORD _OutputThread();
    };
}

namespace winrt::Microsoft::Terminal::TerminalConnection::factory_implementation
{
    struct ConhostConnection : ConhostConnectionT<ConhostConnection, implementation::ConhostConnection>
    {
    };
}
