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

namespace Microsoft
{
    namespace Console
    {
        namespace Render
        {
            class RenderThread sealed 
            {
            public:
                static HRESULT s_CreateInstance(_In_ IRenderer* const pRendererParent, _Outptr_ RenderThread** const ppRenderThread);

                void NotifyPaint() const;

                ~RenderThread();

            private:
                static DWORD WINAPI s_ThreadProc(_In_ LPVOID lpParameter);
                DWORD WINAPI _ThreadProc();

                static DWORD const s_FrameLimitMilliseconds = 8;

                RenderThread(_In_ IRenderer* const pRenderer);

                HANDLE _hThread;
                HANDLE _hEvent;

                IRenderer* const _pRenderer;

                bool _fKeepRunning;
            };
        };
    };
};