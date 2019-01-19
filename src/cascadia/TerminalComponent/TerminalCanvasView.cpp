/********************************************************
 *                                                       *
 *   Copyright (C) Microsoft. All rights reserved.       *
 *                                                       *
 ********************************************************/

#include "pch.h"
#include "TerminalCanvasView.h"

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Foundation::Numerics;

using namespace winrt::Windows::UI::Xaml::Controls;

using namespace winrt::Microsoft::Graphics::Canvas;
using namespace winrt::Microsoft::Graphics::Canvas::Text;
using namespace winrt::Microsoft::Graphics::Canvas::UI::Xaml;

TerminalCanvasView::TerminalCanvasView(SwapChainPanel canvasControl, std::wstring typefaceName, float fontSize) :
    _canvasControl { canvasControl },
    _typefaceName { typefaceName },
    _fontSize { fontSize },
    _initialized { false },
    _glyphSize{ 0.f, 0.f },
    _pixelsPerDip{ 96.0f },
    _pDrawingSession{ nullptr }
{
}


TerminalCanvasView::~TerminalCanvasView()
{
}

void TerminalCanvasView::Initialize()
{
    _initialized = true;
    _SetupTypeface();
}

void TerminalCanvasView::Invalidate()
{
    /*_canvasControl.Invalidate();*/
}

COORD TerminalCanvasView::PixelsToChars(float width, float height)
{
    FAIL_FAST_IF(!_initialized); // need to get setup during a CanvasControl.CreateResources first
    COORD result{};
    result.X = static_cast<SHORT>(width / _glyphSize.x);
    result.Y = static_cast<SHORT>(height / _glyphSize.y);
    return result;
}

void TerminalCanvasView::PrepDrawingSession(CanvasDrawingSession& drawingSession)
{
    _pDrawingSession = &drawingSession;
}

void TerminalCanvasView::FinishDrawingSession()
{
    _pDrawingSession = nullptr;
}

winrt::Windows::Foundation::Numerics::float2 TerminalCanvasView::GetGlyphSize()
{
    FAIL_FAST_IF(!_initialized); // need to get setup during a CanvasControl.CreateResources first

    /*CanvasTextLayout layout(_canvasControl, L"g", _textFormat, 0, 0);
    auto lb = layout.LayoutBounds();

    return winrt::Windows::Foundation::Numerics::float2(lb.Width, lb.Height);*/
    return winrt::Windows::Foundation::Numerics::float2(0, 0);
}

// Must be called during a frame or during a CreateResources call
void TerminalCanvasView::_SetupTypeface()
{
    _textFormat = CanvasTextFormat();
    _textFormat.FontFamily(_typefaceName);
    _textFormat.FontSize(_fontSize);
    _textFormat.WordWrapping(CanvasWordWrapping::NoWrap);
    _glyphSize = GetGlyphSize();
}

void TerminalCanvasView::PaintRun(std::wstring_view chars,
                                  COORD origin,
                                  winrt::Windows::UI::Color fg,
                                  winrt::Windows::UI::Color bg)
{
    auto glyphWidth = _glyphSize.x;
    auto glyphHeight = _glyphSize.y;

    auto textOriginX = origin.X * glyphWidth;
    auto textOriginY = origin.Y * glyphHeight;

    winrt::hstring hstr(chars);

    CanvasTextLayout textLayout(*_pDrawingSession, hstr, _textFormat, textOriginX, textOriginY);
    auto lb = textLayout.LayoutBounds();
    auto realWidth = lb.Width;
    auto realHeight = lb.Height;

    // _pDrawingSession->FillRectangle(textOriginX,
    //                                 textOriginY,
    //                                 glyphWidth * chars.size(),
    //                                 glyphHeight,
    //                                 bg);
    _pDrawingSession->FillRectangle(textOriginX,
                                    textOriginY,
                                    realWidth,
                                    realHeight,
                                    bg);
    _pDrawingSession->DrawText(chars, textOriginX, textOriginY, fg, _textFormat);
}
