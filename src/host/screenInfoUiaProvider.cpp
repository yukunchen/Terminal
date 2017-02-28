/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#include "precomp.h"

#include "screenInfoUiaProvider.hpp"
#include "screenInfo.hpp"

#include "windowUiaProvider.hpp"
#include "window.hpp"

// A helper function to create a SafeArray Version of an int array of a specified length
SAFEARRAY * BuildIntSafeArray(_In_reads_(length) const int * data, _In_ int length)
{
    SAFEARRAY *sa = SafeArrayCreateVector(VT_I4, 0, length);
    if (sa == NULL)
    {
        return NULL;
    }

    for (long i = 0; i < length; i++)
    {
        if (FAILED(SafeArrayPutElement(sa, &i, (void *)&(data[i]))))
        {
            SafeArrayDestroy(sa);
            sa = NULL;
            break;
        }
    }

    return sa;
}

ScreenInfoUiaProvider::ScreenInfoUiaProvider(_In_ Window* const pParent, _In_ SCREEN_INFORMATION* const pScreenInfo) :
    _pWindow(pParent),
    _pScreenInfo(pScreenInfo),
    _refCount(1)
{
}

ScreenInfoUiaProvider::~ScreenInfoUiaProvider()
{

}

#pragma region IUnknown

IFACEMETHODIMP_(ULONG) ScreenInfoUiaProvider::AddRef()
{
    return InterlockedIncrement(&_refCount);
}

IFACEMETHODIMP_(ULONG) ScreenInfoUiaProvider::Release()
{
    long val = InterlockedDecrement(&_refCount);
    if (val == 0)
    {
        delete this;
    }
    return val;
}

IFACEMETHODIMP ScreenInfoUiaProvider::QueryInterface(_In_ REFIID riid, _Outptr_result_maybenull_ void** ppInterface)
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
IFACEMETHODIMP ScreenInfoUiaProvider::get_ProviderOptions(_Out_ ProviderOptions* pRetVal)
{
    *pRetVal = ProviderOptions_ServerSideProvider;
    return S_OK;
}

// Implementation of IRawElementProviderSimple::get_PatternProvider.
// Gets the object that supports ISelectionPattern.
IFACEMETHODIMP ScreenInfoUiaProvider::GetPatternProvider(_In_ PATTERNID patternId, _Outptr_result_maybenull_ IUnknown** ppRetVal)
{
    UNREFERENCED_PARAMETER(patternId);

    *ppRetVal = NULL;

    // TODO: MSFT: 7960168 - insert patterns here 

    return S_OK;
}

// Implementation of IRawElementProviderSimple::get_PropertyValue.
// Gets custom properties.
IFACEMETHODIMP ScreenInfoUiaProvider::GetPropertyValue(_In_ PROPERTYID propertyId, _Out_ VARIANT* pRetVal)
{
    pRetVal->vt = VT_EMPTY;

    // Returning the default will leave the property as the default
    // so we only really need to touch it for the properties we want to implement
    if (propertyId == UIA_ControlTypePropertyId)
    {
        // This control is the Document control type, implying that it is
        // a complex document that supports text pattern
        pRetVal->vt = VT_I4;
        pRetVal->lVal = UIA_DocumentControlTypeId;
    }
    else if (propertyId == UIA_NamePropertyId)
    {
        // In a production application, this would be localized text.
        pRetVal->bstrVal = SysAllocString(L"Text Area");
        if (pRetVal->bstrVal != NULL)
        {
            pRetVal->vt = VT_BSTR;
        }
    }
    else if (propertyId == UIA_AutomationIdPropertyId)
    {
        pRetVal->bstrVal = SysAllocString(L"Text Area");
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
    else if (propertyId == UIA_IsContentElementPropertyId)
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
        pRetVal->bstrVal = SysAllocString(L"Microsoft Console Host: Screen Information Text Area");
        if (pRetVal->bstrVal != NULL)
        {
            pRetVal->vt = VT_BSTR;
        }
    }

    return S_OK;
}

IFACEMETHODIMP ScreenInfoUiaProvider::get_HostRawElementProvider(_Outptr_result_maybenull_ IRawElementProviderSimple** ppRetVal)
{
    *ppRetVal = NULL;

    return S_OK;
}
#pragma endregion

#pragma region IRawElementProviderFragment

HRESULT STDMETHODCALLTYPE ScreenInfoUiaProvider::Navigate(_In_ NavigateDirection direction, _Outptr_result_maybenull_ IRawElementProviderFragment** ppRetVal)
{
    *ppRetVal = NULL;

    if (direction == NavigateDirection_Parent)
    {
        *ppRetVal = new WindowUiaProvider(_pWindow);
        RETURN_IF_NULL_ALLOC(*ppRetVal);
    }

    // For the other directions the default of NULL is correct
    return S_OK;
}

HRESULT STDMETHODCALLTYPE ScreenInfoUiaProvider::GetRuntimeId(_Outptr_result_maybenull_ SAFEARRAY** ppRetVal)
{
    // Root defers this to host, others must implement it...
    *ppRetVal = NULL;

    // AppendRuntimeId is a magic Number that tells UIAutomation to Append its own Runtime ID(From the HWND)
    int rId[] = { UiaAppendRuntimeId, -1 };
    // BuildIntSafeArray is a custom function to hide the SafeArray creation
    *ppRetVal = BuildIntSafeArray(rId, 2);
    RETURN_IF_NULL_ALLOC(*ppRetVal);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE ScreenInfoUiaProvider::get_BoundingRectangle(_Out_ UiaRect* pRetVal)
{
    RETURN_HR_IF_NULL((HRESULT)UIA_E_ELEMENTNOTAVAILABLE, _pWindow);

    RECT const rc = _pWindow->GetWindowRect();

    pRetVal->left = rc.left;
    pRetVal->top = rc.top;
    pRetVal->width = rc.right - rc.left;
    pRetVal->height = rc.bottom - rc.top;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE ScreenInfoUiaProvider::GetEmbeddedFragmentRoots(_Outptr_result_maybenull_ SAFEARRAY** ppRetVal)
{
    *ppRetVal = NULL;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE ScreenInfoUiaProvider::SetFocus()
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE ScreenInfoUiaProvider::get_FragmentRoot(_Outptr_result_maybenull_ IRawElementProviderFragmentRoot** ppRetVal)
{
    *ppRetVal = new WindowUiaProvider(_pWindow);
    RETURN_IF_NULL_ALLOC(*ppRetVal);
    return S_OK;
}

#pragma endregion
