/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#include "precomp.h"

#include "windowUiaProvider.hpp"
#include "window.hpp"

#include "screenInfoUiaProvider.hpp"

using namespace Microsoft::Console::Interactivity::Win32;

WindowUiaProvider::WindowUiaProvider(_In_ Window* const pWindow) :
    _pWindow(pWindow),
    _cRefs(1)
{

}

WindowUiaProvider::~WindowUiaProvider()
{

}

#pragma region IUnknown

IFACEMETHODIMP_(ULONG) WindowUiaProvider::AddRef()
{
    return InterlockedIncrement(&_cRefs);
}

IFACEMETHODIMP_(ULONG) WindowUiaProvider::Release()
{
    long val = InterlockedDecrement(&_cRefs);
    if (val == 0)
    {
        delete this;
    }
    return val;
}

IFACEMETHODIMP WindowUiaProvider::QueryInterface(_In_ REFIID riid, _COM_Outptr_result_maybenull_ void** ppInterface)
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
        *ppInterface = nullptr;
        return E_NOINTERFACE;
    }

    (static_cast<IUnknown*>(*ppInterface))->AddRef();

    return S_OK;
}

#pragma endregion

#pragma region IRawElementProviderSimple

// Implementation of IRawElementProviderSimple::get_ProviderOptions.
// Gets UI Automation provider options.
IFACEMETHODIMP WindowUiaProvider::get_ProviderOptions(_Out_ ProviderOptions* pOptions)
{
    RETURN_IF_FAILED(_EnsureValidHwnd());

    *pOptions = ProviderOptions_ServerSideProvider;
    return S_OK;
}

// Implementation of IRawElementProviderSimple::get_PatternProvider.
// Gets the object that supports ISelectionPattern.
IFACEMETHODIMP WindowUiaProvider::GetPatternProvider(_In_ PATTERNID patternId, _COM_Outptr_result_maybenull_ IUnknown** ppInterface)
{
    UNREFERENCED_PARAMETER(patternId);
    RETURN_IF_FAILED(_EnsureValidHwnd());

    *ppInterface = nullptr;
    return S_OK;
}

// Implementation of IRawElementProviderSimple::get_PropertyValue.
// Gets custom properties.
IFACEMETHODIMP WindowUiaProvider::GetPropertyValue(_In_ PROPERTYID propertyId, _Out_ VARIANT* pVariant)
{
    RETURN_IF_FAILED(_EnsureValidHwnd());

    pVariant->vt = VT_EMPTY;

    // Returning the default will leave the property as the default
    // so we only really need to touch it for the properties we want to implement
    if (propertyId == UIA_ControlTypePropertyId)
    {
        pVariant->vt = VT_I4;
        pVariant->lVal = UIA_WindowControlTypeId;
    }
    else if (propertyId == UIA_AutomationIdPropertyId)
    {
        pVariant->bstrVal = SysAllocString(L"Console Window");
        if (pVariant->bstrVal != nullptr)
        {
            pVariant->vt = VT_BSTR;
        }
    }
    else if (propertyId == UIA_IsControlElementPropertyId)
    {
        pVariant->vt = VT_BOOL;
        pVariant->boolVal = VARIANT_TRUE;
    }
    else if (propertyId == UIA_IsKeyboardFocusablePropertyId)
    {
        pVariant->vt = VT_BOOL;
        pVariant->boolVal = VARIANT_FALSE;
    }
    else if (propertyId == UIA_HasKeyboardFocusPropertyId)
    {
        pVariant->vt = VT_BOOL;
        pVariant->boolVal = VARIANT_FALSE;
    }
    else if (propertyId == UIA_ProviderDescriptionPropertyId)
    {
        pVariant->bstrVal = SysAllocString(L"Microsoft Console Host Window");
        if (pVariant->bstrVal != nullptr)
        {
            pVariant->vt = VT_BSTR;
        }
    }

    return S_OK;
}

// Implementation of IRawElementProviderSimple::get_HostRawElementProvider.
// Gets the default UI Automation provider for the host window. This provider 
// supplies many properties.
IFACEMETHODIMP WindowUiaProvider::get_HostRawElementProvider(_COM_Outptr_result_maybenull_ IRawElementProviderSimple** ppProvider)
{
    RETURN_HR_IF_NULL((HRESULT)UIA_E_ELEMENTNOTAVAILABLE, _pWindow);

    HWND const hwnd = _pWindow->GetWindowHandle();
    RETURN_HR_IF_NULL((HRESULT)UIA_E_ELEMENTNOTAVAILABLE, hwnd);

    return UiaHostProviderFromHwnd(hwnd, ppProvider);
}
#pragma endregion

#pragma region IRawElementProviderFragment

IFACEMETHODIMP WindowUiaProvider::Navigate(_In_ NavigateDirection direction, _COM_Outptr_result_maybenull_ IRawElementProviderFragment** ppProvider)
{
    RETURN_IF_FAILED(_EnsureValidHwnd());
    *ppProvider = nullptr;

    if (direction == NavigateDirection_FirstChild || direction == NavigateDirection_LastChild)
    {
        *ppProvider = _GetScreenInfoProvider();
        RETURN_IF_NULL_ALLOC(*ppProvider);
    }

    // For the other directions (parent, next, previous) the default of nullptr is correct
    return S_OK;
}

IFACEMETHODIMP WindowUiaProvider::GetRuntimeId(_Outptr_result_maybenull_ SAFEARRAY** ppRuntimeId)
{
    RETURN_IF_FAILED(_EnsureValidHwnd());
    // Root defers this to host, others must implement it...
    *ppRuntimeId = nullptr;

    return S_OK;
}

IFACEMETHODIMP WindowUiaProvider::get_BoundingRectangle(_Out_ UiaRect* pRect)
{
    RETURN_IF_FAILED(_EnsureValidHwnd());

    RETURN_HR_IF_NULL((HRESULT)UIA_E_ELEMENTNOTAVAILABLE, _pWindow);

    RECT const rc = _pWindow->GetWindowRect();

    pRect->left = rc.left;
    pRect->top = rc.top;
    pRect->width = rc.right - rc.left;
    pRect->height = rc.bottom - rc.top;

    return S_OK;
}

IFACEMETHODIMP WindowUiaProvider::GetEmbeddedFragmentRoots(_Outptr_result_maybenull_ SAFEARRAY** ppRoots)
{
    RETURN_IF_FAILED(_EnsureValidHwnd());

    *ppRoots = nullptr;
    return S_OK;
}

IFACEMETHODIMP WindowUiaProvider::SetFocus()
{
    RETURN_IF_FAILED(_EnsureValidHwnd());

    return S_OK;
}

IFACEMETHODIMP WindowUiaProvider::get_FragmentRoot(_COM_Outptr_result_maybenull_ IRawElementProviderFragmentRoot** ppProvider)
{
    RETURN_IF_FAILED(_EnsureValidHwnd());

    *ppProvider = this;
    AddRef();
    return S_OK;
}

#pragma endregion

#pragma region IRawElementProviderFragmentRoot

IFACEMETHODIMP WindowUiaProvider::ElementProviderFromPoint(_In_ double x, _In_ double y, _COM_Outptr_result_maybenull_ IRawElementProviderFragment** ppProvider)
{
    RETURN_IF_FAILED(_EnsureValidHwnd());

    *ppProvider = nullptr;

    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);

    *ppProvider = _GetScreenInfoProvider();
    RETURN_IF_NULL_ALLOC(*ppProvider);

    return S_OK;
}

IFACEMETHODIMP WindowUiaProvider::GetFocus(_COM_Outptr_result_maybenull_ IRawElementProviderFragment** ppProvider)
{
    RETURN_IF_FAILED(_EnsureValidHwnd());

    *ppProvider = nullptr;
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
