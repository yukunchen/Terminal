
#pragma once

#include "UiaTextRange.hpp"


class UiaDocumentTextRange : public UiaTextRange
{
public:

    UiaDocumentTextRange(IRawElementProviderSimple* pProvider,
                         const TEXT_BUFFER_INFO* const pOutputBuffer,
                         SMALL_RECT viewport,
                         const COORD currentFontSize);

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

};