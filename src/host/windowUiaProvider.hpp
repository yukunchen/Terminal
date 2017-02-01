/*++
Copyright (c) Microsoft Corporation

Module Name:
- windowUiaProvider.hpp

Abstract:
- This module provides UI Automation access to the console window to support both automation tests and accessibility (screen reading) applications.
- Based on examples, sample code, and guidance from https://msdn.microsoft.com/en-us/library/windows/desktop/ee671596(v=vs.85).aspx

Author(s):
- Michael Niksa (MiNiksa)     2017
--*/
#pragma once

// Forward declare, prevent circular ref.
class Window;

class WindowUiaProvider : public IRawElementProviderSimple,
                          public IRawElementProviderFragment,
                          public IRawElementProviderFragmentRoot
{
public:
    WindowUiaProvider(_In_ Window* const pWindow);

    // IUnknown methods
    IFACEMETHODIMP_(ULONG) AddRef();
    IFACEMETHODIMP_(ULONG) Release();
    IFACEMETHODIMP QueryInterface(REFIID riid, void**ppInterface);

    // IRawElementProviderSimple methods
    IFACEMETHODIMP get_ProviderOptions(ProviderOptions * pRetVal);
    IFACEMETHODIMP GetPatternProvider(PATTERNID iid, IUnknown * * pRetVal);
    IFACEMETHODIMP GetPropertyValue(PROPERTYID idProp, VARIANT * pRetVal);
    IFACEMETHODIMP get_HostRawElementProvider(IRawElementProviderSimple ** pRetVal);

    // IRawElementProviderFragment methods
    HRESULT STDMETHODCALLTYPE Navigate(NavigateDirection direction, _Outptr_result_maybenull_ IRawElementProviderFragment ** retVal);
    HRESULT STDMETHODCALLTYPE GetRuntimeId(_Outptr_result_maybenull_ SAFEARRAY ** retVal);
    HRESULT STDMETHODCALLTYPE get_BoundingRectangle(_Out_ UiaRect * retVal);
    HRESULT STDMETHODCALLTYPE GetEmbeddedFragmentRoots(_Outptr_result_maybenull_ SAFEARRAY ** retVal);
    HRESULT STDMETHODCALLTYPE SetFocus();
    HRESULT STDMETHODCALLTYPE get_FragmentRoot(_Outptr_result_maybenull_ IRawElementProviderFragmentRoot * * retVal);

    // IRawElementProviderFragmenRoot methods
    HRESULT STDMETHODCALLTYPE ElementProviderFromPoint(double x, double y, _Outptr_result_maybenull_ IRawElementProviderFragment ** retVal);
    HRESULT STDMETHODCALLTYPE GetFocus(_Outptr_result_maybenull_ IRawElementProviderFragment ** retVal);

private:

    HWND _GetHwnd() const;
    HRESULT _EnsureValidHwnd() const;

    // Ref counter for COM object
    ULONG _refCount;

    Window* const _pWindow;
};
