#pragma once
#include "TerminalCanvasView.h"
#include "../../renderer/inc/IRenderThread.hpp"

class CanvasViewRenderThread : public ::Microsoft::Console::Render::IRenderThread
{
public:
    CanvasViewRenderThread(TerminalCanvasView& canvasView);
    void NotifyPaint() override;
    void EnablePainting() override;
    void WaitForPaintCompletionAndDisable(DWORD dwTimeoutMs) override;
private:
    TerminalCanvasView& _canvasView;
};
