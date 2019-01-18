/********************************************************
 *                                                       *
 *   Copyright (C) Microsoft. All rights reserved.       *
 *                                                       *
 ********************************************************/

#include "pch.h"
#include "CanvasViewRenderThread.hpp"

CanvasViewRenderThread::CanvasViewRenderThread(TerminalCanvasView& canvasView) :
    _canvasView(canvasView)
{
}

void CanvasViewRenderThread::NotifyPaint()
{
    _canvasView.Invalidate();
}

void CanvasViewRenderThread::EnablePainting()
{
    //todo
    _canvasView.Invalidate();
}

void CanvasViewRenderThread::WaitForPaintCompletionAndDisable(DWORD /*dwTimeoutMs*/)
{
    //todo
    _canvasView.Invalidate();
}

