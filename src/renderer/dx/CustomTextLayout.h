#pragma once

#include <dwrite.h>
#include <d2d1.h>

#include <wrl.h>
#include <wrl/client.h>

class CustomTextLayout : public IDWriteTextAnalysisSource, 
                         public IDWriteTextAnalysisSink
{
public:
    // Based on the Windows 7 SDK sample at https://github.com/pauldotknopf/WindowsSDK7-Samples/tree/master/multimedia/DirectWrite/CustomLayout

    CustomTextLayout(IDWriteTextAnalyzer* const analyzer,
                     IDWriteTextFormat* const format,
                     IDWriteFontFace* const font,
                     const std::wstring_view text);
    ~CustomTextLayout();

    // IDWriteTextLayout methods (but we don't actually want to implement them all, so just this one matching the existing interface)
    HRESULT STDMETHODCALLTYPE Draw(_In_opt_ void* clientDrawingContext,
                                   _In_ IDWriteTextRenderer* renderer,
                                   FLOAT originX,
                                   FLOAT originY);

    // IUnknown methods
    virtual ULONG STDMETHODCALLTYPE AddRef() override;
    virtual ULONG STDMETHODCALLTYPE Release() override;
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) override;

    // IDWriteTextAnalysisSource methods
    virtual HRESULT STDMETHODCALLTYPE GetTextAtPosition(UINT32 textPosition,
                                                        _Outptr_result_buffer_(*textLength) WCHAR const** textString,
                                                        _Out_ UINT32* textLength) override;
    virtual HRESULT STDMETHODCALLTYPE GetTextBeforePosition(UINT32 textPosition,
                                                            _Outptr_result_buffer_(*textLength) WCHAR const** textString,
                                                            _Out_ UINT32* textLength) override;
    virtual DWRITE_READING_DIRECTION STDMETHODCALLTYPE GetParagraphReadingDirection() override;
    virtual HRESULT STDMETHODCALLTYPE GetLocaleName(UINT32 textPosition,
                                                   _Out_ UINT32* textLength,
                                                   _Outptr_result_z_ WCHAR const** localeName) override;
    virtual HRESULT STDMETHODCALLTYPE GetNumberSubstitution(UINT32 textPosition,
                                                            _Out_ UINT32* textLength,
                                                            _COM_Outptr_ IDWriteNumberSubstitution** numberSubstitution) override;

    // IDWriteTextAnalysisSink methods
    virtual HRESULT STDMETHODCALLTYPE SetScriptAnalysis(UINT32 textPosition,
                                                        UINT32 textLength,
                                                        _In_ DWRITE_SCRIPT_ANALYSIS const* scriptAnalysis) override;
    virtual HRESULT STDMETHODCALLTYPE SetLineBreakpoints(UINT32 textPosition,
                                                         UINT32 textLength,
                                                         _In_reads_(textLength) DWRITE_LINE_BREAKPOINT const* lineBreakpoints) override;
    virtual HRESULT STDMETHODCALLTYPE SetBidiLevel(UINT32 textPosition,
                                                   UINT32 textLength,
                                                   UINT8 explicitLevel,
                                                   UINT8 resolvedLevel) override;
    virtual HRESULT STDMETHODCALLTYPE SetNumberSubstitution(UINT32 textPosition,
                                                            UINT32 textLength,
                                                            _In_ IDWriteNumberSubstitution* numberSubstitution) override;
protected:
    // A single contiguous run of characters containing the same analysis results.
    struct Run
    {
        Run()
            : textStart(),
            textLength(),
            glyphStart(),
            glyphCount(),
            bidiLevel(),
            script(),
            isNumberSubstituted(),
            isSideways()
        { }

        UINT32 textStart;   // starting text position of this run
        UINT32 textLength;  // number of contiguous code units covered
        UINT32 glyphStart;  // starting glyph in the glyphs array
        UINT32 glyphCount;  // number of glyphs associated with this run of text
        DWRITE_SCRIPT_ANALYSIS script;
        UINT8 bidiLevel;
        bool isNumberSubstituted;
        bool isSideways;

        inline bool ContainsTextPosition(UINT32 desiredTextPosition) const throw()
        {
            return desiredTextPosition >= textStart
                && desiredTextPosition < textStart + textLength;
        }

        inline bool operator==(UINT32 desiredTextPosition) const throw()
        {
            // Search by text position using std::find
            return ContainsTextPosition(desiredTextPosition);
        }
    };

    // Single text analysis run, which points to the next run.
    struct LinkedRun : Run
    {
        LinkedRun()
            : nextRunIndex(0)
        { }

        UINT32 nextRunIndex;  // index of next run
    };

    LinkedRun& _FetchNextRun(UINT32& textLength);
    void _SetCurrentRun(const UINT32 textPosition);
    void _SplitCurrentRun(const UINT32 splitPosition);

    void _AnalyzeRuns();
    void _ShapeGlyphRuns();
    void _ShapeGlyphRun(const UINT32 runIndex, UINT32& glyphStart);

    static UINT32 _EstimateGlyphCount(UINT32 textLength);


private:
    // DirectWrite analyzer
    const ::Microsoft::WRL::ComPtr<IDWriteTextAnalyzer> _analyzer;

    // DirectWrite text format
    const ::Microsoft::WRL::ComPtr<IDWriteTextFormat> _format;

    // DirectWrite font face
    const ::Microsoft::WRL::ComPtr<IDWriteFontFace> _font;

    // COM count
    std::atomic<ULONG> _refCount;

    // The text we're analyzing and processing into a layout
    const std::wstring _text;

    // Properties of the text that might be relevant.
    std::wstring _localeName;
    ::Microsoft::WRL::ComPtr<IDWriteNumberSubstitution> _numberSubstitution;
    DWRITE_READING_DIRECTION _readingDirection;

    // Text analysis results
    std::vector<LinkedRun> _runs;
    std::vector<DWRITE_LINE_BREAKPOINT> _breakpoints;

    // Text analysis interim status variable (to assist the Analyzer Sink in operations involving _runs)
    UINT32 _runIndex;

    // Glyph shaping results
    std::vector<DWRITE_GLYPH_OFFSET> _glyphOffsets;
    std::vector<UINT16> _glyphClusters;
    std::vector<UINT16> _glyphIndices;
    std::vector<float> _glyphAdvances;
};
