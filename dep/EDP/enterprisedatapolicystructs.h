/*++
Copyright (c) Microsoft Corporation. All Rights Reserved.

Module Name:

    policyinterfacestructs.h

Abstract:

    Defines the EDP policy interface structs exposed to other components.

Author:

    Jugal Kaku (jugalk)

Environment:

    User Mode or Kernel Mode

Revision History:

    Created on  11/11/2014.

--*/

#pragma once

#ifndef _POLICY_INTERFACE_STRUCTS_H
#define _POLICY_INTERFACE_STRUCTS_H

#ifndef MIDL_PASS

typedef struct
{
    PSRP_ENTERPRISE_CONTEXT Context;
    PSRP_ENTERPRISE_ID EnterpriseIdForUIEnforcement;
} EDP_ENTERPRISE_CONTEXT, *PEDP_ENTERPRISE_CONTEXT;

typedef struct
{
    PCWSTR SourceAppName;
    PCWSTR TargetAppName;
    PCWSTR DataInfo;
} EDP_AUDIT_DATA;

typedef enum {
    SystemGenerated = 0,
    FileDecrypt,
    CopyToLocation,
    SendToRecipient,
    Other,
    FileRead,
    NetworkAccess,
} EDP_AUDIT_ACTION;

// Allocate these strings with HeapAlloc(GetProcessHeap())
typedef struct
{
    PWSTR sourceDescription;
    PWSTR targetDescription;
    PWSTR dataDescription;
    EDP_AUDIT_ACTION action;

    // These are used to grant access to transferred files for the target app
    ULONG targetProcessId;
    PWSTR targetAppId;
    PWSTR dataFilePaths; // A '|' delimited list of files paths
} AUDIT_CALLBACK_INFO;

typedef enum
{
    EDP_CONTEXT_NONE = 0x0,
    EDP_CONTEXT_IS_EXEMPT = 0x1,
    EDP_CONTEXT_IS_ENLIGHTENED = 0x2,
    EDP_CONTEXT_IS_UNENLIGHTENED_ALLOWED = 0x4,
    EDP_CONTEXT_IS_PERMISSIVE = 0x8,
    EDP_CONTEXT_IS_COPY_EXEMPT = 0x10,
    EDP_CONTEXT_IS_DENIED = 0x20,
} EDP_CONTEXT_STATES;

DEFINE_ENUM_FLAG_OPERATORS(EDP_CONTEXT_STATES);

typedef struct
{
    EDP_CONTEXT_STATES contextStates;
    ULONG allowedEnterpriseIdCount;
    PWSTR enterpriseIdForUIEnforcement;
    PWSTR allowedEnterpriseIds[1]; // Variable sized based on enterpriseIdForUIEnforcement
} EDP_CONTEXT;

typedef enum
{
    EDP_REQUEST_ACCESS_OVERRIDE_NONE = 0,

    // Skip policy evaluation and just show a block dialog
    EDP_REQUEST_ACCESS_OVERRIDE_SHOW_BLOCK_DIALOG,

    // Skip policy evaluation and just show an override dialog
    EDP_REQUEST_ACCESS_OVERRIDE_SHOW_OVERRIDE_DIALOG,

    // If user consent is necessary,
    EDP_REQUEST_ACCESS_OVERRIDE_DISABLE_OVERRIDE_DIALOG,

    // Try get as far as possible without throwing a dialog, including auditing and caching
    EDP_REQUEST_ACCESS_OVERRIDE_NO_DIALOG
} EDP_REQUEST_ACCESS_OVERRIDE;

typedef enum
{
    EDP_TELEMETRY_CALLER_COPYPASTE = 0,
    EDP_TELEMETRY_CALLER_SHARE,
    EDP_TELEMETRY_CALLER_OLEDRAGDROP,
    EDP_TELEMETRY_CALLER_WINRTDRAGDROP,
    EDP_TELEMETRY_CALLER_WINRTSTATIC,
    EDP_TELEMETRY_CALLER_OTHER
} EDP_TELEMETRY_CALLER;

typedef enum
{
    // Default: Use the sourceEnterpriseId parameter.
    EDP_SOURCE_ENTERPRISEID_OPTION_USE_INPUT = 0,

    // Perform an upfront audit callback and then use the retrieved
    // dataFilePaths to fetch the sourceEnterpriseId.
    EDP_SOURCE_ENTERPRISEID_OPTION_EVALUATE_DATA_FILE_PATHS = 1
} EDP_SOURCE_ENTERPRISEID_OPTION;

typedef struct
{
    // Optional process id to use to check if consent has already been given for this process,
    // and to set cache if consent is obtained.
    // Cache will not be used or set if this is 0.
    ULONG processIdForClipboardConsentCache;

    // Optional text to use for the body of the block dialog if needed.
    // Default text will be used if this is nullptr.
    PCWSTR dialogBlockedBodyText;

    // Optional text to use for the body of the override dialog if needed.
    // Default text will be used if this is nullptr.
    PCWSTR dialogOverrideBodyText;

    // Optional override flag. See EDP_REQUEST_ACCESS_OVERRIDE description
    EDP_REQUEST_ACCESS_OVERRIDE overrideOption;

    // Optional flag for EDP telemetry purposes which gives a description of the caller
    EDP_TELEMETRY_CALLER telemetryCaller;

    // Optional enum that specifies how to fetch the sourceEnterpriseId.
    EDP_SOURCE_ENTERPRISEID_OPTION sourceEnterpriseIdOption;
} EDP_REQUEST_ACCESS_OPTIONS;

typedef SRP_ENTERPRISE_ID EDP_ENTERPRISE_ID, *PEDP_ENTERPRISE_ID;

typedef enum _EdpPolicyEnforcementMode
{
    EdpPolicyEnforcementMode_NoPrompt,
    EdpPolicyEnforcementMode_BlockingPrompt,
    EdpPolicyEnforcementMode_AllowOnPrompt,
    EdpPolicyEnforcementMode_MaxValue
} EdpPolicyEnforcementMode;

#endif // MIDL_PASS

typedef enum
{
    EdpPolicyResult_Unknown = 0,
    EdpPolicyResult_Allowed,
    EdpPolicyResult_Blocked,
    EdpPolicyResult_ConsentRequired,
    EdpPolicyResult_ConsentRequired_Add_Encryption,
    EdpPolicyResult_MaxValue
} EDP_POLICY_RESULT;

typedef enum
{
    EdpCaller_Unknown = 0,
    EdpCaller_DragDrop,
    EdpCaller_Share,
    EdpCaller_CopyPaste,
    // EdpCaller_CopyDoublePaste is still a copy paste, but is used in a couple
    // special cases where we need to distinguish from above.
    // Desktop: The paste can't be done in the same flow as the dialog, and on override,
    //          the user will have to paste again (with no prompt the second time)
    // Phone: When pasting via SIP paste, blocking on the dialog causes a deadlock,
    //        so on override, the VK_PASTE is sent manually
    EdpCaller_CopyDoublePaste,
    EdpCaller_MaxValue
} EDP_CALLER;

#endif // _POLICY_INTERFACE_STRUCTS_H
