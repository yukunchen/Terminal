#include "precomp.h"


#include "CustomTextRenderer.h"

#include <wrl.h>
#include <wrl/client.h>

CustomTextRenderer::CustomTextRenderer() :
    m_refCount(0)
{
}


CustomTextRenderer::~CustomTextRenderer()
{
}

// IUnknown methods
ULONG STDMETHODCALLTYPE CustomTextRenderer::AddRef()
{
    m_refCount++;
    return m_refCount;
}

ULONG STDMETHODCALLTYPE CustomTextRenderer::Release()
{
    m_refCount--;
    LONG newCount = m_refCount;

    if (m_refCount == 0)
        delete this;

    return newCount;
}

HRESULT STDMETHODCALLTYPE CustomTextRenderer::QueryInterface(_In_ REFIID riid,
                                                             _Outptr_ void** ppOutput)
{
    *ppOutput = nullptr;
    HRESULT hr = S_OK;

    if (riid == __uuidof(IDWriteTextRenderer))
    {
        *ppOutput = static_cast<IDWriteTextRenderer*>(this);
        AddRef();
    }
    else if (riid == __uuidof(IDWritePixelSnapping))
    {
        *ppOutput = static_cast<IDWritePixelSnapping*>(this);
        AddRef();
    }
    else if (riid == __uuidof(IUnknown))
    {
        *ppOutput = this;
        AddRef();
    }
    else
    {
        hr = E_NOINTERFACE;
    }
    return hr;
}

// IDWritePixelSnapping methods
HRESULT CustomTextRenderer::IsPixelSnappingDisabled(void * /*clientDrawingContext*/,
                                                    _Out_ BOOL * isDisabled)
{
    *isDisabled = false;
    return S_OK;
}

HRESULT CustomTextRenderer::GetPixelsPerDip(void * clientDrawingContext,
                                            _Out_ FLOAT * pixelsPerDip)
{
    DrawingContext * drawingContext =
        static_cast<DrawingContext *>(clientDrawingContext);

    float dpiX, dpiY;
    drawingContext->renderTarget->GetDpi(&dpiX, &dpiY);
    *pixelsPerDip = dpiX / 96;
    return S_OK;
}

HRESULT CustomTextRenderer::GetCurrentTransform(void * clientDrawingContext,
                                                DWRITE_MATRIX * transform)
{
    DrawingContext * drawingContext =
        static_cast<DrawingContext *>(clientDrawingContext);

    // Matrix structures are defined identically
    drawingContext->renderTarget->GetTransform((D2D1_MATRIX_3X2_F *)transform);
    return S_OK;
}

HRESULT CustomTextRenderer::DrawUnderline(void * clientDrawingContext,
                                          FLOAT baselineOriginX,
                                          FLOAT baselineOriginY,
                                          _In_ const DWRITE_UNDERLINE *
                                          underline,
                                          IUnknown * clientDrawingEffect)

{
    FillRectangle(clientDrawingContext,
                  clientDrawingEffect,
                  baselineOriginX,
                  baselineOriginY + underline->offset,
                  underline->width,
                  underline->thickness,
                  underline->readingDirection,
                  underline->flowDirection);
    return S_OK;
}

HRESULT CustomTextRenderer::DrawStrikethrough(void * clientDrawingContext,
                                              FLOAT baselineOriginX,
                                              FLOAT baselineOriginY,
                                              _In_ const DWRITE_STRIKETHROUGH *
                                              strikethrough,
                                              IUnknown * clientDrawingEffect)
{
    FillRectangle(clientDrawingContext,
                  clientDrawingEffect,
                  baselineOriginX,
                  baselineOriginY + strikethrough->offset,
                  strikethrough->width,
                  strikethrough->thickness,
                  strikethrough->readingDirection,
                  strikethrough->flowDirection);
    return S_OK;
}

void CustomTextRenderer::FillRectangle(void * clientDrawingContext,
                                       IUnknown * clientDrawingEffect,
                                       float x, float y,
                                       float width, float thickness,
                                       DWRITE_READING_DIRECTION /*readingDirection*/,
                                       DWRITE_FLOW_DIRECTION /*flowDirection*/)
{
    DrawingContext * drawingContext =
        static_cast<DrawingContext *>(clientDrawingContext);

    // Get brush
    ID2D1Brush * brush = drawingContext->defaultBrush;

    if (clientDrawingEffect != nullptr)
    {
        brush = static_cast<ID2D1Brush *>(clientDrawingEffect);
    }

    D2D1_RECT_F rect = D2D1::RectF(x, y, x + width, y + thickness);
    drawingContext->renderTarget->FillRectangle(&rect, brush);
}

HRESULT CustomTextRenderer::DrawInlineObject(void * clientDrawingContext,
                                             FLOAT originX,
                                             FLOAT originY,
                                             IDWriteInlineObject * inlineObject,
                                             BOOL isSideways,
                                             BOOL isRightToLeft,
                                             IUnknown * clientDrawingEffect)
{
    /*DrawingContext * drawingContext =
        static_cast<DrawingContext *>(clientDrawingContext);*/

    return inlineObject->Draw(clientDrawingContext,
                              this,
                              originX,
                              originY,
                              isSideways,
                              isRightToLeft,
                              clientDrawingEffect);
}


HRESULT CustomTextRenderer::DrawGlyphRun(
    void                         * clientDrawingContext,
    FLOAT                         baselineOriginX,
    FLOAT                         baselineOriginY,
    DWRITE_MEASURING_MODE         measuringMode,
    const DWRITE_GLYPH_RUN             * glyphRun,
    const DWRITE_GLYPH_RUN_DESCRIPTION * glyphRunDescription,
    IUnknown                     * /*clientDrawingEffect*/
)
{
    // Color glyph rendering sourced from https://github.com/Microsoft/Windows-universal-samples/tree/master/Samples/DWriteColorGlyph

    D2D1_POINT_2F baselineOrigin = D2D1::Point2F(baselineOriginX, baselineOriginY);

    DrawingContext * drawingContext =
        static_cast<DrawingContext *>(clientDrawingContext);

    ::Microsoft::WRL::ComPtr<ID2D1DeviceContext4> d2dContext4;
    RETURN_IF_FAILED(drawingContext->renderTarget->QueryInterface(d2dContext4.GetAddressOf()));

    if (WI_IsFlagSet(drawingContext->options, D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT))
    {
        ::Microsoft::WRL::ComPtr<IDWriteFactory4> dwriteFactory4;
        RETURN_IF_FAILED(drawingContext->dwriteFactory->QueryInterface(dwriteFactory4.GetAddressOf()));

        // The list of glyph image formats this renderer is prepared to support.
        DWRITE_GLYPH_IMAGE_FORMATS supportedFormats =
            DWRITE_GLYPH_IMAGE_FORMATS_TRUETYPE |
            DWRITE_GLYPH_IMAGE_FORMATS_CFF |
            DWRITE_GLYPH_IMAGE_FORMATS_COLR |
            DWRITE_GLYPH_IMAGE_FORMATS_SVG |
            DWRITE_GLYPH_IMAGE_FORMATS_PNG |
            DWRITE_GLYPH_IMAGE_FORMATS_JPEG |
            DWRITE_GLYPH_IMAGE_FORMATS_TIFF |
            DWRITE_GLYPH_IMAGE_FORMATS_PREMULTIPLIED_B8G8R8A8;

        // Determine whether there are any color glyph runs within glyphRun. If
        // there are, glyphRunEnumerator can be used to iterate through them.
        ::Microsoft::WRL::ComPtr<IDWriteColorGlyphRunEnumerator1> glyphRunEnumerator;
        HRESULT hr = dwriteFactory4->TranslateColorGlyphRun(baselineOrigin,
                                                            glyphRun,
                                                            glyphRunDescription,
                                                            supportedFormats,
                                                            measuringMode,
                                                            nullptr,
                                                            0,
                                                            &glyphRunEnumerator);

        if (hr == DWRITE_E_NOCOLOR)
        {
            d2dContext4->DrawGlyphRun(baselineOrigin,
                                      glyphRun,
                                      glyphRunDescription,
                                      drawingContext->defaultBrush,
                                      measuringMode);

        }
        else
        {
            RETURN_IF_FAILED(hr);

            ::Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> tempBrush;

            // Complex case: the run has one or more color runs within it. Iterate
            // over the sub-runs and draw them, depending on their format.
            for (;;)
            {
                BOOL haveRun;
                RETURN_IF_FAILED(glyphRunEnumerator->MoveNext(&haveRun));
                if (!haveRun)
                    break;

                DWRITE_COLOR_GLYPH_RUN1 const* colorRun;
                RETURN_IF_FAILED(glyphRunEnumerator->GetCurrentRun(&colorRun));

                D2D1_POINT_2F currentBaselineOrigin = D2D1::Point2F(colorRun->baselineOriginX, colorRun->baselineOriginY);

                switch (colorRun->glyphImageFormat)
                {
                case DWRITE_GLYPH_IMAGE_FORMATS_PNG:
                case DWRITE_GLYPH_IMAGE_FORMATS_JPEG:
                case DWRITE_GLYPH_IMAGE_FORMATS_TIFF:
                case DWRITE_GLYPH_IMAGE_FORMATS_PREMULTIPLIED_B8G8R8A8:
                {
                    // This run is bitmap glyphs. Use Direct2D to draw them.
                    d2dContext4->DrawColorBitmapGlyphRun(colorRun->glyphImageFormat,
                                                         currentBaselineOrigin,
                                                         &colorRun->glyphRun,
                                                         measuringMode);
                }
                break;

                case DWRITE_GLYPH_IMAGE_FORMATS_SVG:
                {
                    // This run is SVG glyphs. Use Direct2D to draw them.
                    d2dContext4->DrawSvgGlyphRun(currentBaselineOrigin,
                                                 &colorRun->glyphRun,
                                                 drawingContext->defaultBrush,
                                                 nullptr,                // svgGlyphStyle
                                                 0,                      // colorPaletteIndex
                                                 measuringMode);
                }
                break;

                case DWRITE_GLYPH_IMAGE_FORMATS_TRUETYPE:
                case DWRITE_GLYPH_IMAGE_FORMATS_CFF:
                case DWRITE_GLYPH_IMAGE_FORMATS_COLR:
                default:
                {
                    // This run is solid-color outlines, either from non-color
                    // glyphs or from COLR glyph layers. Use Direct2D to draw them.

                    ID2D1Brush* layerBrush;
                    if (colorRun->paletteIndex == 0xFFFF)
                    {
                        // This run uses the current text color.
                        layerBrush = drawingContext->defaultBrush;
                    }
                    else
                    {
                        if (!tempBrush)
                        {
                            RETURN_IF_FAILED(d2dContext4->CreateSolidColorBrush(colorRun->runColor, &tempBrush));
                        }
                        else
                        {
                            // This run specifies its own color.
                            tempBrush->SetColor(colorRun->runColor);

                        }
                        layerBrush = tempBrush.Get();
                    }

                    // Draw the run with the selected color.
                    d2dContext4->DrawGlyphRun(currentBaselineOrigin,
                                              &colorRun->glyphRun,
                                              colorRun->glyphRunDescription,
                                              layerBrush,
                                              measuringMode);
                }
                break;
                }
            }
        }
    }
    else
    {
        // Simple case: the run has no color glyphs. Draw the main glyph run
              // using the current text color.
        d2dContext4->DrawGlyphRun(baselineOrigin,
                                  glyphRun,
                                  glyphRunDescription,
                                  drawingContext->defaultBrush,
                                  measuringMode);

        // // This is regular text but manually
        //     ::Microsoft::WRL::ComPtr<ID2D1Factory> d2dFactory;
        //drawingContext->renderTarget->GetFactory(d2dFactory.GetAddressOf());

        //::Microsoft::WRL::ComPtr<ID2D1PathGeometry> pathGeometry;
        //d2dFactory->CreatePathGeometry(pathGeometry.GetAddressOf());

        //::Microsoft::WRL::ComPtr<ID2D1GeometrySink> geometrySink;
        //pathGeometry->Open(geometrySink.GetAddressOf());

        //glyphRun->fontFace->GetGlyphRunOutline(
        //    glyphRun->fontEmSize,
        //    glyphRun->glyphIndices,
        //    glyphRun->glyphAdvances,
        //    glyphRun->glyphOffsets,
        //    glyphRun->glyphCount,
        //    glyphRun->isSideways,
        //    glyphRun->bidiLevel % 2,
        //    geometrySink.Get()
        //);

        //geometrySink->Close();

        //D2D1::Matrix3x2F const matrixAlign = D2D1::Matrix3x2F::Translation(baselineOriginX, baselineOriginY);

        //::Microsoft::WRL::ComPtr<ID2D1TransformedGeometry> transformedGeometry;
        //d2dFactory->CreateTransformedGeometry(pathGeometry.Get(),
        //                                      &matrixAlign,
        //                                      transformedGeometry.GetAddressOf());

        //drawingContext->renderTarget->FillGeometry(transformedGeometry.Get(), drawingContext->defaultBrush);

        //{ // This is glow text
        //    ::Microsoft::WRL::ComPtr<ID2D1TransformedGeometry> alignedGeometry;
        //    d2dFactory->CreateTransformedGeometry(pathGeometry.Get(),
        //        &matrixAlign,
        //        alignedGeometry.GetAddressOf());

        //    ::Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
        //    ::Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> outlineBrush;

        //    drawingContext->renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White, 1.0f), brush.GetAddressOf());
        //    drawingContext->renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red, 1.0f), outlineBrush.GetAddressOf());

        //    drawingContext->renderTarget->DrawGeometry(transformedGeometry.Get(), outlineBrush.Get(), 2.0f);

        //    drawingContext->renderTarget->FillGeometry(alignedGeometry.Get(), brush.Get());
        //}
    }
    return S_OK;
}
