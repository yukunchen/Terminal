#pragma once
namespace Microsoft::Console::Render
{
    class IRenderThread
    {
    public:
        virtual ~IRenderThread() {}
        virtual void NotifyPaint() = 0;
        virtual void EnablePainting() = 0;
        virtual void WaitForPaintCompletionAndDisable(DWORD dwTimeoutMs) = 0;
    };
}
