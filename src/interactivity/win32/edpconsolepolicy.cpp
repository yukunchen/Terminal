#include "precomp.h"
#include "EdpConsolePolicy.hpp"

#include <enterprisecontextstructs.h>
#include <enterprisedatapolicystructs.h>
#include <edppolicydefs.h>

using namespace Microsoft::Console::Interactivity::Win32;

// GetProcAddress helpers for EdpAuditClipboard()
BOOL EdpGetIsManaged(_In_ HMODULE _hEdpUtil);
HRESULT EdpGetEnterpriseIdForClipboard(_In_ HMODULE _hEdpUtil, _Outptr_result_maybenull_ PWSTR* enterpriseId);
HRESULT EdpGetDataInfoFromWin32Clipboard(_In_ HMODULE _hEdpUtil, _Outptr_result_z_ PWSTR* dataInfo);
HRESULT EdpGetSourceAppIdForClipboard(_In_ HMODULE _hEdpUtil, _Outptr_result_maybenull_ PWSTR* SourceAppId);
HRESULT EdpAuditAction(_In_ HMODULE _hEdpUtil,
                              _In_ PCWSTR ObjectDescription,
                              _In_ EDP_AUDIT_ACTION Action,
                              _In_opt_ PCWSTR SourceName,
                              _In_opt_ PCWSTR SourceEID,
                              _In_opt_ PCWSTR DestinationName,
                              _In_opt_ PCWSTR DestinationEID,
                              _In_opt_ PCWSTR Application);

void EdpConsolePolicy::EdpAuditClipboard()
{
    // If user is WIP enrolled, check event and audit if necessary
    if (EdpGetIsManaged(_hEdpUtil.get()))
    {
        wil::unique_cotaskmem_string enterpriseId, dataDescription, sourceDescription = L'\0';
        std::wstring DestinationName = _LoadString(ID_CONSOLE_WIP_DESTINATIONNAME);
        
        // Get enterprise id of clipboard's content if there is one
        HRESULT hr = EdpGetEnterpriseIdForClipboard(_hEdpUtil.get(), &enterpriseId);

        // Check if the source is enterprise content 
        if (SUCCEEDED(hr) && !(enterpriseId.get() == nullptr || enterpriseId.get()[0] == L'\0'))
        {
            // Get description (format) of data
            hr = EdpGetDataInfoFromWin32Clipboard(_hEdpUtil.get(), &dataDescription);

            // Get AppId of data source
            if (SUCCEEDED(hr))
            {
                hr = EdpGetSourceAppIdForClipboard(_hEdpUtil.get(), &sourceDescription);
            }

            // Audits for all enrolled users
            if (SUCCEEDED(hr))
            {
                hr = EdpAuditAction(_hEdpUtil.get(),
                                    dataDescription.get(),
                                    EDP_AUDIT_ACTION::SystemGenerated,
                                    sourceDescription.get(),
                                    enterpriseId.get(),
                                    DestinationName.c_str(),
                                    nullptr,
                                    nullptr);
            }
        }

        LOG_IF_FAILED(hr);
    }
}

BOOL EdpGetIsManaged(_In_ HMODULE _hEdpUtil)
{
    if (_hEdpUtil != nullptr) {
        typedef BOOL(*PfnEdpGetIsManaged)();

        static bool tried = false;
        static PfnEdpGetIsManaged pfn = nullptr;

        if (!tried)
        {
            pfn = (PfnEdpGetIsManaged)GetProcAddress(_hEdpUtil, "EdpGetIsManaged");
        }

        tried = true;

        if (pfn != nullptr)
        {
            return pfn();
        }
    }

    return FALSE;
}

HRESULT EdpGetEnterpriseIdForClipboard(_In_ HMODULE _hEdpUtil, _Outptr_result_maybenull_ PWSTR* enterpriseId)
{
    if (_hEdpUtil != nullptr) {
        typedef HRESULT(*PfnEdpGetEnterpriseIdForClipboard)(_Outptr_result_maybenull_ PWSTR*);

        static bool tried = false;
        static PfnEdpGetEnterpriseIdForClipboard pfn = nullptr;

        if (!tried)
        {
            pfn = (PfnEdpGetEnterpriseIdForClipboard)GetProcAddress(_hEdpUtil, "EdpGetEnterpriseIdForClipboard");
        }

        tried = true;

        if (pfn != nullptr)
        {
            return pfn(enterpriseId);
        }
    }

    return E_FAIL;
}

HRESULT EdpGetDataInfoFromWin32Clipboard(_In_ HMODULE _hEdpUtil, _Outptr_result_z_ PWSTR* dataInfo)
{
    if (_hEdpUtil != nullptr) {
        typedef HRESULT(*PfnEdpGetDataInfoFromWin32Clipboard)(_Outptr_result_z_ PWSTR*);

        static bool tried = false;
        static PfnEdpGetDataInfoFromWin32Clipboard pfn = nullptr;
        static LPCVOID edpData = nullptr;
        
        if (!tried)
        {
            pfn = (PfnEdpGetDataInfoFromWin32Clipboard)GetProcAddress(_hEdpUtil, "EdpGetDataInfoFromWin32Clipboard");
        }

        tried = true;

        if (pfn != nullptr)
        {
            return pfn(dataInfo);
        }
    }

    return E_FAIL;
}

HRESULT EdpGetSourceAppIdForClipboard(_In_ HMODULE _hEdpUtil, _Outptr_result_maybenull_ PWSTR* SourceAppId)
{
    if (_hEdpUtil != nullptr) {
        typedef HRESULT(*PfnEdpGetSourceAppIdForClipboard)(_Outptr_result_maybenull_ PWSTR*);

        static bool tried = false;
        static PfnEdpGetSourceAppIdForClipboard pfn = nullptr;

        if (!tried)
        {
            pfn = (PfnEdpGetSourceAppIdForClipboard)GetProcAddress(_hEdpUtil, "EdpGetSourceAppIdForClipboard");
        }

        tried = true;
        if (pfn != nullptr)
        {
            return pfn(SourceAppId);
        }
    }

    return E_FAIL;
}

HRESULT EdpAuditAction(_In_ HMODULE _hEdpUtil,
                              _In_ PCWSTR ObjectDescription,
                              _In_ EDP_AUDIT_ACTION Action,
                              _In_opt_ PCWSTR SourceName,
                              _In_opt_ PCWSTR SourceEID,
                              _In_opt_ PCWSTR DestinationName,
                              _In_opt_ PCWSTR DestinationEID,
                              _In_opt_ PCWSTR Application)
{
    if (_hEdpUtil != nullptr) {
        typedef HRESULT(*PfnEdpAuditAction)(_In_ PCWSTR,
                                            _In_ EDP_AUDIT_ACTION,
                                            _In_opt_ PCWSTR,
                                            _In_opt_ PCWSTR,
                                            _In_opt_ PCWSTR,
                                            _In_opt_ PCWSTR,
                                            _In_opt_ PCWSTR);

        static bool tried = false;
        static PfnEdpAuditAction pfn = nullptr;

        if (!tried)
        {
            pfn = (PfnEdpAuditAction)GetProcAddress(_hEdpUtil, "EdpAuditAction");
        }

        tried = true;

        if (pfn != nullptr)
        {
            return pfn(ObjectDescription, Action, SourceName, SourceEID, DestinationName, DestinationEID, Application);
        }
    }
    return E_FAIL;
}

EdpConsolePolicy::EdpConsolePolicy()
{
    _hEdpUtil.reset(LoadLibraryEx(L"edputil.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32));
}