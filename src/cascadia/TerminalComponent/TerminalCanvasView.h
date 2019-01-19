/********************************************************
 *                                                       *
 *   Copyright (C) Microsoft. All rights reserved.       *
 *                                                       *
 ********************************************************/

#include "winrt/Microsoft.Graphics.Canvas.UI.Xaml.h"
#include "winrt/Microsoft.Graphics.Canvas.UI.h"
#pragma once

class TerminalCanvasView
{
public:
    TerminalCanvasView(::winrt::Windows::UI::Xaml::Controls::SwapChainPanel canvasControl, std::wstring typefaceName, float fontSize);
    ~TerminalCanvasView();

    void Initialize();
    void Invalidate();
    COORD PixelsToChars(float width, float height);
    void PrepDrawingSession(winrt::Microsoft::Graphics::Canvas::CanvasDrawingSession& drawingSession);
    void FinishDrawingSession();
    winrt::Windows::Foundation::Numerics::float2 GetGlyphSize();

    void PaintRun(std::wstring_view chars, COORD origin, winrt::Windows::UI::Color fg, winrt::Windows::UI::Color bg);

private:
    ::winrt::Windows::UI::Xaml::Controls::SwapChainPanel _canvasControl;
    std::wstring _typefaceName;
    float _fontSize;
    bool _initialized;
    winrt::Windows::Foundation::Numerics::float2 _glyphSize;
    float _pixelsPerDip;
    winrt::Microsoft::Graphics::Canvas::Text::CanvasTextFormat _textFormat;

    winrt::Microsoft::Graphics::Canvas::CanvasDrawingSession* _pDrawingSession;

    void _SetupTypeface();
};

