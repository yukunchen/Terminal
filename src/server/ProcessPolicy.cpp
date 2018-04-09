/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "ProcessPolicy.h"

#include "..\host\globals.h"
#include "..\host\telemetry.hpp"

#include <wil\TokenHelpers.h>

// AttributesPresent flags
#define ACCESS_CLAIM_WIN_SYSAPPID_PRESENT    0x00000001  // WIN://SYSAPPID
#define ACCESS_CLAIM_WIN_PKG_PRESENT         0x00000002  // WIN://PKG
#define ACCESS_CLAIM_WIN_SKUID_PRESENT       0x00000004  // WIN://SKUID

#define NT_ASSERT(_exp) assert(_exp)

typedef struct _PS_PKG_CLAIM {
    ULONGLONG Flags : 16;
    ULONGLONG Origin : 8;
} PS_PKG_CLAIM, *PPS_PKG_CLAIM;

extern "C" NTSYSAPI LONG NTAPI RtlQueryPackageClaims(
    _In_ PVOID TokenObject,
    _Out_writes_bytes_to_opt_(*PackageSize, *PackageSize) PWSTR PackageFullName,
    _Inout_opt_ PSIZE_T PackageSize,
    _Out_writes_bytes_to_opt_(*AppIdSize, *AppIdSize) PWSTR AppId,
    _Inout_opt_ PSIZE_T AppIdSize,
    _Out_opt_ LPGUID DynamicId,
    _Out_opt_ PVOID /* PPS_PKG_CLAIM */ PkgClaim,
    _Out_opt_ PULONG64 AttributesPresent
);

#include <appmodelpolicy.h>

// Routine Description:
// - Constructs a new instance of the process policy class.
// Arguments:
// - All arguments specify a true/false status to a policy that could be applied to a console client app.
ConsoleProcessPolicy::ConsoleProcessPolicy(const bool fCanReadOutputBuffer,
                                           const bool fCanWriteInputBuffer) :
    _fCanReadOutputBuffer(fCanReadOutputBuffer),
    _fCanWriteInputBuffer(fCanWriteInputBuffer)
{
}

// Routine Description:
// - Destructs an instance of the process policy class.
ConsoleProcessPolicy::~ConsoleProcessPolicy()
{
}

// Routine Description:
// - Opens the process token for the given handle and resolves the application model policies
//   that apply to the given process handle. This may reveal restrictions on operations that are
//   supposed to be enforced against a given console client application.
// Arguments:
// - hProcess - Handle to a connected process
// Return Value:
// - ConsoleProcessPolicy object containing resolved policy data.
ConsoleProcessPolicy ConsoleProcessPolicy::s_CreateInstance(const HANDLE hProcess)
{
    // If we cannot determine the policy status, then we block access by default.
    bool fCanReadOutputBuffer = false;
    bool fCanWriteInputBuffer = false;

    wil::unique_handle hToken;
    if (LOG_IF_WIN32_BOOL_FALSE(OpenProcessToken(hProcess, TOKEN_READ, &hToken)))
    {
        bool fIsWrongWayBlocked = true;

        // First check AppModel Policy:
        LOG_IF_FAILED(s_CheckAppModelPolicy(hToken.get(), fIsWrongWayBlocked));

        // If we're not restricted by AppModel Policy, also check for Integrity Level below our own.
        if (!fIsWrongWayBlocked)
        {
            LOG_IF_FAILED(s_CheckIntegrityLevelPolicy(hToken.get(), fIsWrongWayBlocked));
        }

        // If we're not blocking wrong way verbs, adjust the read/write policies to permit read out and write in.
        if (!fIsWrongWayBlocked)
        {
            fCanReadOutputBuffer = true;
            fCanWriteInputBuffer = true;
        }
    }

    return ConsoleProcessPolicy(fCanReadOutputBuffer, fCanWriteInputBuffer);
}

[[nodiscard]]
HRESULT ConsoleProcessPolicy::s_CheckAppModelPolicy(const HANDLE hToken,
                                                    _Inout_ bool& fIsWrongWayBlocked)
{
    // If we cannot determine the policy status, then we block access by default.
    fIsWrongWayBlocked = true;

    const AppModelPolicy::ConsoleBufferAccessPolicy bufferAccessPolicy = AppModelPolicy::GetConsoleBufferAccessPolicy(hToken);

    switch (bufferAccessPolicy)
    {
    case AppModelPolicy::ConsoleBufferAccessPolicy::Unrestricted:
        fIsWrongWayBlocked = false;
        break;
    case AppModelPolicy::ConsoleBufferAccessPolicy::RestrictedUnidirectional:
        fIsWrongWayBlocked = true;
        break;
    default:
        RETURN_HR(E_NOTIMPL);
    }

    return S_OK;
}

[[nodiscard]]
HRESULT ConsoleProcessPolicy::s_CheckIntegrityLevelPolicy(const HANDLE hOtherToken,
                                                          _Inout_ bool& fIsWrongWayBlocked)
{
    // If we cannot determine the policy status, then we block access by default.
    fIsWrongWayBlocked = true;

    // This is a magic value, we don't need to free it.
    const HANDLE hMyToken = GetCurrentProcessToken();

    DWORD dwMyIntegrityLevel;
    RETURN_IF_FAILED(s_GetIntegrityLevel(hMyToken, dwMyIntegrityLevel));

    DWORD dwOtherIntegrityLevel;
    RETURN_IF_FAILED(s_GetIntegrityLevel(hOtherToken, dwOtherIntegrityLevel));

    // If the other process is at or above the conhost's integrity, we allow the verbs.
    if (dwOtherIntegrityLevel >= dwMyIntegrityLevel)
    {
        fIsWrongWayBlocked = false;
    }

    return S_OK;
}

[[nodiscard]]
HRESULT ConsoleProcessPolicy::s_GetIntegrityLevel(const HANDLE hToken,
                                                  _Out_ DWORD& dwIntegrityLevel)
{
    dwIntegrityLevel = 0;

    // Get the Integrity level.
    wistd::unique_ptr<TOKEN_MANDATORY_LABEL> tokenLabel;
    RETURN_IF_FAILED(wil::GetTokenInformationNoThrow(tokenLabel, hToken));

    dwIntegrityLevel = *GetSidSubAuthority(tokenLabel->Label.Sid,
        (DWORD)(UCHAR)(*GetSidSubAuthorityCount(tokenLabel->Label.Sid) - 1));

    return S_OK;
}

// Routine Description:
// - Determines whether a console client should be allowed to read back from the output buffers.
// - This includes any of our classic APIs which could allow retrieving data from the output "screen buffer".
// Arguments:
// - <none>
// Return Value:
// - True if read back is allowed. False otherwise.
bool ConsoleProcessPolicy::CanReadOutputBuffer() const
{
    return _fCanReadOutputBuffer;
}

// Routine Description:
// - Determines whether a console client should be allowed to write to the input buffers.
// - This includes any of our classic APIs which could allow inserting data into the input buffer.
// Arguments:
// - <none>
// Return Value:
// - True if writing input is allowed. False otherwise.
bool ConsoleProcessPolicy::CanWriteInputBuffer() const
{
    return _fCanWriteInputBuffer;
}
