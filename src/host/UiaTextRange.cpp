
#include "precomp.h"
#include "UiaTextRange.hpp"

int clamp(int val, int low, int high)
{
    return max(low, min(val, high));
}

// degenerate range constructor
UiaTextRange::UiaTextRange(IRawElementProviderSimple* pProvider,
                           const TEXT_BUFFER_INFO* const pOutputBuffer,
                           SMALL_RECT viewport,
                           const COORD currentFontSize) :
    _cRefs{ 1 },
    _pProvider{ pProvider },
    _pOutputBuffer{ pOutputBuffer },
    _viewport{ viewport },
    _currentFontSize{ currentFontSize },
    _start{ 0 },
    _end{ 0 }
{
}

UiaTextRange::UiaTextRange(IRawElementProviderSimple* pProvider,
                           const TEXT_BUFFER_INFO* const pOutputBuffer,
                           SMALL_RECT viewport,
                           const COORD currentFontSize,
                           const int start,
                           const int end) :
    _cRefs{ 1 },
    _pProvider{ pProvider },
    _pOutputBuffer{ pOutputBuffer },
    _viewport{ viewport },
    _currentFontSize{ currentFontSize },
    _start{ start },
    _end{ end }
{
}

/*
UiaTextRange::UiaTextRange(IRawElementProviderSimple* pProvider,
                           const TEXT_BUFFER_INFO* const pOutputBuffer,
                           SMALL_RECT viewport,
                           const COORD currentFontSize,
                           int lineNumberStart,
                           int lineNumberEnd,
                           int charStart,
                           int charEnd) :
    _cRefs{ 1 },
    _pProvider{ pProvider },
    _pOutputBuffer{ pOutputBuffer },
    _viewport{ viewport },
    _currentFontSize{ currentFontSize },
    _lineNumberStart{ lineNumberStart },
    _lineNumberEnd{ lineNumberEnd },
    _charStart{ charStart },
    _charEnd{ charEnd }
{
    if (lineNumberEnd < lineNumberStart)
    {
        throw std::invalid_argument("lineNumberEnd can't be less than lineNumberStart");
    }
    if (lineNumberStart == lineNumberEnd && charStart != charEnd)
    {
        throw std::invalid_argument("degenerate range can't have mismatched char endpoints");
    }
    if (lineNumberStart + 1 == lineNumberEnd && charEnd < charStart)
    {
        throw std::invalid_argument("single line range may not have charEnd less than charStart");
    }
}
*/

UiaTextRange::~UiaTextRange()
{
    _pProvider->Release();
}

const int UiaTextRange::getStart() const
{
    return _start;
}

const int UiaTextRange::getEnd() const
{
    return _end;
}

#pragma region IUnknown

IFACEMETHODIMP_(ULONG) UiaTextRange::AddRef()
{
    return InterlockedIncrement(&_cRefs);
}

IFACEMETHODIMP_(ULONG) UiaTextRange::Release()
{
    long val = InterlockedDecrement(&_cRefs);
    if (val == 0)
    {
        delete this;
    }
    return val;
}

IFACEMETHODIMP UiaTextRange::QueryInterface(_In_ REFIID riid, _COM_Outptr_result_maybenull_ void** ppInterface)
{
    if (riid == __uuidof(IUnknown))
    {
        *ppInterface = static_cast<ITextRangeProvider*>(this);
    }
    else if (riid == __uuidof(ITextRangeProvider))
    {
        *ppInterface = static_cast<ITextRangeProvider*>(this);
    }
    else
    {
        *ppInterface = nullptr;
        return E_NOINTERFACE;
    }

    (static_cast<IUnknown*>(*ppInterface))->AddRef();
    return S_OK;
}

#pragma endregion

#pragma region ITextRangeProvider

IFACEMETHODIMP UiaTextRange::Clone(_Outptr_result_maybenull_ ITextRangeProvider** ppRetVal)
{
    try
    {
        *ppRetVal = new UiaTextRange(*this);
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

IFACEMETHODIMP UiaTextRange::Compare(_In_opt_ ITextRangeProvider* pRange, _Out_ BOOL* pRetVal)
{
    *pRetVal = FALSE;
    UiaTextRange* other = dynamic_cast<UiaTextRange*>(pRange);
    if (other)
    {
        *pRetVal = !!(_start == other->getStart() &&
                      _end == other->getEnd());
    }
    return S_OK;
}


IFACEMETHODIMP UiaTextRange::CompareEndpoints(TextPatternRangeEndpoint endpoint,
                                            _In_opt_ ITextRangeProvider* pTargetRange,
                                            _In_ TextPatternRangeEndpoint targetEndpoint,
                                            _Out_ int* pRetVal)
{
    // get the text range that we're comparing to
    UiaTextRange* range = dynamic_cast<UiaTextRange*>(pTargetRange);
    if (range == nullptr)
    {
        return E_NOINTERFACE;
    }

    // get endpoint value that we're comparing to
    int theirValue;
    if (targetEndpoint == TextPatternRangeEndpoint::TextPatternRangeEndpoint_Start)
    {
        theirValue = range->getStart();
    }
    else
    {
        theirValue = range->getEnd();
    }

    // get the values of our endpoint
    int ourValue;
    if (endpoint == TextPatternRangeEndpoint::TextPatternRangeEndpoint_Start)
    {
        ourValue = _start;
    }
    else
    {
        ourValue = _end;
    }

    // compare them
    *pRetVal = clamp(ourValue - theirValue, -1, 1);

    return S_OK;
}

IFACEMETHODIMP UiaTextRange::ExpandToEnclosingUnit(_In_ TextUnit unit)
{
    unit;
    return E_NOTIMPL;
}

IFACEMETHODIMP UiaTextRange::FindAttribute(_In_ TEXTATTRIBUTEID textAttributeId,
                                        _In_ VARIANT val,
                                        _In_ BOOL searchBackward,
                                        _Outptr_result_maybenull_ ITextRangeProvider** ppRetVal)
{
    textAttributeId; val; searchBackward; ppRetVal;
    return E_NOTIMPL;
}

IFACEMETHODIMP UiaTextRange::FindText(_In_ BSTR text,
                                    BOOL searchBackward,
                                    BOOL ignoreCase,
                                    _Outptr_result_maybenull_ ITextRangeProvider** ppRetVal)
{
    text; searchBackward; ignoreCase; ppRetVal;
    return E_NOTIMPL;
}

IFACEMETHODIMP UiaTextRange::GetAttributeValue(_In_ TEXTATTRIBUTEID textAttributeId, _Out_ VARIANT* pRetVal)
{
    textAttributeId; pRetVal;
    return E_NOTIMPL;
}

// TODO test this
IFACEMETHODIMP UiaTextRange::GetBoundingRectangles(_Outptr_result_maybenull_ SAFEARRAY** ppRetVal)
{
    // degenerate range returns an empty safearray
    if (_isDegenerate())
    {
        *ppRetVal = SafeArrayCreateVector(VT_R8, 0, 0);
        return S_OK;
    }

    // vector to put coords into. they go in as four doubles in the
    // order: left, top, width, height. each line will have its own
    // set of coords.
    std::vector<double> coords;

    const COORD screenBufferCoords = g_ciConsoleInformation.GetScreenBufferSize();
    const int totalLines = screenBufferCoords.Y;
    const int startLine = _endpointToRow(_start);
    const int endLine = _endpointToRow(_end);
    //const int totalLinesInRange = (endLine - startLine + totalLines) % totalLines;
    const int totalLinesInRange = (endLine > startLine) ? endLine - startLine : endLine - startLine + totalLines;
    // width of viewport (measured in chars)
    const int viewportWidth = _viewport.Right - _viewport.Left + 1;

    for (int i = 0; i < totalLinesInRange; ++i)
    {
        const int currentLineNumber = (startLine + i) % totalLines;
        if (!_isLineInViewport(currentLineNumber))
        {
            continue;
        }

        // measure boundaries, in pixels
        POINT topLeft;
        POINT bottomRight;

        if (i == 0)
        {
            // special logic for first line since we might not start at
            // the left edge of the viewport
            topLeft.x = _endpointToColumn(_start) * _currentFontSize.X;
        }
        else
        {
            topLeft.x = 0;
        }

        topLeft.y = _lineNumberToViewport(currentLineNumber) * _currentFontSize.Y;

        if (i + 1 == totalLinesInRange)
        {
            // special logic for last line since we might not end at
            // the right edge of the viewport. We need to subtract 1
            // from end because it it not an inclusive range and this
            // might affect what line _endpointToColumn thinks the
            // endpoint lays on but we need to add 1 back to the to
            // total because we want the bounding rectangle to
            // encompass that column.
            bottomRight.x = (_endpointToColumn(_end - 1) + 1) * _currentFontSize.X;
        }
        else
        {
            // we're a full line, so we go the full width of the
            // viewport
            bottomRight.x = viewportWidth * _currentFontSize.X;
        }

        // +1 because we are adding each line individually
        bottomRight.y = topLeft.y + (1 * _currentFontSize.Y);

        // convert the coords to be relative to the screen instead of
        // the client window
        HWND hwnd = g_ciConsoleInformation.hWnd;
        ClientToScreen(hwnd, &topLeft);
        ClientToScreen(hwnd, &bottomRight);

        // insert the coords
        LONG width = bottomRight.x - topLeft.x;
        LONG height = bottomRight.y - topLeft.y;
        coords.push_back(topLeft.x);
        coords.push_back(topLeft.y);
        coords.push_back(width);
        coords.push_back(height);
    }

    // convert to a safearray
    *ppRetVal = SafeArrayCreateVector(VT_R8, 0, static_cast<ULONG>(coords.size()));
    for (LONG i = 0; i < coords.size(); ++i)
    {
        SafeArrayPutElement(*ppRetVal, &i, &coords[i]);
    }

    return S_OK;
}

IFACEMETHODIMP UiaTextRange::GetEnclosingElement(_Outptr_result_maybenull_ IRawElementProviderSimple** ppRetVal)
{
    *ppRetVal = _pProvider;
    _pProvider->AddRef();
    return S_OK;
}

IFACEMETHODIMP UiaTextRange::GetText(_In_ int maxLength, _Out_ BSTR* pRetVal)
{
    std::wstring wstr = L"";
    const bool getPartialText = maxLength != -1;
    const COORD screenBufferCoords = g_ciConsoleInformation.GetScreenBufferSize();
    const int totalLines = screenBufferCoords.Y;
    const int startLine = _endpointToRow(_start);
    const int endLine = _endpointToRow(_end);
    const int totalLinesInRange = (endLine > startLine) ? endLine - startLine : endLine - startLine + totalLines;

    for (int i = 0; i < totalLinesInRange; ++i)
    {
        const ROW* const row = _pOutputBuffer->GetRowByOffset(startLine + i);
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

IFACEMETHODIMP UiaTextRange::Move(_In_ TextUnit unit, _In_ int count, _Out_ int* pRetVal)
{
    unit; count; pRetVal;
    return E_NOTIMPL;
}

IFACEMETHODIMP UiaTextRange::MoveEndpointByUnit(_In_ TextPatternRangeEndpoint endpoint,
                                                _In_ TextUnit unit,
                                                _In_ int count,
                                                _Out_ int* pRetVal)
{
    endpoint; unit; count; pRetVal;
    return E_NOTIMPL;
}

IFACEMETHODIMP UiaTextRange::MoveEndpointByRange(_In_ TextPatternRangeEndpoint endpoint,
                                                _In_opt_ ITextRangeProvider* pTargetRange,
                                                _In_ TextPatternRangeEndpoint targetEndpoint)
{
    endpoint; pTargetRange; targetEndpoint;
    return E_NOTIMPL;
}

IFACEMETHODIMP UiaTextRange::Select()
{

    return E_NOTIMPL;
}

IFACEMETHODIMP UiaTextRange::AddToSelection()
{
    return E_NOTIMPL;
}

IFACEMETHODIMP UiaTextRange::RemoveFromSelection()
{
    return E_NOTIMPL;
}

IFACEMETHODIMP UiaTextRange::ScrollIntoView(_In_ BOOL alignToTop)
{
    alignToTop;
    return E_NOTIMPL;
}

IFACEMETHODIMP UiaTextRange::GetChildren(_Outptr_result_maybenull_ SAFEARRAY** ppRetVal)
{
    ppRetVal;
    return E_NOTIMPL;
}

#pragma endregion

bool UiaTextRange::_isDegenerate()
{
    return (_start == _end);
}

// TODO find a better way to do this
bool UiaTextRange::_isLineInViewport(int lineNumber)
{
    const int viewportHeight = _viewport.Bottom - _viewport.Top + 1;
    const COORD screenBufferCoords = g_ciConsoleInformation.GetScreenBufferSize();

    for (int i = 0; i < viewportHeight; ++i)
    {
        int currentLineNumber = (_viewport.Top + i) % screenBufferCoords.Y;
        if (currentLineNumber == lineNumber)
        {
            return true;
        }
    }
    return false;
}

int UiaTextRange::_lineNumberToViewport(int lineNumber)
{
    const int viewportHeight = _viewport.Bottom - _viewport.Top + 1;
    const COORD screenBufferCoords = g_ciConsoleInformation.GetScreenBufferSize();

    for (int i = 0; i < viewportHeight; ++i)
    {
        int currentLineNumber = (_viewport.Top + i) % screenBufferCoords.Y;
        if (currentLineNumber == lineNumber)
        {
            return i;
        }
    }
    // TODO better failure value
    return -1;

}

const int UiaTextRange::_endpointToRow(const int endpoint)
{
    const COORD screenBufferCoords = g_ciConsoleInformation.GetScreenBufferSize();
    return endpoint / screenBufferCoords.X;
}

const int UiaTextRange::_endpointToColumn(const int endpoint)
{
    const COORD screenBufferCoords = g_ciConsoleInformation.GetScreenBufferSize();
    return endpoint % screenBufferCoords.X;
}