//
//    Copyright (C) Microsoft.  All rights reserved.
//
#pragma once
#include "Resource.h"
#include <new>
#include <lmcons.h>         // for UNLEN and DNLEN

// for GetUserNameEx()
#define SECURITY_WIN32
#include <Security.h>

namespace wil
{
    /// @cond
    namespace details
    {
        // Template specialization for TOKEN_INFORMATION_CLASS, add more mappings here as needed
        // TODO: The mapping should be reversed to be MapTokenInfoClassToStruct since there may
        // be an info class value that uses the same structure. That is the case for the file
        // system information.
        template <typename T> struct MapTokenStructToInfoClass;
        template <> struct MapTokenStructToInfoClass<TOKEN_USER> { static const TOKEN_INFORMATION_CLASS infoClass = TokenUser; };
        template <> struct MapTokenStructToInfoClass<TOKEN_PRIVILEGES> { static const TOKEN_INFORMATION_CLASS infoClass = TokenPrivileges; };
        template <> struct MapTokenStructToInfoClass<TOKEN_OWNER> { static const TOKEN_INFORMATION_CLASS infoClass = TokenOwner; };
        template <> struct MapTokenStructToInfoClass<TOKEN_PRIMARY_GROUP> { static const TOKEN_INFORMATION_CLASS infoClass = TokenPrimaryGroup; };
        template <> struct MapTokenStructToInfoClass<TOKEN_DEFAULT_DACL> { static const TOKEN_INFORMATION_CLASS infoClass = TokenDefaultDacl; };
        template <> struct MapTokenStructToInfoClass<TOKEN_SOURCE> { static const TOKEN_INFORMATION_CLASS infoClass = TokenSource; };
        template <> struct MapTokenStructToInfoClass<TOKEN_TYPE> { static const TOKEN_INFORMATION_CLASS infoClass = TokenType; };
        template <> struct MapTokenStructToInfoClass<SECURITY_IMPERSONATION_LEVEL> { static const TOKEN_INFORMATION_CLASS infoClass = TokenImpersonationLevel; };
        template <> struct MapTokenStructToInfoClass<TOKEN_STATISTICS> { static const TOKEN_INFORMATION_CLASS infoClass = TokenStatistics; };
        template <> struct MapTokenStructToInfoClass<TOKEN_GROUPS_AND_PRIVILEGES> { static const TOKEN_INFORMATION_CLASS infoClass = TokenGroupsAndPrivileges; };
        template <> struct MapTokenStructToInfoClass<TOKEN_ORIGIN> { static const TOKEN_INFORMATION_CLASS infoClass = TokenOrigin; };
        template <> struct MapTokenStructToInfoClass<TOKEN_ELEVATION_TYPE> { static const TOKEN_INFORMATION_CLASS infoClass = TokenElevationType; };
        template <> struct MapTokenStructToInfoClass<TOKEN_LINKED_TOKEN> { static const TOKEN_INFORMATION_CLASS infoClass = TokenLinkedToken; };
        template <> struct MapTokenStructToInfoClass<TOKEN_ELEVATION> { static const TOKEN_INFORMATION_CLASS infoClass = TokenElevation; };
        template <> struct MapTokenStructToInfoClass<TOKEN_ACCESS_INFORMATION> { static const TOKEN_INFORMATION_CLASS infoClass = TokenAccessInformation; };
        template <> struct MapTokenStructToInfoClass<TOKEN_MANDATORY_LABEL> { static const TOKEN_INFORMATION_CLASS infoClass = TokenIntegrityLevel; };
        template <> struct MapTokenStructToInfoClass<TOKEN_MANDATORY_POLICY> { static const TOKEN_INFORMATION_CLASS infoClass = TokenMandatoryPolicy; };
        template <> struct MapTokenStructToInfoClass<TOKEN_APPCONTAINER_INFORMATION> { static const TOKEN_INFORMATION_CLASS infoClass = TokenAppContainerSid; };
    }
    /// @endcond

    //! Current thread or process token, consider using GetCurrentThreadEffectiveToken() instead.
    inline HRESULT OpenCurrentAccessTokenNoThrow(_Out_ HANDLE* tokenHandle)
    {
        HRESULT hr = (OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, FALSE, tokenHandle) ? S_OK : HRESULT_FROM_WIN32(::GetLastError()));
        if (hr == HRESULT_FROM_WIN32(ERROR_NO_TOKEN))
        {
            hr = (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, tokenHandle) ? S_OK : HRESULT_FROM_WIN32(::GetLastError()));
        }
        return hr;
    }

    //! Current thread or process token, consider using GetCurrentThreadEffectiveToken() instead.
    inline wil::unique_handle OpenCurrentAccessTokenFailFast()
    {
        HANDLE rawTokenHandle;
        FAIL_FAST_IF_FAILED(OpenCurrentAccessTokenNoThrow(&rawTokenHandle));
        return wil::unique_handle(rawTokenHandle);
    }

// Exception based function to open current thread/process access token and acquire pointer to it
#ifdef WIL_ENABLE_EXCEPTIONS
    //! Current thread or process token, consider using GetCurrentThreadEffectiveToken() instead.
    inline wil::unique_handle OpenCurrentAccessToken()
    {
        HANDLE rawTokenHandle;
        THROW_IF_FAILED(OpenCurrentAccessTokenNoThrow(&rawTokenHandle));
        return wil::unique_handle(rawTokenHandle);
    }
#endif // WIL_ENABLE_EXCEPTIONS

    //! Bind TOKEN_INFORMATION_CLASS values to wil::unique_ptr<T> from given token or fetches from current thread/process token.
    template <typename T>
    inline HRESULT GetTokenInformationNoThrow(_Inout_ wistd::unique_ptr<T>& tokenInfo, _In_opt_ HANDLE tokenHandle)
    {
        tokenInfo.reset();

        // get the tokenHandle from current thread/process if it's null
        if (tokenHandle == nullptr)
        {
            tokenHandle = GetCurrentThreadEffectiveToken(); // Pseudo token, don't free.
        }

        const auto infoClass = details::MapTokenStructToInfoClass<T>::infoClass;
        DWORD tokenInfoSize = 0;
        RETURN_LAST_ERROR_IF_FALSE((!GetTokenInformation(tokenHandle, infoClass, nullptr, 0, &tokenInfoSize)) &&
            (::GetLastError() == ERROR_INSUFFICIENT_BUFFER));

        wistd::unique_ptr<char> tokenInfoClose(
            static_cast<char*>(operator new(tokenInfoSize, std::nothrow)));
        RETURN_IF_NULL_ALLOC(tokenInfoClose.get());
        RETURN_IF_WIN32_BOOL_FALSE(GetTokenInformation(tokenHandle, infoClass, tokenInfoClose.get(), tokenInfoSize, &tokenInfoSize));
        tokenInfo.reset(reinterpret_cast<T *>(tokenInfoClose.release()));
        
        return S_OK;
    }

    //! Bind TOKEN_INFORMATION_CLASS values to wil::unique_ptr<T> from given token or fetches from current thread/process token.
    template <typename T>
    inline wistd::unique_ptr<T> GetTokenInformationFailFast(_In_ HANDLE token = nullptr)
    {
        wistd::unique_ptr<T> tokenInfo;
        FAIL_FAST_IF_FAILED(GetTokenInformationNoThrow(tokenInfo, token));
        return tokenInfo;
    }

#ifdef WIL_ENABLE_EXCEPTIONS
    //! Bind TOKEN_INFORMATION_CLASS values to wil::unique_ptr<T> from given token or fetches from current thread/process token.
    template <typename T>
    inline wistd::unique_ptr<T> GetTokenInformation(_In_ HANDLE token = nullptr)
    {
        wistd::unique_ptr<T> tokenInfo;
        THROW_IF_FAILED(GetTokenInformationNoThrow(tokenInfo, token));
        return tokenInfo;
    }
#endif

    // http://osgvsowi/658484 To track DSMA(Default System Managed Account) workitem, below implementation will be updated once above workitem is implemented
    // To check if DSMA or not
    inline bool IsDefaultSystemManagedAccount(_In_opt_ PCWSTR userName = nullptr)
    {
        // This is temporary DefaultAccount Scenario Code
        // User OOBE reg keys/values
        const PCWSTR regValueDefaultAccountName = L"DefaultAccountSAMName";
        const PCWSTR regKeyOOBE = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\OOBE";
        wchar_t defaultAccountName[UNLEN + 1];
        DWORD defaultAccountNameByteSize = sizeof(defaultAccountName);
        wchar_t currentUserName[DNLEN + UNLEN + 2];     // +2 for "\" delimeter and terminating NULL

        // Get SAM name for current user if no user name specified.
        if (userName == nullptr)
        {
            DWORD currentUserNameSize = ARRAYSIZE(currentUserName);
            if (GetUserNameExW(NameSamCompatible, currentUserName, &currentUserNameSize))
            {
                userName = wcschr(currentUserName, L'\\') + 1;
            }
        }

        return SUCCEEDED(HRESULT_FROM_WIN32(RegGetValueW(HKEY_LOCAL_MACHINE, regKeyOOBE, regValueDefaultAccountName, RRF_RT_REG_SZ, nullptr, defaultAccountName, &defaultAccountNameByteSize))) &&
            (userName != nullptr) &&
            (CompareStringOrdinal(userName, -1, defaultAccountName, -1, TRUE) == CSTR_EQUAL);
    }
} //namespace wil
