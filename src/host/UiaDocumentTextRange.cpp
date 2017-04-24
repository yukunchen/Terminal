
#include "precomp.h"

#include "UiaDocumentTextRange.hpp"


UiaDocumentTextRange::UiaDocumentTextRange(IRawElementProviderSimple* pProvider,
                                           const TEXT_BUFFER_INFO* const pOutputBuffer,
                                           SMALL_RECT viewport,
                                           const COORD currentFontSize) :
    UiaTextRange(pProvider, pOutputBuffer, viewport, currentFontSize)
{
}

IFACEMETHODIMP UiaDocumentTextRange::Clone(_Outptr_result_maybenull_ ITextRangeProvider** ppRetVal)
{
    try
    {
        *ppRetVal = new UiaDocumentTextRange(*this);
    }
    catch(...)
    {
        *ppRetVal = nullptr;
        return wil::ResultFromCaughtException();
    }
    if (*ppRetVal == nullptr)
    {
        return E_OUTOFMEMORY;
    }
    return S_OK;
}

IFACEMETHODIMP UiaDocumentTextRange::Compare(_In_opt_ ITextRangeProvider* pRange, _Out_ BOOL* pRetVal)
{
    // there's only one document range, so it's always equal to itself.
    *pRetVal = !!dynamic_cast<UiaDocumentTextRange*>(pRange);
    return S_OK;
}

IFACEMETHODIMP UiaDocumentTextRange::CompareEndpoints(TextPatternRangeEndpoint endpoint,
                                            _In_opt_ ITextRangeProvider* pTargetRange,
                                            _In_ TextPatternRangeEndpoint targetEndpoint,
                                            _Out_ int* pRetVal)
{
    UiaDocumentTextRange* documentRange = dynamic_cast<UiaDocumentTextRange*>(pTargetRange);
    if (documentRange == nullptr)
    {
        return E_NOINTERFACE;
    }

    if (endpoint == targetEndpoint)
    {
        *pRetVal = 0;
    }
    else if (endpoint == TextPatternRangeEndpoint::TextPatternRangeEndpoint_Start &&
             targetEndpoint == TextPatternRangeEndpoint::TextPatternRangeEndpoint_End)
    {
        return -1;
    }
    else
    {
        return 1;
    }
    return S_OK;
}

IFACEMETHODIMP UiaDocumentTextRange::ExpandToEnclosingUnit(_In_ TextUnit unit)
{
    // we have nothing we can expand to
    UNREFERENCED_PARAMETER(unit);
    return S_OK;
}

IFACEMETHODIMP UiaDocumentTextRange::FindAttribute(_In_ TEXTATTRIBUTEID textAttributeId,
                                        _In_ VARIANT val,
                                        _In_ BOOL searchBackward,
                                        _Outptr_result_maybenull_ ITextRangeProvider** ppRetVal)
{
    textAttributeId; val; searchBackward; ppRetVal;
    return E_NOTIMPL;
}

IFACEMETHODIMP UiaDocumentTextRange::FindText(_In_ BSTR text,
                                    BOOL searchBackward,
                                    BOOL ignoreCase,
                                    _Outptr_result_maybenull_ ITextRangeProvider** ppRetVal)
{
    text; searchBackward; ignoreCase; ppRetVal;
    return E_NOTIMPL;
}

IFACEMETHODIMP UiaDocumentTextRange::GetAttributeValue(_In_ TEXTATTRIBUTEID textAttributeId, _Out_ VARIANT* pRetVal)
{
    textAttributeId; pRetVal;
    return E_NOTIMPL;
}

IFACEMETHODIMP UiaDocumentTextRange::GetBoundingRectangles(_Outptr_result_maybenull_ SAFEARRAY** ppRetVal)
{
    POINT topLeft;
    POINT bottomRight;

    topLeft.x = 0;
    topLeft.y = 0;

    size_t viewportWidth = _viewport.Right - _viewport.Left + 1;
    size_t viewportHeight = _viewport.Bottom - _viewport.Top + 1;
    bottomRight.x = static_cast<LONG>(topLeft.x + (_currentFontSize.X * viewportWidth));
    bottomRight.y = static_cast<LONG>(topLeft.y + (_currentFontSize.Y * viewportHeight));

    // convert from client window coords to screen coords
    HWND hwnd = g_ciConsoleInformation.hWnd;
    ClientToScreen(hwnd, &topLeft);
    ClientToScreen(hwnd, &bottomRight);

    // put coords into safearray. they go in as four doubles in the
    // order: left, top, width, height.
    *ppRetVal = SafeArrayCreateVector(VT_R8, 0, 4);

    // left
    LONG currentElement = 0;
    double inputData = topLeft.x;
    SafeArrayPutElement(*ppRetVal, &currentElement, &inputData);

    // top
    ++currentElement;
    inputData = topLeft.y;
    SafeArrayPutElement(*ppRetVal, &currentElement, &inputData);

    // width
    ++currentElement;
    inputData = bottomRight.x - topLeft.x;
    SafeArrayPutElement(*ppRetVal, &currentElement, &inputData);


    // height
    ++currentElement;
    inputData = bottomRight.y - topLeft.y;
    SafeArrayPutElement(*ppRetVal, &currentElement, &inputData);

    return S_OK;
}

IFACEMETHODIMP UiaDocumentTextRange::GetEnclosingElement(_Outptr_result_maybenull_ IRawElementProviderSimple** ppRetVal)
{
    *ppRetVal = _pProvider;
    _pProvider->AddRef();
    return S_OK;
}

IFACEMETHODIMP UiaDocumentTextRange::GetText(_In_ int maxLength, _Out_ BSTR* pRetVal)
{
    std::wstring wstr = L"";
    const bool getPartialText = maxLength != -1;

    for (UINT i = 0; i < _pOutputBuffer->TotalRowCount(); ++i)
    {
        const ROW* const row = _pOutputBuffer->GetRowByOffset(i);
        if (row->CharRow.ContainsText())
        {
            std::wstring tempString = std::wstring(row->CharRow.Chars + row->CharRow.Left,
                                                    row->CharRow.Chars + row->CharRow.Right);
            wstr += tempString;
        }
        wstr += L"\r\n";
        if (getPartialText && wstr.size() > maxLength)
        {
            wstr.resize(maxLength);
            break;
        }
    }

    *pRetVal = SysAllocString(wstr.c_str());
    return S_OK;
}

IFACEMETHODIMP UiaDocumentTextRange::Move(_In_ TextUnit unit, _In_ int count, _Out_ int* pRetVal)
{
    // we can't move
    UNREFERENCED_PARAMETER(unit);
    UNREFERENCED_PARAMETER(count);
    *pRetVal = 0;
    return S_OK;
}

IFACEMETHODIMP UiaDocumentTextRange::MoveEndpointByUnit(_In_ TextPatternRangeEndpoint endpoint,
                                                _In_ TextUnit unit,
                                                _In_ int count,
                                                _Out_ int* pRetVal)
{
    // we can't move
    UNREFERENCED_PARAMETER(endpoint);
    UNREFERENCED_PARAMETER(unit);
    UNREFERENCED_PARAMETER(count);
    *pRetVal = 0;
    return S_OK;
}

IFACEMETHODIMP UiaDocumentTextRange::MoveEndpointByRange(_In_ TextPatternRangeEndpoint endpoint,
                                                _In_opt_ ITextRangeProvider* pTargetRange,
                                                _In_ TextPatternRangeEndpoint targetEndpoint)
{
    // we can't move
    UNREFERENCED_PARAMETER(endpoint);
    UNREFERENCED_PARAMETER(pTargetRange);
    UNREFERENCED_PARAMETER(targetEndpoint);
    return S_OK;
}

IFACEMETHODIMP UiaDocumentTextRange::Select()
{
    return E_NOTIMPL;
}

IFACEMETHODIMP UiaDocumentTextRange::AddToSelection()
{
    return E_NOTIMPL;
}

IFACEMETHODIMP UiaDocumentTextRange::RemoveFromSelection()
{
    return E_NOTIMPL;
}

IFACEMETHODIMP UiaDocumentTextRange::ScrollIntoView(_In_ BOOL alignToTop)
{
    alignToTop;
    return E_NOTIMPL;
}

IFACEMETHODIMP UiaDocumentTextRange::GetChildren(_Outptr_result_maybenull_ SAFEARRAY** ppRetVal)
{
    ppRetVal;
    return E_NOTIMPL;
}
