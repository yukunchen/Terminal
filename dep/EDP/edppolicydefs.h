// Copyright (C) Microsoft. All rights reserved.
// Bare bones definitions that can be included without header
// conflicts. Currently required for NTFS.

#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

//
// Enumerates the allowed values of EDPEnforcementLevel policy.
//

typedef enum _EDP_ENFORCEMENT_LEVEL
{
    EdpEnforcementLevel_Off = 0,
    EdpEnforcementLevel_Silent,
    EdpEnforcementLevel_Override,
    EdpEnforcementLevel_Block,
    EdpEnforcementLevel_Max
} EDP_ENFORCEMENT_LEVEL;

#ifdef __cplusplus
}
#endif // __cplusplus