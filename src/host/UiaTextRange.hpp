#pragma once

class UiaTextRange : ITextRangeProvider
{
public:

    // TODO:
    // - turn the other constructors back on.
    // - reorder args
    // - possibly find a better way than constructor overloading to
    // differentiate between the different needs of the range which
    // change depending on the TextUnit
    /*
    UiaTextRange(IRawElementProviderSimple* pProvider,
                 const TEXT_BUFFER* const pOutputBuffer,
                 TextUnit textUnit,
                 const COORD currentFontSize);
    */

    UiaTextRange(IRawElementProviderSimple* pProvider,
                 const TEXT_BUFFER_INFO* const pOutputBuffer,
                 TextUnit textUnit,
                 const COORD currentFontSize,
                 size_t lineNumber,
                 size_t viewportLineNumber,
                 SMALL_RECT viewport);

    /*
    UiaTextRange(IRawElementProviderSimple* pProvider,
                 const TEXT_BUFFER* const pOutputBuffer,
                 TextUnit textUnit,
                 const COORD currentFontSize,
                 size_t lineNumber,
                 size_t unitNumber);
    */

    ~UiaTextRange();

    // IUnknown methods
    IFACEMETHODIMP_(ULONG) AddRef();
    IFACEMETHODIMP_(ULONG) Release();
    IFACEMETHODIMP QueryInterface(_In_ REFIID riid, _COM_Outptr_result_maybenull_ void** ppInterface);

    // ITextRangeProvider methods
    IFACEMETHODIMP Clone(_Outptr_result_maybenull_ ITextRangeProvider** ppRetVal);
    IFACEMETHODIMP Compare(_In_opt_ ITextRangeProvider* pRange, _Out_ BOOL* pRetVal);
    IFACEMETHODIMP CompareEndpoints(TextPatternRangeEndpoint endpoint,
                                               _In_opt_ ITextRangeProvider* pTargetRange,
                                               _In_ TextPatternRangeEndpoint targetEndpoint,
                                               _Out_ int* pRetVal);
    IFACEMETHODIMP ExpandToEnclosingUnit(_In_ TextUnit unit);
    IFACEMETHODIMP FindAttribute(_In_ TEXTATTRIBUTEID textAttributeId,
                                            _In_ VARIANT val,
                                            _In_ BOOL searchBackward,
                                            _Outptr_result_maybenull_ ITextRangeProvider** ppRetVal);
    IFACEMETHODIMP FindText(_In_ BSTR text,
                                       BOOL searchBackward,
                                       BOOL ignoreCase,
                                       _Outptr_result_maybenull_ ITextRangeProvider** ppRetVal);
    IFACEMETHODIMP GetAttributeValue(_In_ TEXTATTRIBUTEID textAttributeId, _Out_ VARIANT* pRetVal);
    IFACEMETHODIMP GetBoundingRectangles(_Outptr_result_maybenull_ SAFEARRAY** ppRetVal);
    IFACEMETHODIMP GetEnclosingElement(_Outptr_result_maybenull_ IRawElementProviderSimple** ppRetVal);
    IFACEMETHODIMP GetText(_In_ int maxLength, _Out_ BSTR* pRetVal);
    IFACEMETHODIMP Move(_In_ TextUnit unit, _In_ int count, _Out_ int* pRetVal);
    IFACEMETHODIMP MoveEndpointByUnit(_In_ TextPatternRangeEndpoint endpoint,
                                                 _In_ TextUnit unit,
                                                 _In_ int count,
                                                 _Out_ int* pRetVal);
    IFACEMETHODIMP MoveEndpointByRange(_In_ TextPatternRangeEndpoint endpoint,
                                                  _In_opt_ ITextRangeProvider* pTargetRange,
                                                  _In_ TextPatternRangeEndpoint targetEndpoint);
    IFACEMETHODIMP Select();
    IFACEMETHODIMP AddToSelection();
    IFACEMETHODIMP RemoveFromSelection();
    IFACEMETHODIMP ScrollIntoView(_In_ BOOL alignToTop);
    IFACEMETHODIMP GetChildren(_Outptr_result_maybenull_ SAFEARRAY** ppRetVal);


    friend bool operator==(const UiaTextRange& a, const UiaTextRange& b);

private:
    ULONG _cRefs;
    const TEXT_BUFFER_INFO* const _pOutputBuffer;
    TextUnit _textUnit;
    const COORD _currentFontSize;
    IRawElementProviderSimple* _pProvider;
    size_t _viewportLineNumber;
    SMALL_RECT _viewport;
    // these describe ranges like [start, end) when applicable
    size_t _lineNumberStart;
    size_t _lineNumberEnd;
    size_t _unitNumberStart;
    size_t _unitNumberEnd;
};

bool operator==(const UiaTextRange& a, const UiaTextRange& b);