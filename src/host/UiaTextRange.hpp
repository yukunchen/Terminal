#pragma once

class UiaTextRange : public ITextRangeProvider
{
public:

    UiaTextRange(IRawElementProviderSimple* pProvider,
                 const TEXT_BUFFER_INFO* const pOutputBuffer,
                 SMALL_RECT viewport,
                 const COORD currentFontSize);

    ~UiaTextRange();

    // IUnknown methods
    IFACEMETHODIMP_(ULONG) AddRef();
    IFACEMETHODIMP_(ULONG) Release();
    IFACEMETHODIMP QueryInterface(_In_ REFIID riid, _COM_Outptr_result_maybenull_ void** ppInterface);

    // ITextRangeProvider methods
    virtual IFACEMETHODIMP Clone(_Outptr_result_maybenull_ ITextRangeProvider** ppRetVal) = 0;
    virtual IFACEMETHODIMP Compare(_In_opt_ ITextRangeProvider* pRange, _Out_ BOOL* pRetVal) = 0;
    virtual IFACEMETHODIMP CompareEndpoints(TextPatternRangeEndpoint endpoint,
                                               _In_opt_ ITextRangeProvider* pTargetRange,
                                               _In_ TextPatternRangeEndpoint targetEndpoint,
                                               _Out_ int* pRetVal) = 0;
    virtual IFACEMETHODIMP ExpandToEnclosingUnit(_In_ TextUnit unit) = 0;
    virtual IFACEMETHODIMP FindAttribute(_In_ TEXTATTRIBUTEID textAttributeId,
                                            _In_ VARIANT val,
                                            _In_ BOOL searchBackward,
                                            _Outptr_result_maybenull_ ITextRangeProvider** ppRetVal) = 0;
    virtual IFACEMETHODIMP FindText(_In_ BSTR text,
                                       BOOL searchBackward,
                                       BOOL ignoreCase,
                                       _Outptr_result_maybenull_ ITextRangeProvider** ppRetVal) = 0;
    virtual IFACEMETHODIMP GetAttributeValue(_In_ TEXTATTRIBUTEID textAttributeId, _Out_ VARIANT* pRetVal) = 0;
    virtual IFACEMETHODIMP GetBoundingRectangles(_Outptr_result_maybenull_ SAFEARRAY** ppRetVal) = 0;
    virtual IFACEMETHODIMP GetEnclosingElement(_Outptr_result_maybenull_ IRawElementProviderSimple** ppRetVal) = 0;
    virtual IFACEMETHODIMP GetText(_In_ int maxLength, _Out_ BSTR* pRetVal) = 0;
    virtual IFACEMETHODIMP Move(_In_ TextUnit unit, _In_ int count, _Out_ int* pRetVal) = 0;
    virtual IFACEMETHODIMP MoveEndpointByUnit(_In_ TextPatternRangeEndpoint endpoint,
                                                 _In_ TextUnit unit,
                                                 _In_ int count,
                                                 _Out_ int* pRetVal) = 0;
    virtual IFACEMETHODIMP MoveEndpointByRange(_In_ TextPatternRangeEndpoint endpoint,
                                                  _In_opt_ ITextRangeProvider* pTargetRange,
                                                  _In_ TextPatternRangeEndpoint targetEndpoint) = 0;
    virtual IFACEMETHODIMP Select() = 0;
    virtual IFACEMETHODIMP AddToSelection() = 0;
    virtual IFACEMETHODIMP RemoveFromSelection() = 0;
    virtual IFACEMETHODIMP ScrollIntoView(_In_ BOOL alignToTop) = 0;
    virtual IFACEMETHODIMP GetChildren(_Outptr_result_maybenull_ SAFEARRAY** ppRetVal) = 0;

protected:
    const TEXT_BUFFER_INFO* const _pOutputBuffer;
    const COORD _currentFontSize;
    IRawElementProviderSimple* _pProvider;
    SMALL_RECT _viewport;

private:
    ULONG _cRefs;
};