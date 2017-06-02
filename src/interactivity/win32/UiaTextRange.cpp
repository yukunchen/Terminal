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


// The msdn documentation (and hence this file) talks a bunch about a
// degenerate range. A range is degenerate if it contains
// no text (both the start and end endpoints are the same). Note that
// a degenerate range may have a position in the text.


// degenerate range constructor.
UiaTextRange::UiaTextRange(_In_ IRawElementProviderSimple* const pProvider,
                           _In_ const TEXT_BUFFER_INFO* const pOutputBuffer,
                           _In_ const SCREEN_INFORMATION* const pScreenInfo) :
    _cRefs{ 1 },
    _pProvider{ THROW_HR_IF_NULL(E_INVALIDARG, pProvider) },
    _pOutputBuffer{ THROW_HR_IF_NULL(E_INVALIDARG, pOutputBuffer) },
    _pScreenInfo{ THROW_HR_IF_NULL(E_INVALIDARG, pScreenInfo) },
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
                           _In_ const Endpoint start,
                           _In_ const Endpoint end) :
    UiaTextRange(pProvider, pOutputBuffer, pScreenInfo)
{
    _start = start;
    _end = end;
}

// returns a degenerate text range of the start of the row closest to the y value of point
UiaTextRange::UiaTextRange(_In_ IRawElementProviderSimple* const pProvider,
                           _In_ const TEXT_BUFFER_INFO* const pOutputBuffer,
                           _In_ const SCREEN_INFORMATION* const pScreenInfo,
                           _In_ const UiaPoint point) :
    UiaTextRange(pProvider, pOutputBuffer, pScreenInfo)
{
    POINT clientPoint;
    clientPoint.x = static_cast<LONG>(point.x);
    clientPoint.y = static_cast<LONG>(point.y);
    // get row that point resides in
    const RECT windowRect = _getWindow()->GetWindowRect();
    const SMALL_RECT viewport = _getViewport();
    Row row;
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
        const COORD currentFontSize = _pScreenInfo->GetScreenFontSize();
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

const Endpoint UiaTextRange::getStart() const
{
    return _start;
}

const Endpoint UiaTextRange::getEnd() const
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
    Endpoint theirValue;
    if (targetEndpoint == TextPatternRangeEndpoint::TextPatternRangeEndpoint_Start)
    {
        theirValue = range->getStart();
    }
    else
    {
        theirValue = range->getEnd();
    }

    // get the values of our endpoint
    Endpoint ourValue;
    if (endpoint == TextPatternRangeEndpoint::TextPatternRangeEndpoint_Start)
    {
        ourValue = _start;
    }
    else
    {
        ourValue = _end;
    }

    // compare them
    *pRetVal = clamp(static_cast<int>(ourValue) - static_cast<int>(theirValue), -1, 1);

    return S_OK;
}

IFACEMETHODIMP UiaTextRange::ExpandToEnclosingUnit(_In_ TextUnit unit)
{
    const Row topRow = _getScreenBufferTopRow();
    const Row bottomRow = _getScreenBufferBottomRow();
    const unsigned int rowWidth = _getRowWidth();

    if (unit <= TextUnit::TextUnit_Line)
    {
        // expand to line
        _start = _rowToEndpoint(_endpointToRow(_start));
        _end = _start + rowWidth;
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
    const unsigned int totalRows = _getTotalRows();
    const Row startRow = _endpointToRow(_start);
    const Row endRow = _endpointToRow(_end);
    const unsigned int totalRowsInRange = _rowCountInRange();
    // width of viewport (measured in chars)
    const SMALL_RECT viewport = _getViewport();
    const unsigned int viewportWidth = _getViewportWidth(viewport);
    const COORD currentFontSize = _pScreenInfo->GetScreenFontSize();
    *ppRetVal = nullptr;

    // vector to put coords into. they go in as four doubles in the
    // order: left, top, width, height. each line will have its own
    // set of coords.
    std::vector<double> coords;

    if (_isDegenerate() && _isRowInViewport(startRow))
    {
        POINT topLeft;
        POINT bottomRight;
        topLeft.x = _endpointToColumn(_start) * currentFontSize.X;
        try
        {
            topLeft.y = _rowToViewport(startRow) * currentFontSize.Y;
        }
        CATCH_RETURN();

        bottomRight.x = topLeft.x;
        bottomRight.y = topLeft.y + currentFontSize.Y;

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

    for (unsigned int i = 0; i < totalRowsInRange; ++i)
    {
        const int currentRowNumber = _normalizeRow(startRow + i);
        if (!_isRowInViewport(currentRowNumber))
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
            topLeft.x = _endpointToColumn(_start) * currentFontSize.X;
        }
        else
        {
            topLeft.x = 0;
        }

        try
        {
            topLeft.y = _rowToViewport(currentRowNumber) * currentFontSize.Y;
        }
        CATCH_RETURN();

        if (i + 1 == totalRowsInRange)
        {
            // special logic for last line since we might not end at
            // the right edge of the viewport. We need to subtract 1
            // from end because it is not an inclusive range and this
            // might affect what line _endpointToColumn thinks the
            // endpoint lays on but we need to add 1 back to the to
            // total because we want the bounding rectangle to
            // encompass that column.
            bottomRight.x = (_endpointToColumn(_end - 1) + 1) * currentFontSize.X;
        }
        else
        {
            // we're a full line, so we go the full width of the
            // viewport
            bottomRight.x = viewportWidth * currentFontSize.X;
        }

        // + 1 of the font height because we are adding each line individually
        bottomRight.y = topLeft.y + currentFontSize.Y;

        // convert the coords to be relative to the screen instead of
        // the client window
        HWND hwnd = _getWindowHandle();
        ClientToScreen(hwnd, &topLeft);
        ClientToScreen(hwnd, &bottomRight);

        const LONG width = bottomRight.x - topLeft.x;
        const LONG height = bottomRight.y - topLeft.y;

        // insert the coords
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
    const unsigned int totalRows = _getTotalRows();
    const Row startRow = _endpointToRow(_start);
    const Row endRow = _endpointToRow(_end);
    const unsigned int totalRowsInRange = _rowCountInRange();

    for (unsigned int i = 0; i < totalRowsInRange; ++i)
    {
        try
        {
            const ROW* const row = _pOutputBuffer->GetRowByOffset(startRow + i);
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

    int incrementAmount;
    Row limitingRow;
    if (count > 0)
    {
        // moving forward
        incrementAmount = 1;
        limitingRow = _getScreenBufferBottomRow();
    }
    else
    {
        // moving backward
        incrementAmount = -1;
        limitingRow = _getScreenBufferTopRow();
    }

    Row currentRow = _endpointToRow(_start);
    for (int i = 0; i < abs(count); ++i)
    {
        if (currentRow == limitingRow)
        {
            break;
        }
        currentRow = _normalizeRow(currentRow + incrementAmount);
        *pRetVal += incrementAmount;
    }

    // update endpoints
   _start = _rowToEndpoint(currentRow);
   // we don't need to normalize because the ending endpoint is
   // an exclusive range so normalizing would break it if we're at the
   // bottom edge the output buffer.
   _end = _rowToEndpoint(currentRow + 1);

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

    const bool isDegenerate = _isDegenerate();
    const bool shrinkingRange = (count < 0 &&
                                 endpoint == TextPatternRangeEndpoint::TextPatternRangeEndpoint_End) ||
                                (count > 0 &&
                                 endpoint == TextPatternRangeEndpoint::TextPatternRangeEndpoint_Start);


    // determine which endpoint we're moving
    Endpoint* pInternalEndpoint;
    Endpoint otherEndpoint;
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
    Row currentRow = _endpointToRow(*pInternalEndpoint);

    // set values depending on move direction
    int incrementAmount;
    Row limitingRow;
    if (count > 0)
    {
        // moving forward
        incrementAmount = 1;
        limitingRow = _getScreenBufferBottomRow();
    }
    else
    {
        // moving backward
        incrementAmount = -1;
        limitingRow = _getScreenBufferTopRow();
    }

    // move the endpoint
    bool crossedEndpoints = false;
    for (int i = 0; i < abs(count); ++i)
    {
        if (currentRow == limitingRow)
        {
            break;
        }
        currentRow = _normalizeRow(currentRow + incrementAmount);
        *pRetVal += incrementAmount;
        if (currentRow == otherEndpoint)
        {
            crossedEndpoints = true;
        }
    }
    *pInternalEndpoint = _rowToEndpoint(currentRow);

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
    Endpoint newValue;
    if (targetEndpoint == TextPatternRangeEndpoint::TextPatternRangeEndpoint_Start)
    {
        newValue = range->getStart();
    }
    else
    {
        newValue = range->getEnd();
    }

    // get the endpoint that we're changing
    Endpoint* pInternalEndpoint;
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
    const SMALL_RECT oldViewport = _getViewport();
    const unsigned int viewportHeight = _getViewportHeight(oldViewport);
    const unsigned int totalRows = _getTotalRows();
    // range rows
    const Row startRow = _endpointToRow(_start);
    // - 1 to account for exclusivity
    const Row endRow = _endpointToRow(_end - 1);
    // screen buffer rows
    const Row topRow = _getScreenBufferTopRow();
    const Row bottomRow = _getScreenBufferBottomRow();

    SMALL_RECT newViewport = oldViewport;
    newViewport.Top = static_cast<SHORT>(topRow);
    // - 1 because SMALL_RECTs are inclusive on both sides
    newViewport.Bottom = static_cast<SHORT>(_normalizeRow(newViewport.Top + viewportHeight - 1));

    // shift the viewport down one line at a time until we have
    // the range at the correct edge of the viewport or we hit
    // the bottom edge of the screen buffer
    if (alignToTop)
    {
        while (static_cast<Row>(newViewport.Top) != startRow &&
               static_cast<Row>(newViewport.Bottom) != bottomRow)
        {
            newViewport.Top = static_cast<SHORT>(_normalizeRow(newViewport.Top + 1));
            newViewport.Bottom = static_cast<SHORT>(_normalizeRow(newViewport.Bottom + 1));
        }
    }
    else if (!_isRowInViewport(endRow, newViewport))
    {

        while (static_cast<Row>(newViewport.Bottom) != endRow &&
               static_cast<Row>(newViewport.Bottom) != bottomRow)
        {
            newViewport.Top = static_cast<SHORT>(_normalizeRow(newViewport.Top + 1));
            newViewport.Bottom = static_cast<SHORT>(_normalizeRow(newViewport.Bottom + 1));
        }
    }

    assert(_getViewportHeight(oldViewport) == _getViewportHeight(newViewport));

    _getWindow()->SetViewportOrigin(newViewport);

    return S_OK;
}

IFACEMETHODIMP UiaTextRange::GetChildren(_Outptr_result_maybenull_ SAFEARRAY** ppRetVal)
{
    // we don't have any children
    *ppRetVal = SafeArrayCreateVector(VT_UNKNOWN, 0, 0);
    return S_OK;
}

#pragma endregion

// Routine Description:
// - returns true if the range is currently degenerate (empty range).
// Arguments:
// - <none>
// Return Value:
// - true if range is degenerate, false otherwise.
const bool UiaTextRange::_isDegenerate() const
{
    return (_start == _end);
}

// Routine Description:
// - checks if the row is currently visible in the viewport. Uses the
// default viewport.
// Arguments:
// - row - the row to check
// Return Value:
// - true if the row is within the bounds of the viewport
const bool UiaTextRange::_isRowInViewport(_In_ const Row row) const
{
    const SMALL_RECT viewport = _getViewport();
    return _isRowInViewport(row, viewport);
}

// Routine Description:
// - checks if the row is currently visible in the viewport
// Arguments:
// - row - the row to check
// - viewport - the viewport to use for the bounds
// Return Value:
// - true if the row is within the bounds of the viewport
const bool UiaTextRange::_isRowInViewport(_In_ const Row row,
                                          _In_ const SMALL_RECT viewport) const
{
    if (viewport.Top < viewport.Bottom)
    {
        return (row >= static_cast<Row>(viewport.Top) &&
                row <= static_cast<Row>(viewport.Bottom));
    }
    else if (viewport.Top == viewport.Bottom)
    {
        return row == static_cast<Row>(viewport.Top);
    }
    else
    {
        return (row >= static_cast<Row>(viewport.Top) ||
                row <= static_cast<Row>(viewport.Bottom));
    }
}

// Routine Description:
// - converts a row index to an index of the current viewport. Is
// 0-indexed.
// Arguments:
// - row - the row to convert
// Return Value:
// - The index of the row in the viewport.
// Note:
// throws out_of_range exception if row is not in viewport
const ViewportRow UiaTextRange::_rowToViewport(_In_ const Row row) const
{
    const SMALL_RECT viewport = _getViewport();
    return _rowToViewport(row, viewport);
}

const ViewportRow UiaTextRange::_rowToViewport(_In_ const Row row,
                                               _In_ const SMALL_RECT viewport) const
{
    // we return a failure value if the line is not currently visible
    if (!_isRowInViewport(row, viewport))
    {
        throw std::out_of_range(row + " is not in viewport");
    }

    if (row >= static_cast<Row>(viewport.Top))
    {
        return row - viewport.Top;
    }
    else
    {
        return _getViewportHeight(viewport) - viewport.Bottom + row;
    }
}

// Routine Description:
// - calculates the row refered to by the endpoint.
// Arguments:
// - endpoint - the endpoint to translate
// Return Value:
// - the row value
const Row UiaTextRange::_endpointToRow(_In_ const Endpoint endpoint)
{
    return endpoint / _getRowWidth();
}

// Routine Description:
// - calculates the column refered to by the endpoint.
// Arguments:
// - endpoint - the endpoint to translate
// Return Value:
// - the column value
const Column UiaTextRange::_endpointToColumn(_In_ const Endpoint endpoint)
{
    return endpoint % _getRowWidth();
}

// Routine Description:
// - Converts a row to an endpoint value for the beginning of the row.
// Arguments:
// - row - the row to convert.
// Return Value:
// - the converted endpoint value.
const Endpoint UiaTextRange::_rowToEndpoint(_In_ const Row row) const
{
    return row * _getRowWidth();
}

// Routine Description:
// - Gets the number of rows in the output text buffer.
// Arguments:
// - <none>
// Return Value:
// - The number of rows
const unsigned int UiaTextRange::_getTotalRows() const
{
    return _pOutputBuffer->TotalRowCount();
}

// Routine Description:
// - Gets the width of the text buffer rows
// Arguments:
// - <none>
// Return Value:
// - The row width
const unsigned int UiaTextRange::_getRowWidth()
{
    // make sure that we can't leak a 0
    return max(_getScreenBufferCoords().X, 1);
}

// Routine Description:
// - Gets the viewport height, measured in char rows.
// Arguments:
// - viewport - The viewport to measure
// Return Value:
// - The viewport height
const unsigned int UiaTextRange::_getViewportHeight(_In_ const SMALL_RECT viewport) const
{
    if (viewport.Top <= viewport.Bottom)
    {
        // + 1 because COORD is inclusive on both sides so subtracting top
        // and bottom gets rid of 1 more then it should.
        return _normalizeRow(viewport.Bottom - viewport.Top + 1);
    }
    else
    {
        // the text buffer has cycled through its buffer enough that
        // the 0th row is somewhere in the viewport
        const int totalRows = _getTotalRows();
        return totalRows - viewport.Top + viewport.Bottom + 1;
    }
}

// Routine Description:
// - Gets the viewport width, measured in char columns.
// Arguments:
// - viewport - The viewport to measure
// Return Value:
// - The viewport width
const unsigned int UiaTextRange::_getViewportWidth(_In_ const SMALL_RECT viewport)
{
    assert(viewport.Right >= viewport.Left);

    // + 1 because COORD is inclusive on both sides so subtracting left
    // and right gets rid of 1 more then it should.
    return (viewport.Right - viewport.Left + 1);
}

// Routine Description:
// - Gets the current viewport
// Arguments:
// - <none>
// Return Value:
// - The screen info's current viewport
const SMALL_RECT UiaTextRange::_getViewport() const
{
    return _pScreenInfo->GetBufferViewport();
}

// Routine Description:
// - Gets the current window
// Arguments:
// - <none>
// Return Value:
// - The current window. May return nullptr if there is no current
// window.
IConsoleWindow* UiaTextRange::_getWindow()
{
    return ServiceLocator::LocateConsoleWindow();
}

// Routine Description:
// - gets the current window handle
// Arguments:
// - <none>
// Return Value
// - the current window handle
HWND UiaTextRange::_getWindowHandle()
{
    return ServiceLocator::LocateConsoleWindow()->GetWindowHandle();
}

// Routine Description:
// - gets the current screen buffer size.
// Arguments:
// - <none>
// Return Value:
// - The screen buffer size
const COORD UiaTextRange::_getScreenBufferCoords()
{
    return ServiceLocator::LocateGlobals()->getConsoleInformation()->GetScreenBufferSize();
}

// Routine Description:
// - normalizes the row index to within the bounds of the output
// buffer. The output buffer stores the text in a circular buffer so
// this method makes sure that we circle around gracefully.
// Arguments:
// - the non-normalized row index
// Return Value:
// - the normalized row index
const Row UiaTextRange::_normalizeRow(_In_ const Row row) const
{
    const unsigned int totalRows = _getTotalRows();
    return ((row + totalRows) % totalRows);
}

// Routine Description:
// - gets the 0-indexed row number that is currently the first row of
// the screen buffer.
// Arguments:
// - <none>
// Return Value:
// - the top row index of the screen buffer.
const Row UiaTextRange::_getScreenBufferTopRow() const
{
    return _pOutputBuffer->GetFirstRowIndex();
}

// Routine Description:
// - gets the 0-indexed row number that is currently the last row of
// the screen buffer.
// Arguments:
// - <none>
// Return Value:
// - the bottom row index of the screen buffer.
const Row UiaTextRange::_getScreenBufferBottomRow() const
{
    return _normalizeRow(_getScreenBufferTopRow() - 1);
}

// Routine Description:
// - counts the number of rows that are fully or partially part of the
// range.
// the screen buffer.
// Arguments:
// - <none>
// Return Value:
// - The numbe rof rows in the range.
const unsigned int UiaTextRange::_rowCountInRange() const
{
    const unsigned int totalRows = _getTotalRows();
    const Row startRow = _endpointToRow(_start);
    const Row endRow = _endpointToRow(_end);

    if (endRow >= startRow)
    {
        return endRow - startRow;
    }
    else
    {
        return totalRows + endRow - startRow;
    }
}
