/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#include "precomp.h"

#include "windowUiaProvider.hpp"
#include "window.hpp"

WindowUiaProvider::WindowUiaProvider(_In_ Window* const pWindow) :
    _pWindow(pWindow),
    _refCount(1)
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
    *pRetVal = ProviderOptions_ServerSideProvider;
    return S_OK;
}

// Implementation of IRawElementProviderSimple::get_PatternProvider.
// Gets the object that supports ISelectionPattern.
IFACEMETHODIMP WindowUiaProvider::GetPatternProvider(PATTERNID patternId, IUnknown** pRetVal)
{
    *pRetVal = NULL;
    patternId;
    /*if (patternId == UIA_SelectionPatternId)
    {
        *pRetVal = static_cast<IRawElementProviderSimple*>(this);
        AddRef();
    }*/
    return S_OK;
}

// Implementation of IRawElementProviderSimple::get_PropertyValue.
// Gets custom properties.
IFACEMETHODIMP WindowUiaProvider::GetPropertyValue(PROPERTYID propertyId, VARIANT* pRetVal)
{
    // Although it is hard-coded for the purposes of this sample, localizable 
    // text should be stored in, and loaded from, the resource file (.rc). 
    if (propertyId == UIA_LocalizedControlTypePropertyId)
    {
        pRetVal->vt = VT_BSTR;
        pRetVal->bstrVal = SysAllocString(L"contact list");
    }
    else if (propertyId == UIA_ControlTypePropertyId)
    {
        pRetVal->vt = VT_I4;
        pRetVal->lVal = UIA_DocumentControlTypeId;
    }
    else if (propertyId == UIA_IsKeyboardFocusablePropertyId)
    {
        pRetVal->vt = VT_BOOL;
        pRetVal->boolVal = VARIANT_TRUE;
    }
    // else pRetVal is empty, and UI Automation will attempt to get the property from
    //  the HostRawElementProvider, which is the default provider for the HWND.
    // Note that the Name property comes from the Caption property of the control window, 
    //  if it has one.
    else
    {
        pRetVal->vt = VT_EMPTY;
    }
    return S_OK;
}

// Implementation of IRawElementProviderSimple::get_HostRawElementProvider.
// Gets the default UI Automation provider for the host window. This provider 
// supplies many properties.
IFACEMETHODIMP WindowUiaProvider::get_HostRawElementProvider(IRawElementProviderSimple** pRetVal)
{
    RETURN_HR_IF_NULL((HRESULT)UIA_E_ELEMENTNOTAVAILABLE, _pWindow);

    HWND const hwnd = _pWindow->GetWindowHandle();

    RETURN_HR_IF_NULL((HRESULT)UIA_E_ELEMENTNOTAVAILABLE, hwnd);

    return UiaHostProviderFromHwnd(hwnd, pRetVal);
}
#pragma endregion
