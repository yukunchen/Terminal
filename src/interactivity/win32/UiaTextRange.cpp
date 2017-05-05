/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "UiaTextRange.hpp"
#include "../inc/ServiceLocator.hpp"

#include "window.hpp"
#include "windowdpiapi.hpp"

using namespace Microsoft::Console::Interactivity::Win32;

#if _DEBUG
unsigned long long UiaTextRange::id = 0;
#endif

// returns val if (low <= val <= high)
// returns low if (val < low)
// returns high if (val > high)
int clamp(int val, int low, int high)
{
    ASSERT(low <= high);
    return max(low, min(val, high));
}

// degenerate range constructor
UiaTextRange::UiaTextRange(_In_ IRawElementProviderSimple* const pProvider,
                           _In_ const TEXT_BUFFER_INFO* const pOutputBuffer,
                           _In_ const SCREEN_INFORMATION* const pScreenInfo,
                           _In_ const COORD currentFontSize) :
    _cRefs{ 1 },
    _pProvider{ THROW_HR_IF_NULL(E_INVALIDARG, pProvider) },
    _pOutputBuffer{ THROW_HR_IF_NULL(E_INVALIDARG, pOutputBuffer) },
    _pScreenInfo{ THROW_HR_IF_NULL(E_INVALIDARG, pScreenInfo) },
    _currentFontSize{ currentFontSize },
    _start{ 0 },
    _end{ 0 }
{
#if _DEBUG
   _id = id;
   ++id;
#endif
}

UiaTextRange::UiaTextRange(_In_ IRawElementProviderSimple* const pProvider,
                           _In_ const TEXT_BUFFER_INFO* const pOutputBuffer,
                           _In_ const SCREEN_INFORMATION* const pScreenInfo,
                           _In_ const COORD currentFontSize,
                           _In_ const int start,
                           _In_ const int end) :
    UiaTextRange(pProvider, pOutputBuffer, pScreenInfo, currentFontSize)
{
    _start = start;
    _end = end;
}

// returns a degenerate text range of the start of the line closest to the y value of point
UiaTextRange::UiaTextRange(_In_ IRawElementProviderSimple* const pProvider,
                           _In_ const TEXT_BUFFER_INFO* const pOutputBuffer,
                           _In_ const SCREEN_INFORMATION* const pScreenInfo,
                           _In_ const COORD currentFontSize,
                           _In_ const UiaPoint point) :
    UiaTextRange(pProvider, pOutputBuffer, pScreenInfo, currentFontSize)
{
    POINT clientPoint;
    clientPoint.x = static_cast<LONG>(point.x);
    clientPoint.y = static_cast<LONG>(point.y);
    // get row that point resides in
    const RECT windowRect = _getWindow()->GetWindowRect();
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
        HWND hwnd = _getWindowHandle();
        ScreenToClient(hwnd, &clientPoint);
        row = (clientPoint.y / currentFontSize.Y) + viewport.Top;
    }
    _start = _rowToEndpoint(row);
    _end = _start;
}

UiaTextRange::UiaTextRange(_In_ const UiaTextRange& a) :
    _cRefs{ 1 },
    _pProvider{ a._pProvider },
    _pOutputBuffer{ a._pOutputBuffer },
    _pScreenInfo{ a._pScreenInfo },
    _currentFontSize{ a._currentFontSize },
    _start{ a._start },
    _end{ a._end }

{
    (static_cast<IUnknown*>(_pProvider))->AddRef();
#if _DEBUG
   _id = id;
   ++id;
#endif
}

UiaTextRange::~UiaTextRange()
{
    (static_cast<IUnknown*>(_pProvider))->Release();
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
    const long val = InterlockedDecrement(&_cRefs);
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
    UiaTextRange* other = static_cast<UiaTextRange*>(pRange);
    if (other)
    {
        *pRetVal = !!(_start == other->getStart() &&
                      _end == other->getEnd());
    }
    return S_OK;
}


IFACEMETHODIMP UiaTextRange::CompareEndpoints(_In_ TextPatternRangeEndpoint endpoint,
                                            _In_ ITextRangeProvider* pTargetRange,
                                            _In_ TextPatternRangeEndpoint targetEndpoint,
                                            _Out_ int* pRetVal)
{
    // get the text range that we're comparing to
    UiaTextRange* range = static_cast<UiaTextRange*>(pTargetRange);
    if (range == nullptr)
    {
        return E_INVALIDARG;
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
    const int totalRows = _getTotalRows();
    const int topRow = _pOutputBuffer->GetFirstRowIndex();
    const int bottomRow = (topRow - 1 + totalRows) % totalRows;
    const int lineWidth = _getScreenBufferCoords().X;

    if (unit <= TextUnit::TextUnit_Line)
    {
        // expand to line
        _start = _rowToEndpoint(_endpointToRow(_start));
        _end = _start + lineWidth;
    }
    else
    {
        // expand to document
        _start = _rowToEndpoint(topRow);
        _end = _rowToEndpoint(bottomRow + 1);
    }

    return S_OK;
}

// we don't support this currently
IFACEMETHODIMP UiaTextRange::FindAttribute(_In_ TEXTATTRIBUTEID textAttributeId,
                                        _In_ VARIANT val,
                                        _In_ BOOL searchBackward,
                                        _Outptr_result_maybenull_ ITextRangeProvider** ppRetVal)
{
    UNREFERENCED_PARAMETER(textAttributeId);
    UNREFERENCED_PARAMETER(val);
    UNREFERENCED_PARAMETER(searchBackward);
    UNREFERENCED_PARAMETER(ppRetVal);
    return E_NOTIMPL;
}

// we don't support this currently
IFACEMETHODIMP UiaTextRange::FindText(_In_ BSTR text,
                                      _In_ BOOL searchBackward,
                                      _In_ BOOL ignoreCase,
                                      _Outptr_result_maybenull_ ITextRangeProvider** ppRetVal)
{
    UNREFERENCED_PARAMETER(text);
    UNREFERENCED_PARAMETER(searchBackward);
    UNREFERENCED_PARAMETER(ignoreCase);
    UNREFERENCED_PARAMETER(ppRetVal);
    return E_NOTIMPL;
}

// we don't support this currently
IFACEMETHODIMP UiaTextRange::GetAttributeValue(_In_ TEXTATTRIBUTEID textAttributeId,
                                               _Out_ VARIANT* pRetVal)
{
    UNREFERENCED_PARAMETER(textAttributeId);
    UNREFERENCED_PARAMETER(pRetVal);
    return E_NOTIMPL;
}

IFACEMETHODIMP UiaTextRange::GetBoundingRectangles(_Outptr_result_maybenull_ SAFEARRAY** ppRetVal)
{
    const COORD screenBufferCoords = _getScreenBufferCoords();
    const int totalLines = screenBufferCoords.Y;
    const int startLine = _endpointToRow(_start);
    const int endLine = _endpointToRow(_end);
    const int totalLinesInRange = (endLine >= startLine) ? endLine - startLine : endLine - startLine + totalLines;
    // width of viewport (measured in chars)
    const SMALL_RECT viewport = _getViewport();
    const int viewportWidth = viewport.Right - viewport.Left + 1;

    *ppRetVal = nullptr;

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
        HWND hwnd = _getWindowHandle();
        ClientToScreen(hwnd, &topLeft);
        ClientToScreen(hwnd, &bottomRight);

        // take window dpi into account
        // TODO: MSFT 7960168 need to figure out window dpi weirdness
        // in narrator

        // insert the coords
        const LONG width = bottomRight.x - topLeft.x;
        const LONG height = bottomRight.y - topLeft.y;
        try
        {
            coords.push_back(topLeft.x);
            coords.push_back(topLeft.y);
            coords.push_back(width);
            coords.push_back(height);
        }
        CATCH_RETURN();
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
            // from end because it is not an inclusive range and this
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
        HWND hwnd = _getWindowHandle();
        ClientToScreen(hwnd, &topLeft);
        ClientToScreen(hwnd, &bottomRight);

        // insert the coords
        const LONG width = bottomRight.x - topLeft.x;
        const LONG height = bottomRight.y - topLeft.y;
        try
        {
            coords.push_back(topLeft.x);
            coords.push_back(topLeft.y);
            coords.push_back(width);
            coords.push_back(height);
        }
        CATCH_RETURN();
    }

    // convert to a safearray
    *ppRetVal = SafeArrayCreateVector(VT_R8, 0, static_cast<ULONG>(coords.size()));
    for (LONG i = 0; i < static_cast<LONG>(coords.size()); ++i)
    {
        SafeArrayPutElement(*ppRetVal, &i, &coords[i]);
    }

    return S_OK;
}

IFACEMETHODIMP UiaTextRange::GetEnclosingElement(_Outptr_result_maybenull_ IRawElementProviderSimple** ppRetVal)
{
    return _pProvider->QueryInterface(__uuidof(IRawElementProviderSimple),
                                      reinterpret_cast<void**>(ppRetVal));
}

IFACEMETHODIMP UiaTextRange::GetText(_In_ int maxLength, _Out_ BSTR* pRetVal)
{
    std::wstring wstr = L"";
    const bool getPartialText = maxLength != -1;
    const COORD screenBufferCoords = _getScreenBufferCoords();
    const int totalLines = screenBufferCoords.Y;
    const int startLine = _endpointToRow(_start);
    const int endLine = _endpointToRow(_end);
    const int totalLinesInRange = (endLine > startLine) ? endLine - startLine : endLine - startLine + totalLines;

    for (int i = 0; i < totalLinesInRange; ++i)
    {
        try
        {
            const ROW* const row = _pOutputBuffer->GetRowByOffset(startLine + i);
            if (row->CharRow.ContainsText())
            {
                std::wstring tempString = std::wstring(row->CharRow.Chars + row->CharRow.Left,
                                                    row->CharRow.Chars + row->CharRow.Right);
                wstr += tempString;
            }
            wstr += L"\r\n";
            if (getPartialText && wstr.size() > static_cast<size_t>(maxLength))
            {
                wstr.resize(maxLength);
                break;
            }
        }
        CATCH_RETURN();
    }

    *pRetVal = SysAllocString(wstr.c_str());
    return S_OK;
}

IFACEMETHODIMP UiaTextRange::Move(_In_ TextUnit unit, _In_ int count, _Out_ int* pRetVal)
{
    // for now, we only support line movement
    UNREFERENCED_PARAMETER(unit);

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

    int incrementAmount;
    int limitingRow;
    if (count > 0)
    {
        // moving forward
        incrementAmount = 1;
        limitingRow = bottomRow;
    }
    else
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
   _start = _rowToEndpoint(currentRow);
   const int nextRow = (currentRow + 1 + totalRows) % totalRows;
   _end = _rowToEndpoint(nextRow);

    return S_OK;
}

IFACEMETHODIMP UiaTextRange::MoveEndpointByUnit(_In_ TextPatternRangeEndpoint endpoint,
                                                _In_ TextUnit unit,
                                                _In_ int count,
                                                _Out_ int* pRetVal)
{
    // for now, we only support line movement
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
    int currentRow = _endpointToRow(*pInternalEndpoint);

    // set values depending on move direction
    int incrementAmount;
    int limitingRow;
    if (count > 0)
    {
        // moving forward
        incrementAmount = 1;
        limitingRow = bottomRow;
    }
    else
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
        currentRow = (currentRow + incrementAmount + totalRows) % totalRows;
        *pRetVal += incrementAmount;
        if (currentRow == otherEndpoint)
        {
            crossedEndpoints = true;
        }
    }
    const int rowWidth = _getRowWidth();
    *pInternalEndpoint = currentRow * rowWidth;

    // fix out of order endpoints. If they crossed then the it is
    // turned into a degenerate range at the point where the endpoint
    // we moved stops at.
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
                                                 _In_ ITextRangeProvider* pTargetRange,
                                                 _In_ TextPatternRangeEndpoint targetEndpoint)
{
    UiaTextRange* range = static_cast<UiaTextRange*>(pTargetRange);
    if (range == nullptr)
    {
        return E_INVALIDARG;
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

    // -1 because end is an exclusive endpoint
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
        const short viewportHeight = oldViewport.Bottom - oldViewport.Top + 1;
        const COORD screenBufferCoords = _getScreenBufferCoords();
        const int totalRows = screenBufferCoords.Y;
        const int startRow = _endpointToRow(_start);
        const int endRow = _endpointToRow(_end);
        const int topRow = _pOutputBuffer->GetFirstRowIndex();
        const int bottomRow = (topRow - 1 + totalRows) % totalRows;

        // create a new viewport at the top of the screen buffer
        newViewport.Left = oldViewport.Left;
        newViewport.Right = oldViewport.Right;
        newViewport.Top = static_cast<SHORT>(topRow);
        // -1 because SMALL_RECTs are inclusive on both sides
        newViewport.Bottom = static_cast<SHORT>(((viewportHeight - 1) + topRow) % totalRows);

        // shift the viewport down one line at a time until we have
        // the range at the correct edge of the viewport or we hit
        // the bottom edge of the screen buffer
        if (alignToTop)
        {
            while (newViewport.Top != startRow && newViewport.Bottom != bottomRow)
            {
                newViewport.Top = (newViewport.Top + 1) % totalRows;
                newViewport.Bottom = (newViewport.Bottom + 1) % totalRows;
            }
        }
        else
        {
            while (newViewport.Bottom != endRow && newViewport.Bottom != bottomRow)
            {
                newViewport.Top = (newViewport.Top + 1) % totalRows;
                newViewport.Bottom = (newViewport.Bottom + 1) % totalRows;
            }
        }

        _getWindow()->SetViewportOrigin(newViewport);
    }

    return S_OK;
}

IFACEMETHODIMP UiaTextRange::GetChildren(_Outptr_result_maybenull_ SAFEARRAY** ppRetVal)
{
    // we don't have any children
    *ppRetVal = SafeArrayCreateVector(VT_UNKNOWN, 0, 0);
    return S_OK;
}

#pragma endregion

const bool UiaTextRange::_isDegenerate() const
{
    return (_start == _end);
}

const bool UiaTextRange::_isLineInViewport(const int lineNumber) const
{
    const SMALL_RECT viewport = _getViewport();
    return _isLineInViewport(lineNumber, viewport);
}

// TODO MSFT 7960168 find a better way to do this
const bool UiaTextRange::_isLineInViewport(const int lineNumber, const SMALL_RECT viewport) const
{
    const int viewportHeight = viewport.Bottom - viewport.Top + 1;
    const COORD screenBufferCoords = _getScreenBufferCoords();

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

const int UiaTextRange::_lineNumberToViewport(const int lineNumber) const
{
    const SMALL_RECT viewport = _getViewport();
    const int viewportHeight = viewport.Bottom - viewport.Top + 1;
    const COORD screenBufferCoords = _getScreenBufferCoords();

    for (int i = 0; i < viewportHeight; ++i)
    {
        int currentLineNumber = (viewport.Top + i) % screenBufferCoords.Y;
        if (currentLineNumber == lineNumber)
        {
            return i;
        }
    }
    // TODO MSFT 7960168 better failure value
    return -1;

}

const int UiaTextRange::_endpointToRow(const int endpoint) const
{
    const COORD screenBufferCoords = _getScreenBufferCoords();
    return endpoint / screenBufferCoords.X;
}

const int UiaTextRange::_endpointToColumn(const int endpoint) const
{
    const COORD screenBufferCoords = _getScreenBufferCoords();
    return endpoint % screenBufferCoords.X;
}

const int UiaTextRange::_rowToEndpoint(const int row) const
{
    const COORD screenBufferCoords = _getScreenBufferCoords();
    return row * screenBufferCoords.X;
}


const int UiaTextRange::_getTotalRows() const
{
    const COORD screenBufferCoords = _getScreenBufferCoords();
    const int totalRows = screenBufferCoords.Y;
    return totalRows;
}

const int UiaTextRange::_getRowWidth() const
{
    const COORD screenBufferCoords = _getScreenBufferCoords();
    const int rowWidth = screenBufferCoords.X;
    return rowWidth;
}

const SMALL_RECT UiaTextRange::_getViewport() const
{
    return _pScreenInfo->GetBufferViewport();
}

IConsoleWindow* UiaTextRange::_getWindow()
{
    return ServiceLocator::LocateConsoleWindow();
}

HWND UiaTextRange::_getWindowHandle()
{
    return ServiceLocator::LocateConsoleWindow()->GetWindowHandle();
}

const COORD UiaTextRange::_getScreenBufferCoords() const
{
    return ServiceLocator::LocateGlobals()->getConsoleInformation()->GetScreenBufferSize();
}