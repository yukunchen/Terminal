/*++
Copyright (c) Microsoft Corporation

Module Name:
- windowUiaProvider.hpp

Abstract:
- This module provides UI Automation access to the console window to
  support both automation tests and accessibility (screen reading)
  applications.
- Based on examples, sample code, and guidance from
  https://msdn.microsoft.com/en-us/library/windows/desktop/ee671596(v=vs.85).aspx

Author(s):
- Michael Niksa (MiNiksa)     2017
--*/

#pragma once

namespace Microsoft
{
    namespace Console
    {
        namespace Interactivity
        {
            namespace Win32
            {
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

                    void Signal(_In_ EVENTID id);

                    // IUnknown methods
                    IFACEMETHODIMP_(ULONG) AddRef();
                    IFACEMETHODIMP_(ULONG) Release();
                    IFACEMETHODIMP QueryInterface(_In_ REFIID riid,
                                                  _COM_Outptr_result_maybenull_ void** ppInterface);

                    // IRawElementProviderSimple methods
                    IFACEMETHODIMP get_ProviderOptions(_Out_ ProviderOptions* pOptions);
                    IFACEMETHODIMP GetPatternProvider(_In_ PATTERNID iid,
                                                      _COM_Outptr_result_maybenull_ IUnknown** ppInterface);
                    IFACEMETHODIMP GetPropertyValue(_In_ PROPERTYID idProp,
                                                    _Out_ VARIANT* pVariant);
                    IFACEMETHODIMP get_HostRawElementProvider(_COM_Outptr_result_maybenull_ IRawElementProviderSimple** ppProvider);

                    // IRawElementProviderFragment methods
                    IFACEMETHODIMP Navigate(_In_ NavigateDirection direction,
                                            _COM_Outptr_result_maybenull_ IRawElementProviderFragment** ppProvider);
                    IFACEMETHODIMP GetRuntimeId(_Outptr_result_maybenull_ SAFEARRAY** ppRuntimeId);
                    IFACEMETHODIMP get_BoundingRectangle(_Out_ UiaRect* pRect);
                    IFACEMETHODIMP GetEmbeddedFragmentRoots(_Outptr_result_maybenull_ SAFEARRAY** ppRoots);
                    IFACEMETHODIMP SetFocus();
                    IFACEMETHODIMP get_FragmentRoot(_COM_Outptr_result_maybenull_ IRawElementProviderFragmentRoot** ppProvider);

                    // IRawElementProviderFragmentRoot methods
                    IFACEMETHODIMP ElementProviderFromPoint(_In_ double x,
                                                            _In_ double y,
                                                            _COM_Outptr_result_maybenull_ IRawElementProviderFragment** ppProvider);
                    IFACEMETHODIMP GetFocus(_COM_Outptr_result_maybenull_ IRawElementProviderFragment** ppProvider);

                private:

                    // Ref counter for COM object
                    ULONG _cRefs;

                    Window* const _pWindow;

                    HWND _GetWindowHandle() const;
                    HRESULT _EnsureValidHwnd() const;

                    ScreenInfoUiaProvider* _GetScreenInfoProvider() const;
                };
            }
        }
    }
}
