#include "precomp.h"

#include "CustomTextLayout.h"

#include <wrl.h>
#include <wrl/client.h>

using namespace Microsoft::Console::Render;

CustomTextLayout::CustomTextLayout(IDWriteFactory2* const factory,
                                   IDWriteTextAnalyzer1* const analyzer,
                                   IDWriteTextFormat2* const format,
                                   IDWriteFontFace5* const font,
                                   std::basic_string_view<Cluster> const clusters,
                                   size_t const width) :
    _factory{ factory },
    _analyzer{ analyzer },
    _format{ format },
    _font{ font },
    _refCount{ 0 },
    _localeName{},
    _numberSubstitution{},
    _readingDirection{ DWRITE_READING_DIRECTION_LEFT_TO_RIGHT },
    _runs{},
    _breakpoints{},
    _runIndex{ 0 },
    _width{ width }
{
    // Fetch the locale name out once now from the format
    _localeName.resize(format->GetLocaleNameLength() + 1); // +1 for null
    THROW_IF_FAILED(format->GetLocaleName(_localeName.data(), gsl::narrow<UINT32>(_localeName.size())));

    for (const auto& cluster : clusters)
    {
        const auto cols = gsl::narrow<UINT16>(cluster.GetColumns());
        _textClusterColumns.push_back(cols);
        _text += cluster.GetText();
    }
}

CustomTextLayout::~CustomTextLayout()
{
}

HRESULT STDMETHODCALLTYPE CustomTextLayout::Draw(_In_opt_ void* clientDrawingContext,
                                                 _In_ IDWriteTextRenderer* renderer,
                                                 FLOAT originX,
                                                 FLOAT originY)
{
    RETURN_IF_FAILED(_AnalyzeRuns());
    RETURN_IF_FAILED(_ShapeGlyphRuns());
    RETURN_IF_FAILED(_DrawGlyphRuns(clientDrawingContext, renderer, { originX, originY }));

    return S_OK;
}

HRESULT CustomTextLayout::_AnalyzeRuns() noexcept
{
    try
    {
        // We're going to need the text length in UINT32 format for the DWrite calls.
        // Convert it once up front.
        const auto textLength = gsl::narrow<UINT32>(_text.size());

        // Initially start out with one result that covers the entire range.
        // This result will be subdivided by the analysis processes.
        _runs.resize(1);
        auto& initialRun = _runs.front();
        initialRun.nextRunIndex = 0;
        initialRun.textStart = 0;
        initialRun.textLength = textLength;
        initialRun.bidiLevel = (_readingDirection == DWRITE_READING_DIRECTION_RIGHT_TO_LEFT);

        // Allocate enough room to have one breakpoint per code unit.
        _breakpoints.resize(_text.size());

        // Call each of the analyzers in sequence, recording their results.
        RETURN_IF_FAILED(_analyzer->AnalyzeLineBreakpoints(this, 0, textLength, this));
        RETURN_IF_FAILED(_analyzer->AnalyzeBidi(this, 0, textLength, this));
        RETURN_IF_FAILED(_analyzer->AnalyzeScript(this, 0, textLength, this));
        RETURN_IF_FAILED(_analyzer->AnalyzeNumberSubstitution(this, 0, textLength, this));

        // Perform our custom font fallback analyzer that mimics the pattern of the real analyzers.
        RETURN_IF_FAILED(_AnalyzeFontFallback(this, 0, textLength));

        // Resequence the resulting runs in order before returning to caller.
        size_t totalRuns = _runs.size();
        std::vector<LinkedRun> runs;
        runs.resize(totalRuns);

        UINT32 nextRunIndex = 0;
        for (size_t i = 0; i < totalRuns; ++i)
        {
            runs[i] = _runs[nextRunIndex];
            nextRunIndex = _runs[nextRunIndex].nextRunIndex;
        }

        _runs.swap(runs);
    }
    CATCH_RETURN();
    return S_OK;
}

HRESULT CustomTextLayout::_ShapeGlyphRuns() noexcept
{
    try
    {
        // Shapes all the glyph runs in the layout.
        const auto textLength = gsl::narrow<UINT32>(_text.size());

        // Estimate the maximum number of glyph indices needed to hold a string.
        UINT32 estimatedGlyphCount = _EstimateGlyphCount(textLength);

        _glyphIndices.resize(estimatedGlyphCount);
        _glyphOffsets.resize(estimatedGlyphCount);
        _glyphAdvances.resize(estimatedGlyphCount);
        _glyphClusters.resize(textLength);

        UINT32 glyphStart = 0;

        // Shape each run separately. This is needed whenever script, locale,
        // or reading direction changes.
        for (UINT32 runIndex = 0; runIndex < _runs.size(); ++runIndex)
        {
            _ShapeGlyphRun(runIndex, glyphStart);
        }

        _glyphIndices.resize(glyphStart);
        _glyphOffsets.resize(glyphStart);
        _glyphAdvances.resize(glyphStart);
    }
    CATCH_RETURN();
    return S_OK;
}

HRESULT CustomTextLayout::_ShapeGlyphRun(const UINT32 runIndex, UINT32& glyphStart) noexcept
{
    try
    {
        // Shapes a single run of text into glyphs.
       // Alternately, you could iteratively interleave shaping and line
       // breaking to reduce the number glyphs held onto at once. It's simpler
       // for this demostration to just do shaping and line breaking as two
       // separate processes, but realize that this does have the consequence that
       // certain advanced fonts containing line specific features (like Gabriola)
       // will shape as if the line is not broken.

        Run& run = _runs[runIndex];
        UINT32 textStart = run.textStart;
        UINT32 textLength = run.textLength;
        UINT32 maxGlyphCount = static_cast<UINT32>(_glyphIndices.size() - glyphStart);
        UINT32 actualGlyphCount = 0;

        run.glyphStart = glyphStart;
        run.glyphCount = 0;

        if (textLength == 0)
            return S_FALSE; // Nothing to do..

        // Get the font for this run
        ::Microsoft::WRL::ComPtr<IDWriteFontFace> face;
        if (run.font)
        {
            RETURN_IF_FAILED(run.font->CreateFontFace(&face));
        }
        else
        {
            face = _font;
        }

        ////////////////////
        // Allocate space for shaping to fill with glyphs and other information,
        // with about as many glyphs as there are text characters. We'll actually
        // need more glyphs than codepoints if they are decomposed into separate
        // glyphs, or fewer glyphs than codepoints if multiple are substituted
        // into a single glyph. In any case, the shaping process will need some
        // room to apply those rules to even make that determintation.

        if (textLength > maxGlyphCount)
        {
            maxGlyphCount = _EstimateGlyphCount(textLength);
            UINT32 totalGlyphsArrayCount = glyphStart + maxGlyphCount;
            _glyphIndices.resize(totalGlyphsArrayCount);
        }

        std::vector<DWRITE_SHAPING_TEXT_PROPERTIES>  textProps(textLength);
        std::vector<DWRITE_SHAPING_GLYPH_PROPERTIES> glyphProps(maxGlyphCount);

        ////////////////////
        // Get the glyphs from the text, retrying if needed.

        int tries = 0;

        HRESULT hr = S_OK;
        do
        {
            hr = _analyzer->GetGlyphs(
                &_text[textStart],
                textLength,
                face.Get(),
                run.isSideways,         // isSideways,
                (run.bidiLevel & 1),    // isRightToLeft
                &run.script,
                _localeName.data(),
                (run.isNumberSubstituted) ? _numberSubstitution.Get() : nullptr,
                nullptr,                   // features
                nullptr,                   // featureLengths
                0,                      // featureCount
                maxGlyphCount,          // maxGlyphCount
                &_glyphClusters[textStart],
                &textProps[0],
                &_glyphIndices[glyphStart],
                &glyphProps[0],
                &actualGlyphCount
            );
            tries++;

            if (hr == E_NOT_SUFFICIENT_BUFFER)
            {
                // Try again using a larger buffer.
                maxGlyphCount = _EstimateGlyphCount(maxGlyphCount);
                UINT32 totalGlyphsArrayCount = glyphStart + maxGlyphCount;

                glyphProps.resize(maxGlyphCount);
                _glyphIndices.resize(totalGlyphsArrayCount);
            }
            else
            {
                break;
            }
        } while (tries < 2); // We'll give it two chances.

        RETURN_IF_FAILED(hr);

        ////////////////////
        // Get the placement of the all the glyphs.

        _glyphAdvances.resize(std::max(static_cast<size_t>(glyphStart + actualGlyphCount), _glyphAdvances.size()));
        _glyphOffsets.resize(std::max(static_cast<size_t>(glyphStart + actualGlyphCount), _glyphOffsets.size()));

        const auto fontSizeFormat = _format->GetFontSize();
        const auto fontSize = fontSizeFormat * run.fontScale;

        hr = _analyzer->GetGlyphPlacements(
            &_text[textStart],
            &_glyphClusters[textStart],
            &textProps[0],
            textLength,
            &_glyphIndices[glyphStart],
            &glyphProps[0],
            actualGlyphCount,
            face.Get(),
            fontSize,
            run.isSideways,
            (run.bidiLevel & 1),    // isRightToLeft
            &run.script,
            _localeName.data(),
            NULL,                   // features
            NULL,                   // featureRangeLengths
            0,                      // featureRanges
            &_glyphAdvances[glyphStart],
            &_glyphOffsets[glyphStart]
        );

        RETURN_IF_FAILED(hr);

        // If we need to detect font fallback, we can do it this way:
        // if (!_font->Equals(face.Get()))

        // We're going to walk through and check for advances that don't match the space that we expect to give out.
        {
            IDWriteFontFace1* face1;
            RETURN_IF_FAILED(face.Get()->QueryInterface(&face1));

            DWRITE_FONT_METRICS1 metrics;
            face1->GetMetrics(&metrics);

            // Walk through advances and space out characters that are too small to consume their box.
            for (auto i = glyphStart; i < (glyphStart + actualGlyphCount); i++)
            {
                // Advance is how wide in pixels the glyph is
                auto& advance = _glyphAdvances[i];

                // Offsets is how far to move the origin (in pixels) from where it is
                auto& offset = _glyphOffsets[i];

                // Get how many columns we expected the glyph to have and mutiply into pixels.
                const auto columns = _textClusterColumns[i];
                const auto advanceExpected = (float)(columns * _width);

                // If what we expect is bigger than what we have... pad it out.
                if (advanceExpected > advance)
                {
                    // Get the amount of space we have leftover.
                    const auto diff = advanceExpected - advance;

                    // Move the X offset (pixels to the right from the left edge) by half the excess space
                    // so half of it will be left of the glyph and the other half on the right.
                    offset.advanceOffset += diff / 2;

                    // Set the advance to the perfect width we want.
                    advance = advanceExpected;
                }
                // If what we expect is smaller than what we have... rescale the font size to get a smaller glyph to fit.
                else if (advanceExpected < advance)
                {
                    // We need to retrieve the design information for this specific glyph so we can figure out the appropriate
                    // height proportional to the width that we desire.
                    INT32 advanceInDesignUnits;
                    RETURN_IF_FAILED(face1->GetDesignGlyphAdvances(1, &_glyphIndices[i], &advanceInDesignUnits));

                    DWRITE_GLYPH_METRICS glyphMetrics;
                    RETURN_IF_FAILED(face1->GetDesignGlyphMetrics(&_glyphIndices[i], 1, &glyphMetrics));


                    // When things are drawn, we want the font size (as specified in the base font in the original format)
                    // to be scaled by some factor.
                    // i.e. if the original font size was 16, we might want to draw this glyph with a 15.2 size font so
                    // the width (and height) of the glyph will shrink to fit the monospace cell box.

                    // This pattern is copied from the DxRenderer's algorithm for figuring out the font height for a specific width
                    // and was advised by the DirectWrite team. 
                    const float widthAdvance = static_cast<float>(advanceInDesignUnits) / metrics.designUnitsPerEm;
                    const auto fontSizeWant = advanceExpected / widthAdvance;
                    run.fontScale = fontSizeWant / fontSizeFormat;

                    // Set the advance to the perfect width that we want.
                    advance = advanceExpected;
                }
            }
        }

        ////////////////////
        // Certain fonts, like Batang, contain glyphs for hidden control
        // and formatting characters. So we'll want to explicitly force their
        // advance to zero.
        // I'm leaving this here for future reference, but I don't think we want invisible glyphs for this renderer.
        //if (run.script.shapes & DWRITE_SCRIPT_SHAPES_NO_VISUAL)
        //{
        //    std::fill(_glyphAdvances.begin() + glyphStart,
        //              _glyphAdvances.begin() + glyphStart + actualGlyphCount,
        //              0.0f
        //    );
        //}

        ////////////////////
        // Set the final glyph count of this run and advance the starting glyph.
        run.glyphCount = actualGlyphCount;
        glyphStart += actualGlyphCount;
    }
    CATCH_RETURN();
    return S_OK;
}

HRESULT CustomTextLayout::_DrawGlyphRuns(_In_opt_ void* clientDrawingContext,
                                         IDWriteTextRenderer* renderer,
                                         const D2D_POINT_2F origin) noexcept
{
    try
    {
        // We're going to start from the origin given and walk to the right for each
        // sub-run that was calculated by the layout analysis.
        auto mutableOrigin = origin;

        // Draw each run separately.
        for (UINT32 runIndex = 0; runIndex < _runs.size(); ++runIndex)
        {
            // Get the run
            Run& run = _runs[runIndex];

            // Get the font face from the font metadata provided by the fallback analysis
            ::Microsoft::WRL::ComPtr<IDWriteFontFace> face;
            if (run.font)
            {
                RETURN_IF_FAILED(run.font->CreateFontFace(&face));
            }
            else
            {
                face = _font;
            }
            

            // Prepare the glyph run and description objects by converting our
            // internal storage representation into something that matches DWrite's structures.
            DWRITE_GLYPH_RUN glyphRun = { 0 };
            glyphRun.bidiLevel = run.bidiLevel;
            glyphRun.fontEmSize = _format->GetFontSize() * run.fontScale;
            glyphRun.fontFace = face.Get();
            glyphRun.glyphAdvances = _glyphAdvances.data() + run.glyphStart;
            glyphRun.glyphCount = run.glyphCount;
            glyphRun.glyphIndices = _glyphIndices.data() + run.glyphStart;
            glyphRun.glyphOffsets = _glyphOffsets.data() + run.glyphStart;
            glyphRun.isSideways = false;

            DWRITE_GLYPH_RUN_DESCRIPTION glyphRunDescription = { 0 };
            glyphRunDescription.clusterMap = _glyphClusters.data();
            glyphRunDescription.localeName = _localeName.data();
            glyphRunDescription.string = _text.data();
            glyphRunDescription.stringLength = run.textLength;
            glyphRunDescription.textPosition = run.textStart;

            // Try to draw it
            RETURN_IF_FAILED(renderer->DrawGlyphRun(clientDrawingContext,
                                                   mutableOrigin.x,
                                                   mutableOrigin.y,
                                                   DWRITE_MEASURING_MODE_NATURAL,
                                                   &glyphRun,
                                                   &glyphRunDescription,
                                                   nullptr));

            // Shift origin to the right for the next run based on the amount of space consumed.
            mutableOrigin.x = std::accumulate(_glyphAdvances.begin() + run.glyphStart,
                                              _glyphAdvances.begin() + run.glyphStart + run.glyphCount,
                                              mutableOrigin.x);
        }
    }
    CATCH_RETURN();
    return S_OK;
}

// Estimates the maximum number of glyph indices needed to hold a string of 
    // a given length.  This is the formula given in the Uniscribe SDK and should
    // cover most cases. Degenerate cases will require a reallocation.
UINT32 CustomTextLayout::_EstimateGlyphCount(UINT32 textLength) noexcept
{
    return 3 * textLength / 2 + 16;
}

#pragma region IUnknown methods
ULONG STDMETHODCALLTYPE CustomTextLayout::AddRef()
{
    return ++_refCount;
}

ULONG STDMETHODCALLTYPE CustomTextLayout::Release()
{
    const auto newCount = --_refCount;

    if (newCount == 0)
    {
        delete this;
    }

    return newCount;
}

HRESULT STDMETHODCALLTYPE CustomTextLayout::QueryInterface(_In_ REFIID riid,
                                                           _Outptr_ void** ppOutput)
{
    *ppOutput = nullptr;
    HRESULT hr = S_OK;

    if (riid == __uuidof(IDWriteTextAnalysisSource))
    {
        *ppOutput = static_cast<IDWriteTextAnalysisSource*>(this);
        AddRef();
    }
    else if (riid == __uuidof(IDWriteTextAnalysisSink))
    {
        *ppOutput = static_cast<IDWriteTextAnalysisSink*>(this);
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
#pragma endregion

#pragma region IDWriteTextAnalysisSource methods
HRESULT STDMETHODCALLTYPE CustomTextLayout::GetTextAtPosition(UINT32 textPosition,
                                                              _Outptr_result_buffer_(*textLength) WCHAR const** textString,
                                                              _Out_ UINT32* textLength)
{
    *textString = nullptr;
    *textLength = 0;

    if (textPosition < _text.size())
    {
        *textString = _text.data() + textPosition;
        *textLength = gsl::narrow<UINT32>(_text.size()) - textPosition;
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CustomTextLayout::GetTextBeforePosition(UINT32 textPosition,
                                                                  _Outptr_result_buffer_(*textLength) WCHAR const** textString,
                                                                  _Out_ UINT32* textLength)
{
    *textString = nullptr;
    *textLength = 0;

    if (textPosition > 0 && textPosition <= _text.size())
    {
        *textString = _text.data();
        *textLength = textPosition;
    }

    return S_OK;
}

DWRITE_READING_DIRECTION STDMETHODCALLTYPE CustomTextLayout::GetParagraphReadingDirection()
{
    return _readingDirection;
}

HRESULT STDMETHODCALLTYPE CustomTextLayout::GetLocaleName(UINT32 textPosition,
                                                          _Out_ UINT32* textLength,
                                                          _Outptr_result_z_ WCHAR const** localeName)
{
    *localeName = _localeName.data();
    *textLength = gsl::narrow<UINT32>(_text.size()) - textPosition;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CustomTextLayout::GetNumberSubstitution(UINT32 textPosition,
                                                                  _Out_ UINT32* textLength,
                                                                  _COM_Outptr_ IDWriteNumberSubstitution** numberSubstitution)
{
    *numberSubstitution = nullptr;
    *textLength = gsl::narrow<UINT32>(_text.size()) - textPosition;

    return S_OK;
}
#pragma endregion

#pragma region IDWriteTextAnalysisSink methods
HRESULT STDMETHODCALLTYPE CustomTextLayout::SetScriptAnalysis(UINT32 textPosition,
                                                              UINT32 textLength,
                                                              _In_ DWRITE_SCRIPT_ANALYSIS const* scriptAnalysis)
{
    try
    {
        _SetCurrentRun(textPosition);
        _SplitCurrentRun(textPosition);
        while (textLength > 0)
        {
            auto& run = _FetchNextRun(textLength);
            run.script = *scriptAnalysis;
        }
    }
    CATCH_RETURN();

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CustomTextLayout::SetLineBreakpoints(UINT32 textPosition,
                                                               UINT32 textLength,
                                                               _In_reads_(textLength) DWRITE_LINE_BREAKPOINT const* lineBreakpoints)
{
    UNREFERENCED_PARAMETER(textLength);
    try
    {
        _breakpoints.at(textPosition) = *lineBreakpoints;
    }
    CATCH_RETURN();

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CustomTextLayout::SetBidiLevel(UINT32 textPosition,
                                                         UINT32 textLength,
                                                         UINT8 /*explicitLevel*/,
                                                         UINT8 resolvedLevel)
{
    try
    {
        _SetCurrentRun(textPosition);
        _SplitCurrentRun(textPosition);
        while (textLength > 0)
        {
            auto& run = _FetchNextRun(textLength);
            run.bidiLevel = resolvedLevel;
        }
    }
    CATCH_RETURN();

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CustomTextLayout::SetNumberSubstitution(UINT32 textPosition,
                                                                  UINT32 textLength,
                                                                  _In_ IDWriteNumberSubstitution* numberSubstitution)
{
    try
    {
        _SetCurrentRun(textPosition);
        _SplitCurrentRun(textPosition);
        while (textLength > 0)
        {
            auto& run = _FetchNextRun(textLength);
            run.isNumberSubstituted = (numberSubstitution != nullptr);
        }
    }
    CATCH_RETURN();

    return S_OK;
}
#pragma endregion

#pragma region internal methods for mimicing text analyzer pattern but for font fallback

HRESULT STDMETHODCALLTYPE CustomTextLayout::_AnalyzeFontFallback(IDWriteTextAnalysisSource* const source,
                                                                 UINT32 textPosition,
                                                                 UINT32 textLength)
{
    try
    {
        // Get the font fallback first
        ::Microsoft::WRL::ComPtr<IDWriteTextFormat1> format1;
        RETURN_IF_FAILED(_format.As(&format1));
        RETURN_HR_IF_NULL(E_NOINTERFACE, format1);

        ::Microsoft::WRL::ComPtr<IDWriteFontFallback> fallback;
        RETURN_IF_FAILED(format1->GetFontFallback(&fallback));

        ::Microsoft::WRL::ComPtr<IDWriteFontCollection> collection;
        RETURN_IF_FAILED(format1->GetFontCollection(&collection));

        std::wstring familyName;
        familyName.resize(format1->GetFontFamilyNameLength() + 1);
        RETURN_IF_FAILED(format1->GetFontFamilyName(familyName.data(), gsl::narrow<UINT32>(familyName.size())));

        const auto weight = format1->GetFontWeight();
        const auto style = format1->GetFontStyle();
        const auto stretch = format1->GetFontStretch();

        if (!fallback)
        {
            ::Microsoft::WRL::ComPtr<IDWriteFactory2> factory2;
            RETURN_IF_FAILED(_factory.As(&factory2));
            factory2->GetSystemFontFallback(&fallback);
        }

        // Walk through and analyze the entire string
        while (textLength > 0)
        {
            UINT32 mappedLength = 0;
            IDWriteFont* mappedFont = nullptr;
            FLOAT scale = 0.0f;

            fallback->MapCharacters(source,
                                    textPosition,
                                    textLength,
                                    collection.Get(),
                                    familyName.data(),
                                    weight,
                                    style,
                                    stretch,
                                    &mappedLength,
                                    &mappedFont,
                                    &scale);

            RETURN_IF_FAILED(_SetMappedFont(textPosition, mappedLength, mappedFont, scale));

            textPosition += mappedLength;
            textLength -= mappedLength;
        }
    }
    CATCH_RETURN();

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CustomTextLayout::_SetMappedFont(UINT32 textPosition,
                                                           UINT32 textLength,
                                                           IDWriteFont* const font,
                                                           FLOAT const scale)
{
    try
    {
        _SetCurrentRun(textPosition);
        _SplitCurrentRun(textPosition);
        while (textLength > 0)
        {
            auto& run = _FetchNextRun(textLength);
            run.font = font;
            run.fontScale = scale;
        }
    }
    CATCH_RETURN();

    return S_OK;
}

#pragma endregion

#pragma region internal Run manipulation functions for storing information from sink callbacks
CustomTextLayout::LinkedRun& CustomTextLayout::_FetchNextRun(UINT32& textLength)
{
    // Used by the sink setters, this returns a reference to the next run.
    // Position and length are adjusted to now point after the current run
    // being returned.

    const auto originalRunIndex = _runIndex;

    auto& run = _runs.at(originalRunIndex);
    UINT32 runTextLength = run.textLength;

    // Split the tail if needed (the length remaining is less than the
    // current run's size).
    if (textLength < runTextLength)
    {
        runTextLength = textLength; // Limit to what's actually left.
        UINT32 runTextStart = run.textStart;

        _SplitCurrentRun(runTextStart + runTextLength);
    }
    else
    {
        // Just advance the current run.
        _runIndex = run.nextRunIndex;
    }

    textLength -= runTextLength;

    // Return a reference to the run that was just current.
    // Careful, we have to look it up again as _SplitCurrentRun can resize the array and reshuffle all the reference locations
    return _runs.at(originalRunIndex);
}

void CustomTextLayout::_SetCurrentRun(const UINT32 textPosition)
{
    // Move the current run to the given position.
    // Since the analyzers generally return results in a forward manner,
    // this will usually just return early. If not, find the
    // corresponding run for the text position.

    if (_runIndex < _runs.size()
        && _runs[_runIndex].ContainsTextPosition(textPosition))
    {
        return;
    }

    _runIndex = static_cast<UINT32>(
        std::find(_runs.begin(), _runs.end(), textPosition)
        - _runs.begin()
        );
}

void CustomTextLayout::_SplitCurrentRun(const UINT32 splitPosition)
{
    // Splits the current run and adjusts the run values accordingly.
    UINT32 runTextStart = _runs.at(_runIndex).textStart;

    if (splitPosition <= runTextStart)
        return; // no change

    // Grow runs by one.
    size_t totalRuns = _runs.size();
    try
    {
        _runs.resize(totalRuns + 1);
    }
    catch (...)
    {
        return; // Can't increase size. Return same run.
    }

    // Copy the old run to the end.
    LinkedRun& frontHalf = _runs[_runIndex];
    LinkedRun& backHalf = _runs.back();
    backHalf = frontHalf;

    // Adjust runs' text positions and lengths.
    UINT32 splitPoint = splitPosition - runTextStart;
    backHalf.textStart += splitPoint;
    backHalf.textLength -= splitPoint;
    frontHalf.textLength = splitPoint;
    frontHalf.nextRunIndex = static_cast<UINT32>(totalRuns);
    _runIndex = static_cast<UINT32>(totalRuns);
}
#pragma endregion
