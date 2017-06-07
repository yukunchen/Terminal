/*++
Copyright (c) Microsoft Corporation

Module Name:
- UiaTextRange.hpp

Abstract:
- This module provides UI Automation access to the text of the console
  window to support both automation tests and accessibility (screen
  reading) applications.

Author(s):
- Austin Diviness (AustDi)     2017
--*/

#pragma once

#include "../inc/IConsoleWindow.hpp"

#ifdef UNIT_TESTING
class UiaTextRangeTests;
#endif


// The UiaTextRange deals with several data structures that have
// similar semantics. In order to keep the information from these data
// structures separated, each structure has its own naming for a
// row.
//
// There is the generic Row, which does not know which data structure
// the row came from.
//
// There is the ViewportRow, which is a 0-indexed row value from the
// viewport. The top row of the viewport is at 0, rows below the top
// row increase in value and rows above the top row get increasingly
// negative.
//
// ScreenInfoRow is a row from the screen info data structure. They
// start at 0 at the top of screen info buffer. Their positions do not
// change but their associated row in the text buffer does change each
// time a new line is written.
//
// TextBufferRow is a row from the text buffer. It is not a ROW
// struct, but rather the index of a row. This is also 0-indexed. A
// TextBufferRow with a value of 0 does not necessarilly refer to the
// top row of the console.

typedef unsigned int Row;
typedef int ViewportRow;
typedef unsigned int ScreenInfoRow;
typedef unsigned int TextBufferRow;

typedef SMALL_RECT Viewport;

// A Column is a row agnostic value that refers to the column an
// endpoint is equivalent to. It is 0-indexed.
typedef unsigned int Column;

// an endpoint is a char location in the text buffer. endpoint 0 is
// the first char of the 0th row in the text buffer row array.
typedef unsigned int Endpoint;

namespace Microsoft
{
    namespace Console
    {
        namespace Interactivity
        {
            namespace Win32
            {
                class UiaTextRange final : public ITextRangeProvider
                {
                public:

                    // degenerate range
                    UiaTextRange(_In_ IRawElementProviderSimple* const pProvider,
                                 _In_ const TEXT_BUFFER_INFO* const pOutputBuffer,
                                 _In_ const SCREEN_INFORMATION* const pScreenInfo);

                    // specific range
                    UiaTextRange(_In_ IRawElementProviderSimple* const pProvider,
                                 _In_ const TEXT_BUFFER_INFO* const pOutputBuffer,
                                 _In_ const SCREEN_INFORMATION* const pScreenInfo,
                                 _In_ const Endpoint start,
                                 _In_ const Endpoint end);

                    // range from a UiaPoint
                    UiaTextRange(_In_ IRawElementProviderSimple* const pProvider,
                                 _In_ const TEXT_BUFFER_INFO* const pOutputBuffer,
                                 _In_ const SCREEN_INFORMATION* const pScreenInfo,
                                 _In_ const UiaPoint point);

                    UiaTextRange(_In_ const UiaTextRange& a);

                    ~UiaTextRange();


                    const Endpoint getStart() const;
                    const Endpoint getEnd() const;

                    // IUnknown methods
                    IFACEMETHODIMP_(ULONG) AddRef();
                    IFACEMETHODIMP_(ULONG) Release();
                    IFACEMETHODIMP QueryInterface(_In_ REFIID riid,
                                                  _COM_Outptr_result_maybenull_ void** ppInterface);

                    // ITextRangeProvider methods
                    IFACEMETHODIMP Clone(_Outptr_result_maybenull_ ITextRangeProvider** ppRetVal);
                    IFACEMETHODIMP Compare(_In_opt_ ITextRangeProvider* pRange, _Out_ BOOL* pRetVal);
                    IFACEMETHODIMP CompareEndpoints(_In_ TextPatternRangeEndpoint endpoint,
                                                    _In_ ITextRangeProvider* pTargetRange,
                                                    _In_ TextPatternRangeEndpoint targetEndpoint,
                                                    _Out_ int* pRetVal);
                    IFACEMETHODIMP ExpandToEnclosingUnit(_In_ TextUnit unit);
                    IFACEMETHODIMP FindAttribute(_In_ TEXTATTRIBUTEID textAttributeId,
                                                 _In_ VARIANT val,
                                                 _In_ BOOL searchBackward,
                                                 _Outptr_result_maybenull_ ITextRangeProvider** ppRetVal);
                    IFACEMETHODIMP FindText(_In_ BSTR text,
                                            _In_ BOOL searchBackward,
                                            _In_ BOOL ignoreCase,
                                            _Outptr_result_maybenull_ ITextRangeProvider** ppRetVal);
                    IFACEMETHODIMP GetAttributeValue(_In_ TEXTATTRIBUTEID textAttributeId,
                                                     _Out_ VARIANT* pRetVal);
                    IFACEMETHODIMP GetBoundingRectangles(_Outptr_result_maybenull_ SAFEARRAY** ppRetVal);
                    IFACEMETHODIMP GetEnclosingElement(_Outptr_result_maybenull_ IRawElementProviderSimple** ppRetVal);
                    IFACEMETHODIMP GetText(_In_ int maxLength,
                                           _Out_ BSTR* pRetVal);
                    IFACEMETHODIMP Move(_In_ TextUnit unit,
                                        _In_ int count,
                                        _Out_ int* pRetVal);
                    IFACEMETHODIMP MoveEndpointByUnit(_In_ TextPatternRangeEndpoint endpoint,
                                                      _In_ TextUnit unit,
                                                      _In_ int count,
                                                      _Out_ int* pRetVal);
                    IFACEMETHODIMP MoveEndpointByRange(_In_ TextPatternRangeEndpoint endpoint,
                                                       _In_ ITextRangeProvider* pTargetRange,
                                                       _In_ TextPatternRangeEndpoint targetEndpoint);
                    IFACEMETHODIMP Select();
                    IFACEMETHODIMP AddToSelection();
                    IFACEMETHODIMP RemoveFromSelection();
                    IFACEMETHODIMP ScrollIntoView(_In_ BOOL alignToTop);
                    IFACEMETHODIMP GetChildren(_Outptr_result_maybenull_ SAFEARRAY** ppRetVal);

                protected:
                    IRawElementProviderSimple* const _pProvider;
                    const TEXT_BUFFER_INFO* const _pOutputBuffer;
                    const SCREEN_INFORMATION* const _pScreenInfo;

                    #if _DEBUG
                    // used to debug objects passed back and forth
                    // between the provider and the client
                    static unsigned long long id;
                    unsigned long long _id;
                    #endif

                private:
                    // Ref counter for COM object
                    ULONG _cRefs;

                    // measure units in the form [start, end)
                    Endpoint _start;
                    Endpoint _end;

                    const bool _isDegenerate() const;

                    const Viewport _getViewport() const;
                    static HWND _getWindowHandle();
                    static IConsoleWindow* _getWindow();
                    const unsigned int _getTotalRows() const;
                    static const unsigned int _getRowWidth();
                    static const COORD _getScreenBufferCoords();

                    const unsigned int _rowCountInRange() const;

                    const TextBufferRow _endpointToTextBufferRow(_In_ const Endpoint endpoint) const;
                    const ScreenInfoRow _textBufferRowToScreenInfoRow(_In_ const TextBufferRow row) const;

                    const TextBufferRow _screenInfoRowToTextBufferRow(_In_ const ScreenInfoRow row) const;
                    const TextBufferRow _screenInfoRowToTextBufferRowNoNormalize(_In_ const ScreenInfoRow row) const;
                    const Endpoint _textBufferRowToEndpoint(_In_ const TextBufferRow row) const;

                    const ScreenInfoRow _endpointToScreenInfoRow(_In_ const Endpoint endpoint) const;
                    const Endpoint _screenInfoRowToEndpoint(_In_ const ScreenInfoRow row) const;

                    static const Column _endpointToColumn(_In_ const Endpoint endpoint);

                    const Row _normalizeRow(_In_ const Row row) const;

                    const ViewportRow _screenInfoRowToViewportRow(_In_ const ScreenInfoRow row) const;
                    const ViewportRow _screenInfoRowToViewportRow(_In_ const ScreenInfoRow row,
                                                                  _In_ const Viewport viewport) const;

                    const bool _isScreenInfoRowInViewport(_In_ const ScreenInfoRow row) const;
                    const bool _isScreenInfoRowInViewport(_In_ const ScreenInfoRow row,
                                                          _In_ const Viewport viewport) const;

                    static const unsigned int _getViewportHeight(_In_ const Viewport viewport);
                    static const unsigned int _getViewportWidth(_In_ const Viewport viewport);

                    #ifdef UNIT_TESTING
                    friend class ::UiaTextRangeTests;
                    #endif
                };
            }
        }
    }
}