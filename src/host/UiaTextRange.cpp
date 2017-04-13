
#include "precomp.h"
#include "UiaTextRange.hpp"
#include "window.hpp"

// returns val if (low <= val <= high)
// returns low if (val < low)
// returns high if (val > high)
int clamp(int val, int low, int high)
{
    ASSERT(low <= high);
    return max(low, min(val, high));
}

// degenerate range constructor
UiaTextRange::UiaTextRange(IRawElementProviderSimple* pProvider,
                           const TEXT_BUFFER_INFO* const pOutputBuffer,
                           SCREEN_INFORMATION* const pScreenInfo,
                           const COORD currentFontSize) :
    _cRefs{ 1 },
    _pProvider{ pProvider },
    _pOutputBuffer{ pOutputBuffer },
    _pScreenInfo{ pScreenInfo },
    _currentFontSize{ currentFontSize },
    _start{ 0 },
    _end{ 0 }
{
}

UiaTextRange::UiaTextRange(IRawElementProviderSimple* pProvider,
                           const TEXT_BUFFER_INFO* const pOutputBuffer,
                           SCREEN_INFORMATION* const pScreenInfo,
                           const COORD currentFontSize,
                           const int start,
                           const int end) :
    _cRefs{ 1 },
    _pProvider{ pProvider },
    _pOutputBuffer{ pOutputBuffer },
    _pScreenInfo{ pScreenInfo },
    _currentFontSize{ currentFontSize },
    _start{ start },
    _end{ end }
{
}

// returns a degenerate text range of the start of the line closest to the y value of point
UiaTextRange::UiaTextRange(IRawElementProviderSimple* pProvider,
                           const TEXT_BUFFER_INFO* const pOutputBuffer,
                           SCREEN_INFORMATION* const pScreenInfo,
                           const COORD currentFontSize,
                           const UiaPoint point) :
    _cRefs{ 1 },
    _pProvider{ pProvider },
    _pOutputBuffer{ pOutputBuffer },
    _pScreenInfo{ pScreenInfo },
    _currentFontSize{ currentFontSize }
{

    POINT clientPoint;
    clientPoint.x = static_cast<LONG>(point.x);
    clientPoint.y = static_cast<LONG>(point.y);
    // get row that point resides in
    const RECT windowRect = g_ciConsoleInformation.pWindow->GetWindowRect();
    const SMALL_RECT viewport = _getViewport();
    int row;
    if (clientPoint.y <= windowRect.top)
    {
        row = viewport.Top;
    }
    else if (clientPoint.y >= windowRect.bottom)
    {
        row = viewport.Bottom;
    }
    else
    {
        // change point coords to pixels relative to window
        HWND hwnd = g_ciConsoleInformation.hWnd;
        ScreenToClient(hwnd, &clientPoint);

        row = (clientPoint.y / currentFontSize.Y) + viewport.Top;
    }
    _start = _rowToEndpoint(row);
	_end = _start;
}

UiaTextRange::UiaTextRange(const UiaTextRange& a) :
    _cRefs{ 1 },
    _pProvider{ a._pProvider },
    _pOutputBuffer{ a._pOutputBuffer },
    _pScreenInfo{ a._pScreenInfo },
    _currentFontSize{ a._currentFontSize },
    _start{ a._start },
    _end{ a._end }

{
    _pProvider->AddRef();
}

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
    const COORD screenBufferCoords = g_ciConsoleInformation.GetScreenBufferSize();
    const int totalLines = screenBufferCoords.Y;
    const int startLine = _endpointToRow(_start);
    const int endLine = _endpointToRow(_end);
    const int totalLinesInRange = (endLine >= startLine) ? endLine - startLine : endLine - startLine + totalLines;
    // width of viewport (measured in chars)
    const SMALL_RECT viewport = _getViewport();
    const int viewportWidth = viewport.Right - viewport.Left + 1;

    // vector to put coords into. they go in as four doubles in the
    // order: left, top, width, height. each line will have its own
    // set of coords.
    std::vector<double> coords;

    if (_isDegenerate() && _isLineInViewport(startLine))
    {
        POINT topLeft;
        POINT bottomRight;
        topLeft.x = _endpointToColumn(_start) * _currentFontSize.X;
        topLeft.y = _lineNumberToViewport(startLine) * _currentFontSize.Y;

        bottomRight.x = topLeft.x;
        bottomRight.y = topLeft.y + _currentFontSize.Y;
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

        // + 1 of the font height because we are adding each line individually
        bottomRight.y = topLeft.y + _currentFontSize.Y;

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
    // for now, we only support line movement
    // TODO add more

	*pRetVal = 0;
    if (count == 0)
    {
        return S_OK;
    }

    const int totalRows = _getTotalRows();
    const int topRow = _pOutputBuffer->GetFirstRowIndex();
    const int bottomRow = (topRow - 1 + totalRows) % totalRows;
    const int currentStartRow = _endpointToRow(_start);

    int currentRow = currentStartRow;

    // moving forward
    int incrementAmount = 1;
    int limitingRow = bottomRow;
    if (count < 0)
    {
        // moving backward
        incrementAmount = -1;
        limitingRow = topRow;
    }

    for (int i = 0; i < abs(count); ++i)
    {
        if (currentRow == limitingRow)
        {
            break;
        }
        currentRow = (currentRow + incrementAmount + totalRows) % totalRows;
        *pRetVal += incrementAmount;
    }

    // update endpoints
    const int rowWidth = _getRowWidth();
    _start = currentRow * rowWidth;
    _end = _start + rowWidth;



    UNREFERENCED_PARAMETER(unit);
    return S_OK;
}

IFACEMETHODIMP UiaTextRange::MoveEndpointByUnit(_In_ TextPatternRangeEndpoint endpoint,
                                                _In_ TextUnit unit,
                                                _In_ int count,
                                                _Out_ int* pRetVal)
{
    // for now, we only support line movement
    // TODO add more
    UNREFERENCED_PARAMETER(unit);

    *pRetVal = 0;
    if (count == 0)
    {
        return S_OK;
    }

    const int totalRows = _getTotalRows();
    const int topRow = _pOutputBuffer->GetFirstRowIndex();
    const int bottomRow = (topRow - 1 + totalRows) % totalRows;
    const bool isDegenerate = _isDegenerate();
    const bool shrinkingRange = (count < 0 &&
                                 endpoint == TextPatternRangeEndpoint::TextPatternRangeEndpoint_End) ||
                                (count > 0 &&
                                 endpoint == TextPatternRangeEndpoint::TextPatternRangeEndpoint_Start);


    // determine which endpoint we're moving
    int* pInternalEndpoint;
    int otherEndpoint;
    if (endpoint == TextPatternRangeEndpoint::TextPatternRangeEndpoint_Start)
    {
        pInternalEndpoint = &_start;
        otherEndpoint = _end;
    }
    else
    {
        pInternalEndpoint = &_end;
        otherEndpoint = _start;
    }
    const int currentStartRow = _endpointToRow(*pInternalEndpoint);
    int currentRow = currentStartRow;

    // set values depending on move direction

    // moving forward
    int incrementAmount = 1;
    int limitingRow = bottomRow;
    if (count < 0)
    {
        // moving backward
        incrementAmount = -1;
        limitingRow = topRow;
    }

    // move the endpoint
    bool crossedEndpoints = false;
    for (int i = 0; i < abs(count); ++i)
    {
        if (currentRow == limitingRow)
        {
            break;
        }
        if (currentRow == otherEndpoint)
        {
            crossedEndpoints = true;
        }
        currentRow = (currentRow + incrementAmount + totalRows) % totalRows;
        *pRetVal += incrementAmount;
    }
    const int rowWidth = _getRowWidth();
    *pInternalEndpoint = currentRow * rowWidth;

    // fix out of order endpoints
    if (crossedEndpoints || (isDegenerate && shrinkingRange))
    {
        if (endpoint == TextPatternRangeEndpoint::TextPatternRangeEndpoint_Start)
        {
            _end = _start;
        }
        else
        {
            _start = _end;
        }
    }
    return S_OK;
}

IFACEMETHODIMP UiaTextRange::MoveEndpointByRange(_In_ TextPatternRangeEndpoint endpoint,
                                                _In_opt_ ITextRangeProvider* pTargetRange,
                                                _In_ TextPatternRangeEndpoint targetEndpoint)
{
    UiaTextRange* range = dynamic_cast<UiaTextRange*>(pTargetRange);
    if (range == nullptr)
    {
        return E_NOINTERFACE;
    }

    // get the value that we're updating to
    int newValue;
    if (targetEndpoint == TextPatternRangeEndpoint::TextPatternRangeEndpoint_Start)
    {
        newValue = range->getStart();
    }
    else
    {
        newValue = range->getEnd();
    }

    // get the endpoint that we're changing
    int* pInternalEndpoint;
    if (endpoint == TextPatternRangeEndpoint::TextPatternRangeEndpoint_Start)
    {
        pInternalEndpoint = &_start;
    }
    else
    {
        pInternalEndpoint = &_end;
    }

    // update value, fix any reversed endpoints
    *pInternalEndpoint = newValue;
    if (_start > _end)
    {
        // we move the endpoint that isn't being updated to be the
        // same as the one that was just moved
        if (endpoint == TextPatternRangeEndpoint::TextPatternRangeEndpoint_Start)
        {
            _end = _start;
        }
        else
        {
            _start = _end;
        }
    }
    return S_OK;
}

IFACEMETHODIMP UiaTextRange::Select()
{
    COORD coordStart;
    COORD coordEnd;

    coordStart.X = static_cast<SHORT>(_endpointToColumn(_start));
    coordStart.Y = static_cast<SHORT>(_endpointToRow(_start));

    coordEnd.X = static_cast<SHORT>(_endpointToColumn(_end - 1));
    coordEnd.Y = static_cast<SHORT>(_endpointToRow(_end - 1));

    Selection::Instance().SelectNewRegion(coordStart, coordEnd);
    return S_OK;
}

// we don't support this
IFACEMETHODIMP UiaTextRange::AddToSelection()
{
    return E_NOTIMPL;
}

// we don't support this
IFACEMETHODIMP UiaTextRange::RemoveFromSelection()
{
    return E_NOTIMPL;
}

IFACEMETHODIMP UiaTextRange::ScrollIntoView(_In_ BOOL alignToTop)
{
    if (!_isLineInViewport(_endpointToRow(_start)))
    {
        const SMALL_RECT oldViewport = _getViewport();
        SMALL_RECT newViewport;
        const short viewportHeight = oldViewport.Bottom - oldViewport.Top;
        const int topRow = _endpointToRow(_start);

        newViewport.Left = oldViewport.Left;
        newViewport.Right = oldViewport.Right;
        newViewport.Top = oldViewport.Top;
        newViewport.Bottom = oldViewport.Bottom;
        if (alignToTop)
        {
            newViewport.Top = static_cast<SHORT>(topRow);
            newViewport.Bottom = newViewport.Top + viewportHeight;
        }
        else
        {
            // TODO this is buggy
            while (!_isLineInViewport(topRow, newViewport))
            {
                newViewport.Top += 1;
                newViewport.Bottom = newViewport.Top + viewportHeight;
            }
        }

        g_ciConsoleInformation.pWindow->SetViewportOrigin(newViewport);
    }

    return S_OK;
}

IFACEMETHODIMP UiaTextRange::GetChildren(_Outptr_result_maybenull_ SAFEARRAY** ppRetVal)
{
    *ppRetVal = SafeArrayCreateVector(VT_UNKNOWN, 0, 0);
    return S_OK;
}

#pragma endregion

bool UiaTextRange::_isDegenerate()
{
    return (_start == _end);
}

bool UiaTextRange::_isLineInViewport(const int lineNumber)
{
    const SMALL_RECT viewport = _getViewport();
    return _isLineInViewport(lineNumber, viewport);
}

// TODO find a better way to do this
bool UiaTextRange::_isLineInViewport(const int lineNumber, const SMALL_RECT viewport)
{
    const int viewportHeight = viewport.Bottom - viewport.Top + 1;
    const COORD screenBufferCoords = g_ciConsoleInformation.GetScreenBufferSize();

    for (int i = 0; i < viewportHeight; ++i)
    {
        int currentLineNumber = (viewport.Top + i) % screenBufferCoords.Y;
        if (currentLineNumber == lineNumber)
        {
            return true;
        }
    }
    return false;
}

int UiaTextRange::_lineNumberToViewport(int lineNumber)
{
    const SMALL_RECT viewport = _getViewport();
    const int viewportHeight = viewport.Bottom - viewport.Top + 1;
    const COORD screenBufferCoords = g_ciConsoleInformation.GetScreenBufferSize();

    for (int i = 0; i < viewportHeight; ++i)
    {
        int currentLineNumber = (viewport.Top + i) % screenBufferCoords.Y;
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

const int UiaTextRange::_rowToEndpoint(const int row)
{
    const COORD screenBufferCoords = g_ciConsoleInformation.GetScreenBufferSize();
    return row * screenBufferCoords.X;
}


const int UiaTextRange::_getTotalRows() const
{
    const COORD screenBufferCoords = g_ciConsoleInformation.GetScreenBufferSize();
    const int totalRows = screenBufferCoords.Y;
    return totalRows;
}

const int UiaTextRange::_getRowWidth() const
{
    const COORD screenBufferCoords = g_ciConsoleInformation.GetScreenBufferSize();
    const int rowWidth = screenBufferCoords.X;
    return rowWidth;

}

const SMALL_RECT UiaTextRange::_getViewport() const
{
    return _pScreenInfo->GetBufferViewport();
}