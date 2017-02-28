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
class ScreenInfoUiaProvider;

class WindowUiaProvider : public IRawElementProviderSimple,
                          public IRawElementProviderFragment,
                          public IRawElementProviderFragmentRoot
{
public:
    WindowUiaProvider(_In_ Window* const pWindow);
    virtual ~WindowUiaProvider();

    // IUnknown methods
    IFACEMETHODIMP_(ULONG) AddRef();
    IFACEMETHODIMP_(ULONG) Release();
    IFACEMETHODIMP QueryInterface(_In_ REFIID riid, _Outptr_result_maybenull_ void** ppInterface);

    // IRawElementProviderSimple methods
    IFACEMETHODIMP get_ProviderOptions(_Out_ ProviderOptions* pRetVal);
    IFACEMETHODIMP GetPatternProvider(_In_ PATTERNID iid, _Outptr_result_maybenull_ IUnknown** ppRetVal);
    IFACEMETHODIMP GetPropertyValue(_In_ PROPERTYID idProp, _Out_ VARIANT* pRetVal);
    IFACEMETHODIMP get_HostRawElementProvider(_Outptr_result_maybenull_ IRawElementProviderSimple** pRetVal);

    // IRawElementProviderFragment methods
    HRESULT STDMETHODCALLTYPE Navigate(_In_ NavigateDirection direction, _Outptr_result_maybenull_ IRawElementProviderFragment** ppRetVal);
    HRESULT STDMETHODCALLTYPE GetRuntimeId(_Outptr_result_maybenull_ SAFEARRAY** ppRetVal);
    HRESULT STDMETHODCALLTYPE get_BoundingRectangle(_Out_ UiaRect* pRetVal);
    HRESULT STDMETHODCALLTYPE GetEmbeddedFragmentRoots(_Outptr_result_maybenull_ SAFEARRAY** ppRetVal);
    HRESULT STDMETHODCALLTYPE SetFocus();
    HRESULT STDMETHODCALLTYPE get_FragmentRoot(_Outptr_result_maybenull_ IRawElementProviderFragmentRoot** ppRetVal);

    // IRawElementProviderFragmentRoot methods
    HRESULT STDMETHODCALLTYPE ElementProviderFromPoint(_In_ double x, _In_ double y, _Outptr_result_maybenull_ IRawElementProviderFragment** ppRetVal);
    HRESULT STDMETHODCALLTYPE GetFocus(_Outptr_result_maybenull_ IRawElementProviderFragment** ppRetVal);

private:

    // Ref counter for COM object
    ULONG _refCount;

    Window* const _pWindow;

    HWND _GetWindowHandle() const;
    HRESULT _EnsureValidHwnd() const;

    ScreenInfoUiaProvider* _GetScreenInfoProvider() const;
};
