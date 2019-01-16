/*++
Copyright (c) Microsoft Corporation

Module Name:
- IRenderThread.hpp

Abstract:
- an abstraction for all the actions a render thread needs to perform.

Author(s):
- Mike Griese (migrie) 16 Jan 2019
--*/

#pragma once
namespace Microsoft::Console::Render
{
    class IRenderThread
    {
    public:
        virtual ~IRenderThread() {}
        virtual void NotifyPaint() = 0;
        virtual void EnablePainting() = 0;
        virtual void WaitForPaintCompletionAndDisable(const DWORD dwTimeoutMs) = 0;
    };
}
