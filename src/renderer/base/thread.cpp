/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "thread.hpp"

#pragma hdrstop

using namespace Microsoft::Console::Render;

RenderThread::RenderThread(_In_ IRenderer* const pRenderer) : 
    _pRenderer(pRenderer),
    _hThread(INVALID_HANDLE_VALUE),
    _hEvent(INVALID_HANDLE_VALUE),
    _fKeepRunning(true)
{

}

RenderThread::~RenderThread()
{
    if (_hThread != INVALID_HANDLE_VALUE)
    {
        _fKeepRunning = false; // stop loop after final run
        SignalObjectAndWait(_hEvent, _hThread, INFINITE, FALSE); // signal final paint and wait for thread to finish.

        CloseHandle(_hThread);
        _hThread = INVALID_HANDLE_VALUE;
    }

    if (_hEvent != INVALID_HANDLE_VALUE)
    {
        CloseHandle(_hEvent);
        _hEvent = INVALID_HANDLE_VALUE;
    }
}

HRESULT RenderThread::s_CreateInstance(_In_ IRenderer* const pRendererParent, 
                                       _Outptr_ RenderThread** const ppRenderThread)
{
    RenderThread* pNewThread = new RenderThread(pRendererParent);

    HRESULT hr = (pNewThread == nullptr) ? E_OUTOFMEMORY : S_OK;

    // Create event before thread as thread will start immediately.
    if (SUCCEEDED(hr))
    {
        HANDLE hEvent = CreateEventW(nullptr, // non-inheritable security attributes
                                     FALSE, // auto reset event
                                     FALSE, // initially unsignaled
                                     nullptr // no name
                                     );

        if (hEvent == nullptr)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
        else
        {
            pNewThread->_hEvent = hEvent;
        }
    }

    if (SUCCEEDED(hr))
    {
        HANDLE hThread = CreateThread(nullptr, // non-inheritable security attributes
                                      0, // use default stack size
                                      s_ThreadProc,
                                      pNewThread,
                                      0, // create immediately
                                      nullptr // we don't need the thread ID
                                      );

        if (hThread == nullptr)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
        else
        {
            pNewThread->_hThread = hThread;
        }
    }

    if (SUCCEEDED(hr))
    {
        *ppRenderThread = pNewThread;
    }
    else if (pNewThread != nullptr)
    {
        delete pNewThread;
    }

    return hr;
}

DWORD WINAPI RenderThread::s_ThreadProc(_In_ LPVOID lpParameter)
{
    RenderThread* const pContext = static_cast<RenderThread*>(lpParameter);

    if (pContext != nullptr)
    {
        return pContext->_ThreadProc();
    }
    else
    {
        return (DWORD)E_INVALIDARG;
    }
}

extern void LockConsole();
extern void UnlockConsole();

DWORD WINAPI RenderThread::_ThreadProc()
{
    while (_fKeepRunning)
    {
        WaitForSingleObject(_hEvent, INFINITE);

        LockConsole();
        LOG_IF_FAILED(_pRenderer->PaintFrame());
        UnlockConsole();

        // extra check before we sleep since it's a "long" activity, relatively speaking.
        if (_fKeepRunning)
        {
            Sleep(s_FrameLimitMilliseconds);
        }
    }

    return S_OK;
}

void RenderThread::NotifyPaint() const
{
    SetEvent(_hEvent);
}
