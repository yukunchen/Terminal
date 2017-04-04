
#include "precomp.h"

#include "UiaLineTextRange.hpp"


UiaLineTextRange::UiaLineTextRange(IRawElementProviderSimple* pProvider,
                                   const TEXT_BUFFER_INFO* const pOutputBuffer,
                                   SMALL_RECT viewport,
                                   const COORD currentFontSize,
                                   size_t lineNumber) :
    UiaTextRange(pProvider, pOutputBuffer, viewport, currentFontSize),
    _lineNumberStart{ lineNumber },
    _lineNumberEnd{ lineNumber + 1 } // default to 1 line tall
{
}

const size_t UiaLineTextRange::getLineNumberStart() const
{
    return _lineNumberStart;
}

const size_t UiaLineTextRange::getLineNumberEnd() const
{
    return _lineNumberEnd;
}

IFACEMETHODIMP UiaLineTextRange::Clone(_Outptr_result_maybenull_ ITextRangeProvider** ppRetVal)
{
    try
    {
        *ppRetVal = new UiaLineTextRange(*this);
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

IFACEMETHODIMP UiaLineTextRange::Compare(_In_opt_ ITextRangeProvider* pRange, _Out_ BOOL* pRetVal)
{
    *pRetVal = FALSE;
    UiaLineTextRange* other = dynamic_cast<UiaLineTextRange*>(pRange);
    if (other)
    {
        *pRetVal = !!(_lineNumberStart == other->getLineNumberStart() &&
                      _lineNumberEnd == other->getLineNumberEnd());
    }
    return S_OK;
}

IFACEMETHODIMP UiaLineTextRange::CompareEndpoints(TextPatternRangeEndpoint endpoint,
                                            _In_opt_ ITextRangeProvider* pTargetRange,
                                            _In_ TextPatternRangeEndpoint targetEndpoint,
                                            _Out_ int* pRetVal)
{
    endpoint; pTargetRange; targetEndpoint; pRetVal;
    return E_NOTIMPL;
}

IFACEMETHODIMP UiaLineTextRange::ExpandToEnclosingUnit(_In_ TextUnit unit)
{
    unit;
    return E_NOTIMPL;
}

IFACEMETHODIMP UiaLineTextRange::FindAttribute(_In_ TEXTATTRIBUTEID textAttributeId,
                                        _In_ VARIANT val,
                                        _In_ BOOL searchBackward,
                                        _Outptr_result_maybenull_ ITextRangeProvider** ppRetVal)
{
    textAttributeId; val; searchBackward; ppRetVal;
    return E_NOTIMPL;
}

IFACEMETHODIMP UiaLineTextRange::FindText(_In_ BSTR text,
                                    BOOL searchBackward,
                                    BOOL ignoreCase,
                                    _Outptr_result_maybenull_ ITextRangeProvider** ppRetVal)
{
    text; searchBackward; ignoreCase; ppRetVal;
    return E_NOTIMPL;
}

IFACEMETHODIMP UiaLineTextRange::GetAttributeValue(_In_ TEXTATTRIBUTEID textAttributeId, _Out_ VARIANT* pRetVal)
{
    textAttributeId; pRetVal;
    return E_NOTIMPL;
}

IFACEMETHODIMP UiaLineTextRange::GetBoundingRectangles(_Outptr_result_maybenull_ SAFEARRAY** ppRetVal)
{
    POINT topLeft;
    POINT bottomRight;

    size_t viewportLineNumber = _lineNumberStart - _viewport.Top;
    topLeft.x = 0;
    topLeft.y = static_cast<LONG>(_currentFontSize.Y * viewportLineNumber);

    size_t charLineLength = _viewport.Right - _viewport.Left + 1;
    size_t linesTall = _lineNumberEnd - _lineNumberStart;
    bottomRight.x = static_cast<LONG>(topLeft.x + (_currentFontSize.X * charLineLength));
    bottomRight.y = static_cast<LONG>(topLeft.y + _currentFontSize.Y * linesTall);

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

IFACEMETHODIMP UiaLineTextRange::GetEnclosingElement(_Outptr_result_maybenull_ IRawElementProviderSimple** ppRetVal)
{
    *ppRetVal = _pProvider;
    _pProvider->AddRef();
    return S_OK;
}

IFACEMETHODIMP UiaLineTextRange::GetText(_In_ int maxLength, _Out_ BSTR* pRetVal)
{
    // TODO support multiple lines
    std::wstring wstr = L"";
    const ROW* const row = _pOutputBuffer->GetRowByOffset(static_cast<UINT>(_lineNumberStart));
    if (row->CharRow.ContainsText())
    {
        wstr = std::wstring(row->CharRow.Chars + row->CharRow.Left,
                            row->CharRow.Chars + row->CharRow.Right);
    }
    if (maxLength != -1 && static_cast<size_t>(maxLength) < wstr.size())
    {
        wstr.resize(maxLength);
    }
    *pRetVal = SysAllocString(wstr.c_str());
    return S_OK;
}

// this moves both endpoints at once.
IFACEMETHODIMP UiaLineTextRange::Move(_In_ TextUnit unit, _In_ int count, _Out_ int* pRetVal)
{
    // the smallest unit of movement we currently support is a line,
    // the documentation says to round up the smallest unit of
    // movement that we support if the get a unit smaller than the
    // ones we support.
    if (!_isSupportedTextUnit(unit))
    {
        return E_NOTIMPL;
    }

    // check that we won't be moving past the end of the document.

    // -1 for zero indexing
    int maxLineNumber = _pOutputBuffer->TotalRowCount() - 1;
    if (count < 0)
    {
        *pRetVal = -1 * static_cast<int>(min(_lineNumberStart, abs(count)));
    }
    else
    {
        *pRetVal = static_cast<int>(min(maxLineNumber - _lineNumberStart, count));
    }

    // update endpoint positions
    _lineNumberStart += *pRetVal;
    _lineNumberEnd = _lineNumberStart + 1;
    return S_OK;
}

// TODO fix bugs in this
IFACEMETHODIMP UiaLineTextRange::MoveEndpointByUnit(_In_ TextPatternRangeEndpoint endpoint,
                                                _In_ TextUnit unit,
                                                _In_ int count,
                                                _Out_ int* pRetVal)
{
    // the smallest unit of movement we currently support is a line,
    // the documentation says to round up the smallest unit of
    // movement that we support if the get a unit smaller than the
    // ones we support.
    if (!_isSupportedTextUnit(unit))
    {
        return E_NOTIMPL;
    }

    // get the endpoint that we're moving
    size_t* pInternalEndpoint;
    if (endpoint == TextPatternRangeEndpoint_Start)
    {
        pInternalEndpoint = &_lineNumberStart;
    }
    else
    {
        pInternalEndpoint = &_lineNumberEnd;
    }

    // moving forward
    int maxLineNumber = _pOutputBuffer->TotalRowCount() - 1;
    if (*pInternalEndpoint + count > maxLineNumber)
    {
        *pRetVal = maxLineNumber - static_cast<int>(*pInternalEndpoint);
    }
    // moving backward
    else if (*pInternalEndpoint + count < 0)
    {
        *pRetVal = static_cast<int>(*pInternalEndpoint * -1);
    }
    else
    {
        *pRetVal = count;
    }
    *pInternalEndpoint += *pRetVal;

    // fix the endpoints if they are out of order
    if (_lineNumberStart > _lineNumberEnd)
    {
        // if it was the ending endpoint that we just moved, we need
        // to move the starting one to match it.
        if (endpoint = TextPatternRangeEndpoint_End)
        {
            _lineNumberStart = _lineNumberEnd;
        }
        // vice versa
        else {
            _lineNumberEnd = _lineNumberStart;
        }
    }

    return S_OK;
}

IFACEMETHODIMP UiaLineTextRange::MoveEndpointByRange(_In_ TextPatternRangeEndpoint endpoint,
                                                _In_opt_ ITextRangeProvider* pTargetRange,
                                                _In_ TextPatternRangeEndpoint targetEndpoint)
{
    endpoint; pTargetRange; targetEndpoint;
    return E_NOTIMPL;
}

IFACEMETHODIMP UiaLineTextRange::Select()
{
    return E_NOTIMPL;
}

IFACEMETHODIMP UiaLineTextRange::AddToSelection()
{
    return E_NOTIMPL;
}

IFACEMETHODIMP UiaLineTextRange::RemoveFromSelection()
{
    return E_NOTIMPL;
}

IFACEMETHODIMP UiaLineTextRange::ScrollIntoView(_In_ BOOL alignToTop)
{
    alignToTop;
    return E_NOTIMPL;
}

IFACEMETHODIMP UiaLineTextRange::GetChildren(_Outptr_result_maybenull_ SAFEARRAY** ppRetVal)
{
    ppRetVal;
    return E_NOTIMPL;
}


bool UiaLineTextRange::_isSupportedTextUnit(TextUnit textUnit)
{
    return (textUnit == TextUnit::TextUnit_Character ||
            textUnit == TextUnit::TextUnit_Format ||
            textUnit == TextUnit::TextUnit_Word ||
            textUnit == TextUnit::TextUnit_Line);
}