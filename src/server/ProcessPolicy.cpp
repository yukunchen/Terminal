/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "ProcessPolicy.h"

#include "..\host\globals.h"
#include "..\host\telemetry.hpp"

// AttributesPresent flags
#define ACCESS_CLAIM_WIN_SYSAPPID_PRESENT    0x00000001  // WIN://SYSAPPID
#define ACCESS_CLAIM_WIN_PKG_PRESENT         0x00000002  // WIN://PKG
#define ACCESS_CLAIM_WIN_SKUID_PRESENT       0x00000004  // WIN://SKUID

#define NT_ASSERT(_exp) assert(_exp)

typedef struct _PS_PKG_CLAIM {
   ULONGLONG Flags:16;
   ULONGLONG Origin:8;
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
ConsoleProcessPolicy::ConsoleProcessPolicy(_In_ const bool fCanReadOutputBuffer, 
                                           _In_ const bool fCanWriteInputBuffer) :
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
ConsoleProcessPolicy ConsoleProcessPolicy::s_CreateInstance(_In_ const HANDLE hProcess)
{
    bool fCanReadOutputBuffer = false;
    bool fCanWriteInputBuffer = false;

    wil::unique_handle hToken;
    if (LOG_IF_WIN32_BOOL_FALSE(OpenProcessToken(hProcess, TOKEN_READ, &hToken)))
    {
        const AppModelPolicy::ConsoleBufferAccessPolicy bufferAccessPolicy = AppModelPolicy::GetConsoleBufferAccessPolicy(hToken.get());

        switch (bufferAccessPolicy)
        {
        case AppModelPolicy::ConsoleBufferAccessPolicy::Unrestricted:
            fCanReadOutputBuffer = true;
            fCanWriteInputBuffer = true;
            break;
        case AppModelPolicy::ConsoleBufferAccessPolicy::RestrictedUnidirectional:
            fCanReadOutputBuffer = false;
            fCanWriteInputBuffer = false;
            break;
        default:
            THROW_HR(E_NOTIMPL);
        }
    }

    return ConsoleProcessPolicy(fCanReadOutputBuffer, fCanWriteInputBuffer);
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
