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
        return NULL;

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

IFACEMETHODIMP ScreenInfoUiaProvider::QueryInterface(REFIID riid, void** ppInterface)
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
IFACEMETHODIMP ScreenInfoUiaProvider::get_ProviderOptions(ProviderOptions* pRetVal)
{
    *pRetVal = ProviderOptions_ServerSideProvider;
    return S_OK;
}

// Implementation of IRawElementProviderSimple::get_PatternProvider.
// Gets the object that supports ISelectionPattern.
IFACEMETHODIMP ScreenInfoUiaProvider::GetPatternProvider(PATTERNID patternId, IUnknown** ppRetVal)
{
    UNREFERENCED_PARAMETER(patternId);

    *ppRetVal = NULL;

    // TODO: insert patterns here 

    return S_OK;
}

// Implementation of IRawElementProviderSimple::get_PropertyValue.
// Gets custom properties.
IFACEMETHODIMP ScreenInfoUiaProvider::GetPropertyValue(PROPERTYID propertyId, VARIANT* pRetVal)
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

IFACEMETHODIMP ScreenInfoUiaProvider::get_HostRawElementProvider(IRawElementProviderSimple** pRetVal)
{
    *pRetVal = NULL;

    return S_OK;
}
#pragma endregion

#pragma region IRawElementProviderFragment

HRESULT STDMETHODCALLTYPE ScreenInfoUiaProvider::Navigate(NavigateDirection direction, _Outptr_result_maybenull_ IRawElementProviderFragment ** retVal)
{
    *retVal = NULL;

    if (direction == NavigateDirection_Parent)
    {
        *retVal = new WindowUiaProvider(_pWindow);
        RETURN_IF_NULL_ALLOC(*retVal);
    }

    // For the other directions the default of NULL is correct
    return S_OK;
}

HRESULT STDMETHODCALLTYPE ScreenInfoUiaProvider::GetRuntimeId(_Outptr_result_maybenull_ SAFEARRAY ** ppRetVal)
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

HRESULT STDMETHODCALLTYPE ScreenInfoUiaProvider::get_BoundingRectangle(_Out_ UiaRect * retVal)
{
    RETURN_HR_IF_NULL((HRESULT)UIA_E_ELEMENTNOTAVAILABLE, _pWindow);

    RECT const rc = _pWindow->GetWindowRect();

    retVal->left = rc.left;
    retVal->top = rc.top;
    retVal->width = rc.right - rc.left;
    retVal->height = rc.bottom - rc.top;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE ScreenInfoUiaProvider::GetEmbeddedFragmentRoots(_Outptr_result_maybenull_ SAFEARRAY ** retVal)
{
    *retVal = NULL;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE ScreenInfoUiaProvider::SetFocus()
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE ScreenInfoUiaProvider::get_FragmentRoot(_Outptr_result_maybenull_ IRawElementProviderFragmentRoot ** retVal)
{
    *retVal = new WindowUiaProvider(_pWindow);
    RETURN_IF_NULL_ALLOC(*retVal);
    return S_OK;
}

#pragma endregion
