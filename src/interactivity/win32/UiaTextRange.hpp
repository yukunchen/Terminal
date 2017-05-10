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
                                 _In_ const int start,
                                 _In_ const int end);

                    // range from a UiaPoint
                    UiaTextRange(_In_ IRawElementProviderSimple* const pProvider,
                                 _In_ const TEXT_BUFFER_INFO* const pOutputBuffer,
                                 _In_ const SCREEN_INFORMATION* const pScreenInfo,
                                 _In_ const UiaPoint point);

                    UiaTextRange(_In_ const UiaTextRange& a);

                    ~UiaTextRange();


                    const int getStart() const;
                    const int getEnd() const;

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
                    int _start;
                    int _end;

                    const bool _isDegenerate() const;
                    const bool _isLineInViewport(const int lineNumber) const;
                    const bool _isLineInViewport(const int lineNumber, const SMALL_RECT viewport) const;
                    const int _lineNumberToViewport(const int lineNumber) const;
                    const int _endpointToRow(const int endpoint) const;
                    const int _endpointToColumn(const int endpoint) const;
                    const int _rowToEndpoint(const int row) const;
                    const int _getTotalRows() const;
                    const int _getRowWidth() const;
                    const SMALL_RECT _getViewport() const;
                    HWND _getWindowHandle();
                    IConsoleWindow* _getWindow();
                    const COORD _getScreenBufferCoords() const;
                };
            }
        }
    }
}