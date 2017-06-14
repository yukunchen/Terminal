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

#include <sstream>
// This is a debugging function that prints out the current
// relationship between screen info rows, text buffer rows, and
// endpoints.
void UiaTextRange::_outputRowConversions()
{
    unsigned int totalRows = _getTotalRows();
    OutputDebugString(L"screenBuffer\ttextBuffer\tendpoint\n");
    for (unsigned int i = 0; i < totalRows; ++i)
    {
        std::wstringstream ss;
        ss << i << "\t" << _screenInfoRowToTextBufferRow (i) << "\t" << _screenInfoRowToEndpoint(i) << "\n";
        std::wstring str = ss.str();
        OutputDebugString(str.c_str());
    }
    OutputDebugString(L"\n");
}
#endif


// The msdn documentation (and hence this file) talks a bunch about a
// degenerate range. A range is degenerate if it contains
// no text (both the start and end endpoints are the same). Note that
// a degenerate range may have a position in the text. We indicate a
// degenerate range internally with a bool. If a range is degenerate
// then both endpoints will contain the same value. Passing an ending
// value that is less than the starting value for endpoints to a
// constructor will make the range degenerate.


// degenerate range constructor.
UiaTextRange::UiaTextRange(_In_ IRawElementProviderSimple* const pProvider,
                           _In_ const TEXT_BUFFER_INFO* const pOutputBuffer,
                           _In_ const SCREEN_INFORMATION* const pScreenInfo) :
    _cRefs{ 1 },
    _pProvider{ THROW_HR_IF_NULL(E_INVALIDARG, pProvider) },
    _pOutputBuffer{ THROW_HR_IF_NULL(E_INVALIDARG, pOutputBuffer) },
    _pScreenInfo{ THROW_HR_IF_NULL(E_INVALIDARG, pScreenInfo) },
    _start{ 0 },
    _end{ 0 },
    _degenerate{ true }
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
    _degenerate = end < start;
    _start = start;
    _end = _degenerate ? start : end;
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
    const Viewport viewport = _getViewport();
    ScreenInfoRow row;
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
    _start = _screenInfoRowToEndpoint(row);
    _end = _start;
    _degenerate = true;
}

UiaTextRange::UiaTextRange(_In_ const UiaTextRange& a) :
    _cRefs{ 1 },
    _pProvider{ a._pProvider },
    _pOutputBuffer{ a._pOutputBuffer },
    _pScreenInfo{ a._pScreenInfo },
    _start{ a._start },
    _end{ a._end },
    _degenerate{ a._degenerate }

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

const Endpoint UiaTextRange::GetStart() const
{
    return _start;
}

const Endpoint UiaTextRange::GetEnd() const
{
    return _end;
}

// Routine Description:
// - returns true if the range is currently degenerate (empty range).
// Arguments:
// - <none>
// Return Value:
// - true if range is degenerate, false otherwise.
const bool UiaTextRange::IsDegenerate() const
{
    return _degenerate;
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
    ServiceLocator::LocateGlobals()->getConsoleInformation()->LockConsole();
    auto Unlock = wil::ScopeExit([&]
    {
        ServiceLocator::LocateGlobals()->getConsoleInformation()->UnlockConsole();
    });

    *pRetVal = FALSE;
    UiaTextRange* other = static_cast<UiaTextRange*>(pRange);
    if (other)
    {
        *pRetVal = !!(_start == other->GetStart() &&
                      _end == other->GetEnd() &&
                      _degenerate == other->IsDegenerate());
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
        theirValue = range->GetStart();
    }
    else
    {
        theirValue = range->GetEnd();
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
    const ScreenInfoRow topRow = 0;
    const ScreenInfoRow bottomRow = _getTotalRows() - 1;
    const unsigned int rowWidth = _getRowWidth();

    if (unit <= TextUnit::TextUnit_Line)
    {
        // expand to line
        _start = _textBufferRowToEndpoint(_endpointToTextBufferRow(_start));
        // (rowWidth - 1) is the last column of the row
        _end = _start + rowWidth - 1;
    }
    else
    {
        // expand to document
        _start = _screenInfoRowToEndpoint(topRow);
        // (rowWidth - 1) is the last column of the row
        _end = _screenInfoRowToEndpoint(bottomRow) + rowWidth - 1;
    }

    assert(_start <= _end);
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
    ServiceLocator::LocateGlobals()->getConsoleInformation()->LockConsole();
    auto Unlock = wil::ScopeExit([&]
    {
        ServiceLocator::LocateGlobals()->getConsoleInformation()->UnlockConsole();
    });

    const unsigned int totalRowsInRange = _rowCountInRange();
    const COORD currentFontSize = _pScreenInfo->GetScreenFontSize();
    const TextBufferRow startRow = _endpointToTextBufferRow(_start);
    *ppRetVal = nullptr;

    // vector to put coords into. they go in as four doubles in the
    // order: left, top, width, height. each line will have its own
    // set of coords.
    std::vector<double> coords;

    if (_degenerate && _isScreenInfoRowInViewport(startRow))
    {
        POINT topLeft;
        POINT bottomRight;
        ScreenInfoRow screenInfoRow = _textBufferRowToScreenInfoRow(startRow);

        topLeft.x = 0;
        topLeft.y = _screenInfoRowToViewportRow(screenInfoRow) * currentFontSize.Y;

        bottomRight.x = topLeft.x + currentFontSize.X;
        // we add the font height only once here because we are adding each line individually
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

    for (unsigned int i = 0; i < totalRowsInRange; ++i)
    {
        ScreenInfoRow screenInfoRow = _textBufferRowToScreenInfoRow(startRow + i);
        if (!_isScreenInfoRowInViewport(screenInfoRow))
        {
            continue;
        }

        // measure boundaries, in pixels
        POINT topLeft;
        POINT bottomRight;

        topLeft.x = 0;
        topLeft.y = _screenInfoRowToViewportRow(screenInfoRow) * currentFontSize.Y;

        bottomRight.x = _getViewportWidth(_getViewport()) * currentFontSize.X;
        // we add the font height only once here because we are adding each line individually
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
    ServiceLocator::LocateGlobals()->getConsoleInformation()->LockConsole();
    auto Unlock = wil::ScopeExit([&]
    {
        ServiceLocator::LocateGlobals()->getConsoleInformation()->UnlockConsole();
    });

    assert(_start <= _end);

    std::wstring wstr = L"";
    const bool getPartialText = maxLength != -1;
    const TextBufferRow startTextBufferRow = _endpointToTextBufferRow(_start);
    const unsigned int totalRowsInRange = _rowCountInRange();

    for (unsigned int i = 0; i < totalRowsInRange; ++i)
    {
        try
        {
            const ROW row = _pOutputBuffer->Rows[_normalizeRow(startTextBufferRow + i)];
            if (row.CharRow.ContainsText())
            {
                std::wstring tempString = std::wstring(row.CharRow.Chars + row.CharRow.Left,
                                                       row.CharRow.Chars + row.CharRow.Right);
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

    assert(_start <= _end);

   *pRetVal = 0;
    if (count == 0)
    {
        return S_OK;
    }

    int incrementAmount;
    ScreenInfoRow limitingRow;
    if (count > 0)
    {
        // moving forward
        incrementAmount = 1;
        limitingRow = _getTotalRows() - 1;
    }
    else
    {
        // moving backward
        incrementAmount = -1;
        limitingRow = 0;
    }

    ScreenInfoRow screenInfoRow = _endpointToScreenInfoRow(_start);
    for (int i = 0; i < abs(count); ++i)
    {
        if (screenInfoRow == limitingRow)
        {
            break;
        }
        screenInfoRow += incrementAmount;
        *pRetVal += incrementAmount;
    }

    // update endpoints
    _start = _screenInfoRowToEndpoint(screenInfoRow);
    // (_getRowWidth() - 1) is the last column in the row
    _end = _start + _getRowWidth() - 1;

    // a range can't be degenerate after both endpoints have been
    // moved.
    _degenerate = false;

    assert(_start <= _end);
    return S_OK;
}

IFACEMETHODIMP UiaTextRange::MoveEndpointByUnit(_In_ TextPatternRangeEndpoint endpoint,
                                                _In_ TextUnit unit,
                                                _In_ int count,
                                                _Out_ int* pRetVal)
{
    // for now, we only support line movement
    UNREFERENCED_PARAMETER(unit);

    assert(_start <= _end);

    *pRetVal = 0;
    if (count == 0)
    {
        return S_OK;
    }

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

    ScreenInfoRow screenInfoRow = _endpointToScreenInfoRow(*pInternalEndpoint);

    // set values depending on move direction
    int incrementAmount;
    ScreenInfoRow limitingRow;
    if (count > 0)
    {
        // moving forward
        incrementAmount = 1;
        limitingRow = _getTotalRows() - 1;
    }
    else
    {
        // moving backward
        incrementAmount = -1;
        limitingRow = 0;
    }

    // move the endpoint
    for (int i = 0; i < abs(count); ++i)
    {
        if (screenInfoRow == limitingRow)
        {
            break;
        }
        screenInfoRow += incrementAmount;
        *pRetVal += incrementAmount;
    }
    *pInternalEndpoint = _screenInfoRowToEndpoint(screenInfoRow);

    // fix out of order endpoints. If they crossed then the it is
    // turned into a degenerate range at the point where the endpoint
    // we moved stops at.
    if (_start > _end || (_degenerate && shrinkingRange))
    {
        if (endpoint == TextPatternRangeEndpoint::TextPatternRangeEndpoint_Start)
        {
            _end = _start;
        }
        else
        {
            _start = _end;
        }
        _degenerate = true;
    }
    else
    {
        _degenerate = false;
    }

    assert(_start <= _end);
    return S_OK;
}

IFACEMETHODIMP UiaTextRange::MoveEndpointByRange(_In_ TextPatternRangeEndpoint endpoint,
                                                 _In_ ITextRangeProvider* pTargetRange,
                                                 _In_ TextPatternRangeEndpoint targetEndpoint)
{
    assert(_start <= _end);

    UiaTextRange* range = static_cast<UiaTextRange*>(pTargetRange);
    if (range == nullptr)
    {
        return E_INVALIDARG;
    }

    // get the value that we're updating to
    Endpoint newValue;
    if (targetEndpoint == TextPatternRangeEndpoint::TextPatternRangeEndpoint_Start)
    {
        newValue = range->GetStart();
    }
    else
    {
        newValue = range->GetEnd();
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
    assert(_start <= _end);
    return S_OK;
}

IFACEMETHODIMP UiaTextRange::Select()
{
    ServiceLocator::LocateGlobals()->getConsoleInformation()->LockConsole();
    auto Unlock = wil::ScopeExit([&]
    {
        ServiceLocator::LocateGlobals()->getConsoleInformation()->UnlockConsole();
    });

    assert(_start <= _end);

    COORD coordStart;
    COORD coordEnd;

    coordStart.X = static_cast<SHORT>(_endpointToColumn(_start));
    coordStart.Y = static_cast<SHORT>(_endpointToScreenInfoRow(_start));

    coordEnd.X = static_cast<SHORT>(_endpointToColumn(_end));
    coordEnd.Y = static_cast<SHORT>(_endpointToScreenInfoRow(_end));

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
    ServiceLocator::LocateGlobals()->getConsoleInformation()->LockConsole();
    auto Unlock = wil::ScopeExit([&]
    {
        ServiceLocator::LocateGlobals()->getConsoleInformation()->UnlockConsole();
    });

    assert(_start <= _end);

    const Viewport oldViewport = _getViewport();
    const unsigned int viewportHeight = _getViewportHeight(oldViewport);
    const unsigned int totalRows = _getTotalRows();

    // range rows
    const ScreenInfoRow startScreenInfoRow = _endpointToScreenInfoRow(_start);
    const ScreenInfoRow endScreenInfoRow = _endpointToScreenInfoRow(_end);

    // screen buffer rows
    const ScreenInfoRow topRow = 0;
    const ScreenInfoRow bottomRow = totalRows - 1;

    Viewport newViewport = oldViewport;

    // there's a bunch of +1/-1s here for setting the viewport. These
    // are to account for the inclusivity of the viewport boundaries.
    if (alignToTop)
    {
        // determine if we can align the start row to the top
        if (startScreenInfoRow + viewportHeight <= bottomRow)
        {
            // we can align to the top
            newViewport.Top = static_cast<SHORT>(startScreenInfoRow);
            newViewport.Bottom = static_cast<SHORT>(startScreenInfoRow + viewportHeight - 1);
        }
        else
        {
            // we can align to the top so we'll just move the viewport
            // to the bottom of the screen buffer
            newViewport.Bottom = static_cast<SHORT>(bottomRow);
            newViewport.Top = static_cast<SHORT>(bottomRow - viewportHeight + 1);
        }
    }
    else
    {
        // we need to align to the bottom
        // check if we can align to the bottom
        if (endScreenInfoRow - viewportHeight >= topRow)
        {
            // we can align to bottom
            newViewport.Bottom = static_cast<SHORT>(endScreenInfoRow);
            newViewport.Top = static_cast<SHORT>(endScreenInfoRow - viewportHeight + 1);
        }
        else
        {
            // we can't align to bottom so we'll move the viewport to
            // the top of the screen buffer
            newViewport.Top = static_cast<SHORT>(topRow);
            newViewport.Bottom = static_cast<SHORT>(topRow + viewportHeight - 1);
        }

    }

    assert(newViewport.Top >= topRow);
    assert(newViewport.Bottom <= static_cast<SHORT>(bottomRow));
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
// - Gets the current viewport
// Arguments:
// - <none>
// Return Value:
// - The screen info's current viewport
const Viewport UiaTextRange::_getViewport() const
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
// - Gets the width of the screen buffer rows
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
// - converts an Endpoint into its equivalent text buffer row.
// Arguments:
// - endpoint - the endpoint to convert
// Return Value:
// - the text buffer row value
const TextBufferRow UiaTextRange::_endpointToTextBufferRow(_In_ const Endpoint endpoint) const
{
    return endpoint / _getRowWidth();
}

// Routine Description:
// - counts the number of rows that are fully or partially part of the
// range.
// Arguments:
// - <none>
// Return Value:
// - The number of rows in the range.
const unsigned int UiaTextRange::_rowCountInRange() const
{
    assert(_start <= _end);

    if (_degenerate)
    {
        return 0;
    }
    const TextBufferRow textBufferStartRow = _endpointToTextBufferRow(_start);
    const TextBufferRow textBufferEndRow = _endpointToTextBufferRow(_end);
    // + 1 to balance subtracting TextBufferRows from each other
    return textBufferEndRow - textBufferStartRow + 1;
}

// Routine Description:
// - Converts a TextBufferRow to a ScreenInfoRow.
// Arguments:
// - row - the TextBufferRow to convert
// Return Value:
// - the equivalent ScreenInfoRow.
const ScreenInfoRow UiaTextRange::_textBufferRowToScreenInfoRow(_In_ const TextBufferRow row) const
{
    const int firstRowIndex = _pOutputBuffer->GetFirstRowIndex();
    return _normalizeRow(row - firstRowIndex);
}

// Routine Description:
// - Converts a ScreenInfoRow to a ViewportRow. Uses the default
// viewport for the conversion.
// Arguments:
// - row - the ScreenInfoRow to convert
// Return Value:
// - the equivalent ViewportRow.
const ViewportRow UiaTextRange::_screenInfoRowToViewportRow(_In_ const ScreenInfoRow row) const
{
    const Viewport viewport = _getViewport();
    return _screenInfoRowToViewportRow(row, viewport);
}

// Routine Description:
// - Converts a ScreenInfoRow to a ViewportRow.
// Arguments:
// - row - the ScreenInfoRow to convert
// - viewport - the viewport to use for the conversion
// Return Value:
// - the equivalent ViewportRow.
const ViewportRow UiaTextRange::_screenInfoRowToViewportRow(_In_ const ScreenInfoRow row,
                                                            _In_ const Viewport viewport) const
{
    return row - viewport.Top;
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
// - Gets the viewport height, measured in char rows.
// Arguments:
// - viewport - The viewport to measure
// Return Value:
// - The viewport height
const unsigned int UiaTextRange::_getViewportHeight(_In_ const Viewport viewport)
{
    assert(viewport.Bottom >= viewport.Top);
    // + 1 because COORD is inclusive on both sides so subtracting top
    // and bottom gets rid of 1 more then it should.
    return viewport.Bottom - viewport.Top + 1;
}

// Routine Description:
// - Gets the viewport width, measured in char columns.
// Arguments:
// - viewport - The viewport to measure
// Return Value:
// - The viewport width
const unsigned int UiaTextRange::_getViewportWidth(_In_ const Viewport viewport)
{
    assert(viewport.Right >= viewport.Left);

    // + 1 because COORD is inclusive on both sides so subtracting left
    // and right gets rid of 1 more then it should.
    return (viewport.Right - viewport.Left + 1);
}

// Routine Description:
// - checks if the row is currently visible in the viewport. Uses the
// default viewport.
// Arguments:
// - row - the screen info row to check
// Return Value:
// - true if the row is within the bounds of the viewport
const bool UiaTextRange::_isScreenInfoRowInViewport(_In_ const ScreenInfoRow row) const
{
    return _isScreenInfoRowInViewport(row, _getViewport());
}

// Routine Description:
// - checks if the row is currently visible in the viewport
// Arguments:
// - row - the row to check
// - viewport - the viewport to use for the bounds
// Return Value:
// - true if the row is within the bounds of the viewport
const bool UiaTextRange::_isScreenInfoRowInViewport(_In_ const ScreenInfoRow row,
                                                    _In_ const Viewport viewport) const
{
    ViewportRow viewportRow = _screenInfoRowToViewportRow(row, viewport);
    return viewportRow >= 0 && viewportRow < static_cast<ViewportRow>(_getViewportHeight(viewport));
}

// Routine Description:
// - Converts a ScreenInfoRow to a TextBufferRow.
// Arguments:
// - row - the ScreenInfoRow to convert
// Return Value:
// - the equivalent TextBufferRow.
const TextBufferRow UiaTextRange::_screenInfoRowToTextBufferRow(_In_ const ScreenInfoRow row) const
{
    const TextBufferRow firstRowIndex = _pOutputBuffer->GetFirstRowIndex();
    return _normalizeRow(row + firstRowIndex);
}

// Routine Description:
// - Converts a TextBufferRow to an Endpoint.
// Arguments:
// - row - the TextBufferRow to convert
// Return Value:
// - the equivalent Endpoint, starting at the beginning of the TextBufferRow.
const Endpoint UiaTextRange::_textBufferRowToEndpoint(_In_ const TextBufferRow row) const
{
    return _getRowWidth() * row;
}

// Routine Description:
// - Converts a ScreenInfoRow to an Endpoint.
// Arguments:
// - row - the ScreenInfoRow to convert
// Return Value:
// - the equivalent Endpoint.
const Endpoint UiaTextRange::_screenInfoRowToEndpoint(_In_ const ScreenInfoRow row) const
{
    return _textBufferRowToEndpoint(_screenInfoRowToTextBufferRow(row));
}

// Routine Description:
// - Converts an Endpoint to an ScreenInfoRow.
// Arguments:
// - endpoint - the endpoint to convert
// Return Value:
// - the equivalent ScreenInfoRow.
const ScreenInfoRow UiaTextRange::_endpointToScreenInfoRow(_In_ const Endpoint endpoint) const
{
    return _textBufferRowToScreenInfoRow(_endpointToTextBufferRow(endpoint));
}