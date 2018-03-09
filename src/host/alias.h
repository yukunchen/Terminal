/*++
Copyright (c) Microsoft Corporation

Module Name:
- alias.h

Abstract:
- Encapsulates the cmdline functions and structures specifically related to
        command alias functionality.
--*/
#pragma once

//
// aliases are grouped per console, per exe.
//

typedef struct _ALIAS
{
    LIST_ENTRY ListLink;
    USHORT SourceLength;
    USHORT TargetLength;
    _Field_size_bytes_opt_(SourceLength) PWCHAR Source;
    _Field_size_bytes_(TargetLength) PWCHAR Target;
} ALIAS, *PALIAS;

typedef struct _EXE_ALIAS_LIST
{
    LIST_ENTRY ListLink;
    USHORT ExeLength;
    _Field_size_bytes_opt_(ExeLength) PWCHAR ExeName;
    LIST_ENTRY AliasList;
} EXE_ALIAS_LIST, *PEXE_ALIAS_LIST;

[[nodiscard]]
NTSTATUS MatchAndCopyAlias(_In_reads_bytes_(cbSource) PWCHAR pwchSource,
                           _In_ USHORT cbSource,
                           _Out_writes_bytes_(*pcbTarget) PWCHAR pwchTarget,
                           _Inout_ PUSHORT pcbTarget,
                           _In_reads_bytes_(cbExe) PWCHAR pwchExe,
                           _In_ USHORT cbExe,
                           _Out_ PDWORD pcLines);

void ClearCmdExeAliases();
void FreeAliasBuffers();
