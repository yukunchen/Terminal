#include "winrt/Microsoft.Graphics.Canvas.UI.Xaml.h"
#include "winrt/Microsoft.Graphics.Canvas.UI.h"
#pragma once

class TerminalCanvasView
{
public:
    TerminalCanvasView(winrt::Microsoft::Graphics::Canvas::UI::Xaml::CanvasControl& canvasControl, std::wstring typefaceName, float fontSize);
    ~TerminalCanvasView();

    void Initialize();
    void Invalidate();
    COORD PixelsToChars(float width, float height);
    void PrepDrawingSession(winrt::Microsoft::Graphics::Canvas::CanvasDrawingSession& drawingSession);
    void FinishDrawingSession();
    winrt::Windows::Foundation::Numerics::float2 GetGlyphSize();

private:
    winrt::Microsoft::Graphics::Canvas::UI::Xaml::CanvasControl &_canvasControl;
    std::wstring _typefaceName;
    float _fontSize;
    bool _initialized;
    winrt::Windows::Foundation::Numerics::float2 _glyphSize;
    float _pixelsPerDip;
    winrt::Microsoft::Graphics::Canvas::Text::CanvasTextFormat _textFormat;

    winrt::Microsoft::Graphics::Canvas::CanvasDrawingSession* _pDrawingSession;

    void _SetupTypeface();
};

