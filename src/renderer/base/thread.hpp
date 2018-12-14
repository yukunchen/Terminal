/*++
Copyright (c) Microsoft Corporation

Module Name:
- Thread.hpp

Abstract:
- This is the definition of our rendering thread designed to throttle and compartmentalize drawing operations.

Author(s):
- Michael Niksa (MiNiksa) Feb 2016
--*/

#pragma once

#include "..\inc\IRenderer.hpp"
#include "..\inc\IRenderThread.hpp"

namespace Microsoft::Console::Render
{
    class RenderThread sealed : public IRenderThread
    {
    public:
        static HRESULT s_CreateInstance(_In_ IRenderer* const pRendererParent, _Outptr_ RenderThread** const ppRenderThread);

        void NotifyPaint() override;

        void EnablePainting() override;
        void WaitForPaintCompletionAndDisable(DWORD dwTimeoutMs) override;

        ~RenderThread();

    private:
        static DWORD WINAPI s_ThreadProc(_In_ LPVOID lpParameter);
        DWORD WINAPI _ThreadProc();

        static DWORD const s_FrameLimitMilliseconds = 8;

        RenderThread(_In_ IRenderer* const pRenderer);

        HANDLE _hThread;
        HANDLE _hEvent;

        HANDLE _hPaintEnabledEvent;
        HANDLE _hPaintCompletedEvent;

        IRenderer* const _pRenderer;

        bool _fKeepRunning;
    };
}
