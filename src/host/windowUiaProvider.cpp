/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#include "precomp.h"

#include "windowUiaProvider.hpp"
#include "window.hpp"

#include "screenInfoUiaProvider.hpp"

WindowUiaProvider::WindowUiaProvider(_In_ Window* const pWindow) :
    _pWindow(pWindow),
    _refCount(1)
{

}

WindowUiaProvider::~WindowUiaProvider()
{

}

#pragma region IUnknown

IFACEMETHODIMP_(ULONG) WindowUiaProvider::AddRef()
{
    return InterlockedIncrement(&_refCount);
}

IFACEMETHODIMP_(ULONG) WindowUiaProvider::Release()
{
    long val = InterlockedDecrement(&_refCount);
    if (val == 0)
    {
        delete this;
    }
    return val;
}

IFACEMETHODIMP WindowUiaProvider::QueryInterface(REFIID riid, void** ppInterface)
{
    if (riid == __uuidof(IUnknown))
    {
        *ppInterface = static_cast<IRawElementProviderSimple*>(this);
    }
    else if (riid == __uuidof(IRawElementProviderSimple))
    {
        *ppInterface = static_cast<IRawElementProviderSimple*>(this);
    }
    else if (riid == __uuidof(IRawElementProviderFragment))
    {
        *ppInterface = static_cast<IRawElementProviderFragment*>(this);
    }
    else if (riid == __uuidof(IRawElementProviderFragmentRoot))
    {
        *ppInterface = static_cast<IRawElementProviderFragmentRoot*>(this);
    }
    else
    {
        *ppInterface = NULL;
        return E_NOINTERFACE;
    }

    (static_cast<IUnknown*>(*ppInterface))->AddRef();

    return S_OK;
}

#pragma endregion

#pragma region IRawElementProviderSimple

// Implementation of IRawElementProviderSimple::get_ProviderOptions.
// Gets UI Automation provider options.
IFACEMETHODIMP WindowUiaProvider::get_ProviderOptions(ProviderOptions* pRetVal)
{
    RETURN_IF_FAILED(_EnsureValidHwnd());

    *pRetVal = ProviderOptions_ServerSideProvider | ProviderOptions_UseComThreading;
    return S_OK;
}

// Implementation of IRawElementProviderSimple::get_PatternProvider.
// Gets the object that supports ISelectionPattern.
IFACEMETHODIMP WindowUiaProvider::GetPatternProvider(PATTERNID patternId, IUnknown** ppRetVal)
{
    UNREFERENCED_PARAMETER(patternId);
    RETURN_IF_FAILED(_EnsureValidHwnd());

    *ppRetVal = NULL;
    return S_OK;
}

// Implementation of IRawElementProviderSimple::get_PropertyValue.
// Gets custom properties.
IFACEMETHODIMP WindowUiaProvider::GetPropertyValue(PROPERTYID propertyId, VARIANT* pRetVal)
{
    RETURN_IF_FAILED(_EnsureValidHwnd());

    pRetVal->vt = VT_EMPTY;


    // Returning the default will leave the property as the default
    // so we only really need to touch it for the properties we want to implement
    if (propertyId == UIA_ControlTypePropertyId)
    {
        pRetVal->vt = VT_I4;
        pRetVal->lVal = UIA_PaneControlTypeId;
    }
    else if (propertyId == UIA_AutomationIdPropertyId)
    {
        pRetVal->bstrVal = SysAllocString(L"Console Window");
        if (pRetVal->bstrVal != NULL)
        {
            pRetVal->vt = VT_BSTR;
        }
    }
    else if (propertyId == UIA_IsControlElementPropertyId)
    {
        pRetVal->vt = VT_BOOL;
        pRetVal->boolVal = VARIANT_TRUE;
    }
    else if (propertyId == UIA_IsKeyboardFocusablePropertyId)
    {
        pRetVal->vt = VT_BOOL;
        pRetVal->boolVal = VARIANT_FALSE;
    }
    else if (propertyId == UIA_HasKeyboardFocusPropertyId)
    {
        pRetVal->vt = VT_BOOL;
        pRetVal->boolVal = VARIANT_FALSE;
    }
    else if (propertyId == UIA_ProviderDescriptionPropertyId)
    {
        pRetVal->bstrVal = SysAllocString(L"Microsoft Console Host Window");
        if (pRetVal->bstrVal != NULL)
        {
            pRetVal->vt = VT_BSTR;
        }
    }

    return S_OK;
}

// Implementation of IRawElementProviderSimple::get_HostRawElementProvider.
// Gets the default UI Automation provider for the host window. This provider 
// supplies many properties.
IFACEMETHODIMP WindowUiaProvider::get_HostRawElementProvider(IRawElementProviderSimple** pRetVal)
{
    RETURN_IF_FAILED(_EnsureValidHwnd());

    *pRetVal = this;
    AddRef();
    return S_OK;

    //RETURN_HR_IF_NULL((HRESULT)UIA_E_ELEMENTNOTAVAILABLE, _pWindow);

    //HWND const hwnd = _pWindow->GetWindowHandle();

    //RETURN_HR_IF_NULL((HRESULT)UIA_E_ELEMENTNOTAVAILABLE, hwnd);

    //return UiaHostProviderFromHwnd(hwnd, pRetVal);
}
#pragma endregion

#pragma region IRawElementProviderFragment

HRESULT STDMETHODCALLTYPE WindowUiaProvider::Navigate(NavigateDirection direction, _Outptr_result_maybenull_ IRawElementProviderFragment ** retVal)
{
    RETURN_IF_FAILED(_EnsureValidHwnd());
    *retVal = NULL;

    if (direction == NavigateDirection_FirstChild || direction == NavigateDirection_LastChild)
    {
        *retVal = _GetScreenInfoProvider();
        RETURN_IF_NULL_ALLOC(*retVal);
    }

    // For the other directions (parent, next, previous) the default of NULL is correct
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WindowUiaProvider::GetRuntimeId(_Outptr_result_maybenull_ SAFEARRAY ** retVal)
{
    RETURN_IF_FAILED(_EnsureValidHwnd());
    // Root defers this to host, others must implement it...
    *retVal = NULL;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WindowUiaProvider::get_BoundingRectangle(_Out_ UiaRect * retVal)
{
    RETURN_IF_FAILED(_EnsureValidHwnd());

    RETURN_HR_IF_NULL((HRESULT)UIA_E_ELEMENTNOTAVAILABLE, _pWindow);

    RECT const rc = _pWindow->GetWindowRect();

    retVal->left = rc.left;
    retVal->top = rc.top;
    retVal->width = rc.right - rc.left;
    retVal->height = rc.bottom - rc.top;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WindowUiaProvider::GetEmbeddedFragmentRoots(_Outptr_result_maybenull_ SAFEARRAY ** retVal)
{
    RETURN_IF_FAILED(_EnsureValidHwnd());

    *retVal = NULL;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WindowUiaProvider::SetFocus()
{
    RETURN_IF_FAILED(_EnsureValidHwnd());

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WindowUiaProvider::get_FragmentRoot(_Outptr_result_maybenull_ IRawElementProviderFragmentRoot ** retVal)
{
    RETURN_IF_FAILED(_EnsureValidHwnd());

    *retVal = this;
    AddRef();
    return S_OK;
}

#pragma endregion

#pragma region IRawElementProviderFragmentRoot

HRESULT STDMETHODCALLTYPE WindowUiaProvider::ElementProviderFromPoint(double x, double y, _Outptr_result_maybenull_ IRawElementProviderFragment ** ppRetVal)
{
    RETURN_IF_FAILED(_EnsureValidHwnd());

    *ppRetVal = NULL;

    x;
    y;

    *ppRetVal = _GetScreenInfoProvider();
    RETURN_IF_NULL_ALLOC(*ppRetVal);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WindowUiaProvider::GetFocus(_Outptr_result_maybenull_ IRawElementProviderFragment ** retVal)
{
    RETURN_IF_FAILED(_EnsureValidHwnd());

    *retVal = NULL;
    return S_OK;
}

#pragma endregion

HWND WindowUiaProvider::_GetWindowHandle() const
{
    HWND hwnd = nullptr;

    if (nullptr != _pWindow)
    {
        hwnd = _pWindow->GetWindowHandle();
    }

    return hwnd;
}

HRESULT WindowUiaProvider::_EnsureValidHwnd() const
{
    HWND const hwnd = _GetWindowHandle();

    RETURN_HR_IF_FALSE((HRESULT)UIA_E_ELEMENTNOTAVAILABLE, IsWindow(hwnd));

    return S_OK;
}

ScreenInfoUiaProvider* WindowUiaProvider::_GetScreenInfoProvider() const
{
    ScreenInfoUiaProvider* pProvider = nullptr;

    if (nullptr != _pWindow)
    {
        SCREEN_INFORMATION* const pScreenInfo = _pWindow->GetScreenInfo();
        pProvider = new ScreenInfoUiaProvider(_pWindow, pScreenInfo);
    }

    return pProvider;
}
