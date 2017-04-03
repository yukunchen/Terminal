
#include "precomp.h"
#include "UiaTextRange.hpp"

// we are using the UIA API's idea of what equality is (the ranges match).
/*
bool operator==(const UiaTextRange& a, const UiaTextRange& b)
{
    return (a._textUnit == b._textUnit &&
            a._lineNumberStart == b._lineNumberStart,
            a._lineNumberEnd == b._lineNumberEnd,
            a._unitNumberStart == b._unitNumberStart,
            a._unitNumberEnd == b._unitNumberEnd);
}
*/

/*
UiaTextRange::UiaTextRange(IRawElementProviderSimple* pProvider,
                           const TEXT_BUFFER* const pOutputBuffer,
                           TextUnit textUnit,
                           const COORD currentFontSize) :
    _cRefs{ 1 },
    _pProvider{ pProvider },
    _pOutputBuffer{ pOutputBuffer },
    _textUnit{ textUnit },
    _currentFontSize{ currentFontSize },
    _lineNumberStart{ 0 },
    _lineNumberEnd{ 0 },
    _unitNumberStart{ 0 },
    _unitNumberEnd{ 0 }
{
}
*/

/*
UiaTextRange::UiaTextRange(IRawElementProviderSimple* pProvider,
                           const TEXT_BUFFER_INFO* const pOutputBuffer,
                           TextUnit textUnit,
                           const COORD currentFontSize,
                           size_t lineNumber,
                           size_t viewportLineNumber,
                           SMALL_RECT viewport) :
    _cRefs{ 1 },
    _pProvider{ pProvider },
    _pOutputBuffer{ pOutputBuffer },
    _textUnit{ textUnit },
    _currentFontSize{ currentFontSize },
    _viewportLineNumber{ viewportLineNumber },
    _lineNumberStart{ lineNumber },
    _lineNumberEnd{ lineNumber + 1 },
    _unitNumberStart{ 0 },
    _unitNumberEnd{ 0 },
    _viewport{ viewport }
{
}
*/

UiaTextRange::UiaTextRange(IRawElementProviderSimple* pProvider,
                           const TEXT_BUFFER_INFO* const pOutputBuffer,
                           SMALL_RECT viewport,
                           const COORD currentFontSize) :
    _cRefs{ 1 },
    _pProvider{ pProvider },
    _pOutputBuffer{ pOutputBuffer },
    _viewport{ viewport },
    _currentFontSize{ currentFontSize }
{
}



/*
UiaTextRange::UiaTextRange(IRawElementProviderSimple* pProvider,
                           const TEXT_BUFFER* const pOutputBuffer,
                           TextUnit textUnit,
                           const COORD currentFontSize,
                           size_t lineNumber,
                           size_t unitNumber) :
    _cRefs{ 1 },
    _pProvider{ pProvider },
    _pOutputBuffer{ pOutputBuffer },
    _textUnit{ textUnit },
    _currentFontSize{ currentFontSize },
    _lineNumberStart{ lineNumber },
    _lineNumberEnd{ lineNumber + 1 },
    _unitNumberStart{ unitNumber },
    _unitNumberEnd{ unitNumber + 1 }
{
}
*/

UiaTextRange::~UiaTextRange()
{
    _pProvider->Release();
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

/*
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
    HRESULT hr = S_OK;
    if (pRange)
    {
        UiaTextRange* uiaTextRange = dynamic_cast<UiaTextRange*>(pRange);
        if (uiaTextRange)
        {
            *pRetVal = !!(*this == *uiaTextRange);
        }
        else
        {
            hr = E_NOINTERFACE;
        }
    }
    return hr;
}

IFACEMETHODIMP UiaTextRange::CompareEndpoints(TextPatternRangeEndpoint endpoint,
                                            _In_opt_ ITextRangeProvider* pTargetRange,
                                            _In_ TextPatternRangeEndpoint targetEndpoint,
                                            _Out_ int* pRetVal)
{
    UNREFERENCED_PARAMETER(endpoint);
    UNREFERENCED_PARAMETER(pTargetRange);
    UNREFERENCED_PARAMETER(targetEndpoint);
    UNREFERENCED_PARAMETER(pRetVal);
    return E_NOTIMPL;
}

// TODO the docs on this function make no sense
IFACEMETHODIMP UiaTextRange::ExpandToEnclosingUnit(_In_ TextUnit unit)
{
    UNREFERENCED_PARAMETER(unit);
    //return E_NOTIMPL;
    return S_OK;
}

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

IFACEMETHODIMP UiaTextRange::FindText(_In_ BSTR text,
                                    BOOL searchBackward,
                                    BOOL ignoreCase,
                                    _Outptr_result_maybenull_ ITextRangeProvider** ppRetVal)
{
    UNREFERENCED_PARAMETER(text);
    UNREFERENCED_PARAMETER(searchBackward);
    UNREFERENCED_PARAMETER(ignoreCase);
    UNREFERENCED_PARAMETER(ppRetVal);
    return E_NOTIMPL;
}

IFACEMETHODIMP UiaTextRange::GetAttributeValue(_In_ TEXTATTRIBUTEID textAttributeId, _Out_ VARIANT* pRetVal)
{
    pRetVal->vt = VT_EMPTY;
    if (textAttributeId == UIA_AnimationStyleAttributeId)
    {
        pRetVal->vt = VT_I4;
        pRetVal->lVal = AnimationStyle_None;
    }
    // TODO the rest. Do I need to implement all of these or just the
    // ones that we care about?
    return S_OK;
    //UNREFERENCED_PARAMETER(textAttributeId);
    //UNREFERENCED_PARAMETER(pRetVal);
    //return E_NOTIMPL;
}

IFACEMETHODIMP UiaTextRange::GetBoundingRectangles(_Outptr_result_maybenull_ SAFEARRAY** ppRetVal)
{
    // TODO for line only currently
    // get the font size
    size_t charLineLength = _viewport.Right - _viewport.Left + 1;
    POINT topLeft;
    POINT bottomRight;
    topLeft.x = 0;
    topLeft.y = static_cast<LONG>(_currentFontSize.Y * _viewportLineNumber);
    bottomRight.x = static_cast<LONG>(topLeft.x + (_currentFontSize.X * charLineLength));
    bottomRight.y = topLeft.y + _currentFontSize.Y; // only 1 line tall


    // convert the result from the client window to the screen
    HWND hwnd = g_ciConsoleInformation.hWnd;
    ClientToScreen(hwnd, &topLeft);
    ClientToScreen(hwnd, &bottomRight);


    // stuff the result in a safearray. it goes in as four doubles in
    // the order: left, top, width, height
	*ppRetVal = SafeArrayCreateVector(VT_R8, 0, 4);

    double inputData = topLeft.x;
    LONG currentElement = 0;
	SafeArrayPutElement(*ppRetVal, &currentElement, &inputData);

	currentElement = 1;
    inputData = topLeft.y;
	SafeArrayPutElement(*ppRetVal, &currentElement, &inputData);

	currentElement = 2;
    inputData = bottomRight.x - topLeft.x;
	SafeArrayPutElement(*ppRetVal, &currentElement, &inputData);

	currentElement = 3;
    inputData = bottomRight.y - topLeft.y;
	SafeArrayPutElement(*ppRetVal, &currentElement, &inputData);

	return S_OK;
}

IFACEMETHODIMP UiaTextRange::GetEnclosingElement(_Outptr_result_maybenull_ IRawElementProviderSimple** ppRetVal)
{
    //UNREFERENCED_PARAMETER(ppRetVal);
    //return E_NOTIMPL;
    *ppRetVal = _pProvider;
    _pProvider->AddRef();
    return S_OK;
}

IFACEMETHODIMP UiaTextRange::GetText(_In_ int maxLength, _Out_ BSTR* pRetVal)
{
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
        *pRetVal = SysAllocString(wstr.c_str());
    }
    else
    {
        *pRetVal = SysAllocString(wstr.c_str());
    }
    return S_OK;
}

IFACEMETHODIMP UiaTextRange::Move(_In_ TextUnit unit, _In_ int count, _Out_ int* pRetVal)
{
    UNREFERENCED_PARAMETER(unit);
    UNREFERENCED_PARAMETER(count);
    UNREFERENCED_PARAMETER(pRetVal);
    return E_NOTIMPL;
}

IFACEMETHODIMP UiaTextRange::MoveEndpointByUnit(_In_ TextPatternRangeEndpoint endpoint,
                                                _In_ TextUnit unit,
                                                _In_ int count,
                                                _Out_ int* pRetVal)
{
    UNREFERENCED_PARAMETER(endpoint);
    UNREFERENCED_PARAMETER(unit);
    UNREFERENCED_PARAMETER(count);
    UNREFERENCED_PARAMETER(pRetVal);
    return E_NOTIMPL;
}

IFACEMETHODIMP UiaTextRange::MoveEndpointByRange(_In_ TextPatternRangeEndpoint endpoint,
                                                _In_opt_ ITextRangeProvider* pTargetRange,
                                                _In_ TextPatternRangeEndpoint targetEndpoint)
{
    UNREFERENCED_PARAMETER(endpoint);
    UNREFERENCED_PARAMETER(pTargetRange);
    UNREFERENCED_PARAMETER(targetEndpoint);
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
    UNREFERENCED_PARAMETER(alignToTop);
    //return E_NOTIMPL;
    // TODO
    return S_OK;
}

// TODO should we be returning individual chars?
IFACEMETHODIMP UiaTextRange::GetChildren(_Outptr_result_maybenull_ SAFEARRAY** ppRetVal)
{
	*ppRetVal = SafeArrayCreateVector(VT_UNKNOWN, 0, 0);
    return S_OK;
}


#pragma endregion
*/