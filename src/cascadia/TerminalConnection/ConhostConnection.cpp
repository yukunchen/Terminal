/********************************************************
 *                                                       *
 *   Copyright (C) Microsoft. All rights reserved.       *
 *                                                       *
 ********************************************************/

#include "pch.h"
#include "ConhostConnection.h"
#include "windows.h"
#include <sstream>
// STARTF_USESTDHANDLES is only defined in WINAPI_PARTITION_DESKTOP
// We're just gonna yolo it for this test code
#ifndef STARTF_USESTDHANDLES
#define STARTF_USESTDHANDLES       0x00000100
#endif

#include <conpty-universal.h>

namespace winrt::TerminalConnection::implementation
{
    ConhostConnection::ConhostConnection(hstring const& commandline,
                                        uint32_t initialRows,
                                        uint32_t initialCols) :
        _connected{ false },
        _inPipe{ INVALID_HANDLE_VALUE },
        _outPipe{ INVALID_HANDLE_VALUE },
        _signalPipe{ INVALID_HANDLE_VALUE },
        _outputThreadId{ 0 },
        _hOutputThread{ INVALID_HANDLE_VALUE },
        _piConhost{ 0 }
    {
        _commandline = commandline;
        _initialRows = initialRows;
        _initialCols = initialCols;

    }

    winrt::event_token ConhostConnection::TerminalOutput(TerminalConnection::TerminalOutputEventArgs const& handler)
    {
        return _outputHandlers.add(handler);
    }

    void ConhostConnection::TerminalOutput(winrt::event_token const& token) noexcept
    {
        _outputHandlers.remove(token);
    }

    winrt::event_token ConhostConnection::TerminalDisconnected(TerminalConnection::TerminalDisconnectedEventArgs const& handler)
    {
        return _disconnectHandlers.add(handler);
    }

    void ConhostConnection::TerminalDisconnected(winrt::event_token const& token) noexcept
    {
        _disconnectHandlers.remove(token);
    }

    void ConhostConnection::Start()
    {
        std::wstring cmdline = _commandline.c_str();

        CreateConPty(cmdline,
                     static_cast<short>(_initialCols),
                     static_cast<short>(_initialRows),
                     &_inPipe,
                     &_outPipe,
                     &_signalPipe,
                     &_piConhost);

        _connected = true;

        // Create our own output handling thread
        // Each console needs to make sure to drain the output from it's backing host.
        _outputThreadId = (DWORD)-1;
        _hOutputThread = CreateThread(nullptr,
                                      0,
                                      (LPTHREAD_START_ROUTINE)StaticOutputThreadProc,
                                      this,
                                      0,
                                      &_outputThreadId);
    }

    void ConhostConnection::WriteInput(hstring const& data)
    {
        std::string str = winrt::to_string(data);
        bool fSuccess = !!WriteFile(_inPipe, str.c_str(), (DWORD)str.length(), nullptr, nullptr);
        fSuccess;
    }

    void ConhostConnection::Resize(uint32_t rows, uint32_t columns)
    {
        if (!_connected)
        {
            _initialRows = rows;
            _initialCols = columns;
        }
        else
        {
            SignalResizeWindow(_signalPipe, static_cast<unsigned short>(columns), static_cast<unsigned short>(rows));
        }
    }

    void ConhostConnection::Close()
    {
        if (!_connected) return;
        // TODO:
        //      terminate the output thread
        //      Close our handles
        //      Close the Pseudoconsole
        //      terminate our processes
        CloseHandle(_inPipe);
        CloseHandle(_outPipe);
        // What? CreateThread is in app partition but TerminateThread isn't?
        //TerminateThread(_hOutputThread, 0);
        TerminateProcess(_piConhost.hProcess, 0);
        CloseHandle(_piConhost.hProcess);
        //throw hresult_not_implemented();
    }

    DWORD ConhostConnection::StaticOutputThreadProc(LPVOID lpParameter)
    {
        ConhostConnection* const pInstance = (ConhostConnection*)lpParameter;
        return pInstance->_OutputThread();
    }

    DWORD ConhostConnection::_OutputThread()
    {
        const size_t bufferSize = 4096;
        BYTE buffer[bufferSize];
        DWORD dwRead;
        while (true)
        {
            dwRead = 0;
            bool fSuccess = false;

            fSuccess = !!ReadFile(_outPipe, buffer, bufferSize, &dwRead, nullptr);
            if (!fSuccess) throw;
            if (dwRead == 0) continue;
            // Convert buffer to hstring
            char* pchStr = (char*)(buffer);
            std::string str{pchStr, dwRead};
            auto hstr = winrt::to_hstring(str);

            // Pass the output to our registered event handlers
            _outputHandlers(hstr);
        }
    }
}
