/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#include "precomp.h"

#include "cmdline.h"

#include "_output.h"
#include "output.h"
#include "stream.h"
#include "_stream.h"
#include "cursor.h"
#include "dbcs.h"
#include "directio.h"
#include "handle.h"
#include "menu.h"
#include "misc.h"
#include "srvinit.h"
#include "utils.hpp"
#include "window.hpp"

#pragma hdrstop

#define COPY_TO_CHAR_PROMPT_LENGTH 26
#define COPY_FROM_CHAR_PROMPT_LENGTH 28

#define COMMAND_NUMBER_PROMPT_LENGTH 22
#define COMMAND_NUMBER_LENGTH 5
#define MINIMUM_COMMAND_PROMPT_SIZE COMMAND_NUMBER_LENGTH

#define ALT_PRESSED     (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED)
#define CTRL_PRESSED    (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED)

#define CTRL_BUT_NOT_ALT(n) \
        (((n) & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) && \
        !((n) & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)))

#define COMMAND_NUM_TO_INDEX(NUM, CMDHIST) (SHORT)(((NUM+(CMDHIST)->FirstCommand)%((CMDHIST)->MaximumNumberOfCommands)))
#define COMMAND_INDEX_TO_NUM(INDEX, CMDHIST) (SHORT)(((INDEX+((CMDHIST)->MaximumNumberOfCommands)-(CMDHIST)->FirstCommand)%((CMDHIST)->MaximumNumberOfCommands)))

#define POPUP_SIZE_X(POPUP) (SHORT)(((POPUP)->Region.Right - (POPUP)->Region.Left - 1))
#define POPUP_SIZE_Y(POPUP) (SHORT)(((POPUP)->Region.Bottom - (POPUP)->Region.Top - 1))
#define COMMAND_NUMBER_SIZE 8   // size of command number buffer


// COMMAND_IND_NEXT and COMMAND_IND_PREV go to the next and prev command
// COMMAND_IND_INC  and COMMAND_IND_DEC  go to the next and prev slots
//
// Don't get the two confused - it matters when the cmd history is not full!
#define COMMAND_IND_PREV(IND, CMDHIST)               \
{                                                    \
    if (IND <= 0) {                                  \
        IND = (CMDHIST)->NumberOfCommands;           \
    }                                                \
    IND--;                                           \
}

#define COMMAND_IND_NEXT(IND, CMDHIST)               \
{                                                    \
    ++IND;                                           \
    if (IND >= (CMDHIST)->NumberOfCommands) {        \
        IND = 0;                                     \
    }                                                \
}

#define COMMAND_IND_DEC(IND, CMDHIST)                \
{                                                    \
    if (IND <= 0) {                                  \
        IND = (CMDHIST)->MaximumNumberOfCommands;    \
    }                                                \
    IND--;                                           \
}

#define COMMAND_IND_INC(IND, CMDHIST)                \
{                                                    \
    ++IND;                                           \
    if (IND >= (CMDHIST)->MaximumNumberOfCommands) { \
        IND = 0;                                     \
    }                                                \
}

#define FMCFL_EXACT_MATCH   1
#define FMCFL_JUST_LOOKING  2

#define UCLP_WRAP   1

// fwd decls
void EmptyCommandHistory(_In_opt_ PCOMMAND_HISTORY CommandHistory);
PCOMMAND_HISTORY ReallocCommandHistory(_In_opt_ PCOMMAND_HISTORY CurrentCommandHistory, _In_ DWORD const NumCommands);
PCOMMAND_HISTORY FindExeCommandHistory(_In_reads_(AppNameLength) PVOID AppName, _In_ DWORD AppNameLength, _In_ BOOLEAN const UnicodeExe);
void DrawCommandListBorder(_In_ PCLE_POPUP const Popup, _In_ PSCREEN_INFORMATION const ScreenInfo);
PCOMMAND GetLastCommand(_In_ PCOMMAND_HISTORY CommandHistory);
SHORT FindMatchingCommand(_In_ PCOMMAND_HISTORY CommandHistory,
                          _In_reads_bytes_(CurrentCommandLength) PCWCHAR CurrentCommand,
                          _In_ ULONG CurrentCommandLength,
                          _In_ SHORT CurrentIndex,
                          _In_ DWORD Flags);
NTSTATUS CommandNumberPopup(_In_ PCOOKED_READ_DATA const CookedReadData, _In_ PCONSOLE_API_MSG WaitReplyMessage, _In_ BOOLEAN WaitRoutine);
void DrawCommandListPopup(_In_ PCLE_POPUP const Popup,
                          _In_ SHORT const CurrentCommand,
                          _In_ PCOMMAND_HISTORY const CommandHistory,
                          _In_ PSCREEN_INFORMATION const ScreenInfo);
void UpdateCommandListPopup(_In_ SHORT Delta,
                            _Inout_ PSHORT CurrentCommand,
                            _In_ PCOMMAND_HISTORY const CommandHistory,
                            _In_ PCLE_POPUP Popup,
                            _In_ PSCREEN_INFORMATION const ScreenInfo,
                            _In_ DWORD const Flags);
NTSTATUS RetrieveCommand(_In_ PCOMMAND_HISTORY CommandHistory,
                         _In_ WORD VirtualKeyCode,
                         _In_reads_bytes_(BufferSize) PWCHAR Buffer,
                         _In_ ULONG BufferSize,
                         _Out_ PULONG CommandSize);
UINT LoadStringEx(_In_ HINSTANCE hModule, _In_ UINT wID, _Out_writes_(cchBufferMax) LPWSTR lpBuffer, _In_ UINT cchBufferMax, _In_ WORD wLangId);

// Extended Edit Key
ExtKeyDefTable gaKeyDef;
CONST ExtKeyDefTable gaDefaultKeyDef = {
    {   // A
     0, VK_HOME, 0, // Ctrl
     LEFT_CTRL_PRESSED, VK_HOME, 0, // Alt
     0, 0, 0,   // Ctrl+Alt
     }
    ,
    {   // B
     0, VK_LEFT, 0, // Ctrl
     LEFT_CTRL_PRESSED, VK_LEFT, 0, // Alt
     }
    ,
    {   // C
     0,
     }
    ,
    {   // D
     0, VK_DELETE, 0,   // Ctrl
     LEFT_CTRL_PRESSED, VK_DELETE, 0,   // Alt
     0, 0, 0,   // Ctrl+Alt
     }
    ,
    {   // E
     0, VK_END, 0,  // Ctrl
     LEFT_CTRL_PRESSED, VK_END, 0,  // Alt
     0, 0, 0,   // Ctrl+Alt
     }
    ,
    {   // F
     0, VK_RIGHT, 0,    // Ctrl
     LEFT_CTRL_PRESSED, VK_RIGHT, 0,    // Alt
     0, 0, 0,   // Ctrl+Alt
     }
    ,
    {   // G
     0,
     }
    ,
    {   // H
     0,
     }
    ,
    {   // I
     0,
     }
    ,
    {   // J
     0,
     }
    ,
    {   // K
     LEFT_CTRL_PRESSED, VK_END, 0,  // Ctrl
     }
    ,
    {   // L
     0,
     }
    ,
    {   // M
     0,
     }
    ,
    {   // N
     0, VK_DOWN, 0, // Ctrl
     }
    ,
    {   // O
     0,
     }
    ,
    {   // P
     0, VK_UP, 0,   // Ctrl
     }
    ,
    {   // Q
     0,
     }
    ,
    {   // R
     0, VK_F8, 0,   // Ctrl
     }
    ,
    {   // S
     0, VK_PAUSE, 0,    // Ctrl
     }
    ,
    {   // T
     LEFT_CTRL_PRESSED, VK_DELETE, 0,   // Ctrl
     }
    ,
    {   // U
     0, VK_ESCAPE, 0,   // Ctrl
     }
    ,
    {   // V
     0,
     }
    ,
    {   // W
     LEFT_CTRL_PRESSED, VK_BACK, EXTKEY_ERASE_PREV_WORD,    // Ctrl
     }
    ,
    {   // X
     0,
     }
    ,
    {   // Y
     0,
     }
    ,
    {   // Z
     0,
     }
    ,
};

// Routine Description:
// - This routine validates a string buffer and returns the pointers of where the strings start within the buffer.
// Arguments:
// - Unicode - Supplies a boolean that is TRUE if the buffer contains Unicode strings, FALSE otherwise.
// - Buffer - Supplies the buffer to be validated.
// - Size - Supplies the size, in bytes, of the buffer to be validated.
// - Count - Supplies the expected number of strings in the buffer.
// ... - Supplies a pair of arguments per expected string. The first one is the expected size, in bytes, of the string
//       and the second one receives a pointer to where the string starts.
// Return Value:
// - TRUE if the buffer is valid, FALSE otherwise.
BOOLEAN IsValidStringBuffer(_In_ BOOLEAN Unicode, _In_reads_bytes_(Size) PVOID Buffer, _In_ ULONG Size, _In_ ULONG Count, ...)
{
    va_list Marker;
    va_start(Marker, Count);

    while (Count > 0)
    {
        ULONG const StringSize = va_arg(Marker, ULONG);
        PVOID* StringStart = va_arg(Marker, PVOID *);

        // Make sure the string fits in the supplied buffer and that it is properly aligned.
        if (StringSize > Size)
        {
            break;
        }

        if ((Unicode != FALSE) && ((StringSize % sizeof(WCHAR)) != 0))
        {
            break;
        }

        *StringStart = Buffer;

        // Go to the next string.
        Buffer = RtlOffsetToPointer(Buffer, StringSize);
        Size -= StringSize;
        Count -= 1;
    }

    va_end(Marker);

    return Count == 0;
}

// Routine Description:
// - Initialize the extended edit key table.
// - If pKeyDefbuf is nullptr, the internal default table is used.
// - Otherwise, lpbyte should point to a valid ExtKeyDefBuf.
void InitExtendedEditKeys(_In_opt_ ExtKeyDefBuf const * const pKeyDefBuf)
{
    // Sanity check:
    // If pKeyDefBuf is nullptr, give it the default value.
    // If the version is not supported, just use the default and bail.
    if (pKeyDefBuf == nullptr || pKeyDefBuf->dwVersion != 0)
    {
        if (pKeyDefBuf != nullptr)
        {
            RIPMSG1(RIP_WARNING, "InitExtendedEditKeys: Unsupported version number(%d)", pKeyDefBuf->dwVersion);
        }

retry_clean:
        memmove(gaKeyDef, gaDefaultKeyDef, sizeof gaKeyDef);
        return;
    }

    // Calculate check sum
    DWORD dwCheckSum = 0;
    BYTE* const lpbyte = (BYTE*)pKeyDefBuf;
    for (int i = FIELD_OFFSET(ExtKeyDefBuf, table); i < sizeof *pKeyDefBuf; ++i)
    {
        dwCheckSum += lpbyte[i];
    }

    if (dwCheckSum != pKeyDefBuf->dwCheckSum)
    {
        goto retry_clean;
    }

    // Copy the entity
    memmove(gaKeyDef, pKeyDefBuf->table, sizeof gaKeyDef);
}

const ExtKeySubst *ParseEditKeyInfo(_Inout_ PKEY_EVENT_RECORD const pKeyEvent)
{
    // If not extended mode, or Control key or Alt key is not pressed, or virtual keycode is out of range, just bail.
    if (!g_ciConsoleInformation.GetExtendedEditKey() ||
        (pKeyEvent->dwControlKeyState & (CTRL_PRESSED | ALT_PRESSED)) == 0 ||
        pKeyEvent->wVirtualKeyCode < 'A' || pKeyEvent->wVirtualKeyCode > 'Z')
    {
        return nullptr;
    }

    // Get the corresponding KeyDef.
    const ExtKeyDef* const pKeyDef = &gaKeyDef[pKeyEvent->wVirtualKeyCode - 'A'];

    const ExtKeySubst *pKeySubst;

    // Get the KeySubst based on the modifier status.
    if (pKeyEvent->dwControlKeyState & ALT_PRESSED)
    {
        if (pKeyEvent->dwControlKeyState & CTRL_PRESSED)
        {
            pKeySubst = &pKeyDef->keys[2];
        }
        else
        {
            pKeySubst = &pKeyDef->keys[1];
        }
    }
    else
    {
        ASSERT(pKeyEvent->dwControlKeyState & CTRL_PRESSED);
        pKeySubst = &pKeyDef->keys[0];
    }

    // If the conbination is not defined, just bail.
    if (pKeySubst->wVirKey == 0)
    {
        return nullptr;
    }

    // Substitute the input with ext key.
    pKeyEvent->dwControlKeyState = pKeySubst->wMod;
    pKeyEvent->wVirtualKeyCode = pKeySubst->wVirKey;
    pKeyEvent->uChar.UnicodeChar = pKeySubst->wUnicodeChar;

    return pKeySubst;
}

// Routine Description:
// - Returns TRUE if pKeyEvent is pause.
// - The default key is Ctrl-S if extended edit keys are not specified.
bool IsPauseKey(_In_ PKEY_EVENT_RECORD const pKeyEvent)
{
    bool fIsPauseKey = false;
    if (g_ciConsoleInformation.GetExtendedEditKey())
    {
        KEY_EVENT_RECORD KeyEvent = *pKeyEvent;
        CONST ExtKeySubst *pKeySubst = ParseEditKeyInfo(&KeyEvent);

        fIsPauseKey = (pKeySubst != nullptr && pKeySubst->wVirKey == VK_PAUSE);
    }

    fIsPauseKey = fIsPauseKey || (pKeyEvent->wVirtualKeyCode == L'S' && CTRL_BUT_NOT_ALT(pKeyEvent->dwControlKeyState));
    return fIsPauseKey;
}

// Routine Description:
// - Detects Word delimiters
bool IsWordDelim(_In_ WCHAR const wch)
{
    // Before it reaches here, L' ' case should have beeen already detected, and gaWordDelimChars is specified.
    ASSERT(wch != L' ' && gaWordDelimChars[0]);

    for (int i = 0; i < WORD_DELIM_MAX && gaWordDelimChars[i]; ++i)
    {
        if (wch == gaWordDelimChars[i])
        {
            return true;
        }
    }

    return false;
}

PEXE_ALIAS_LIST AddExeAliasList(_In_ LPVOID ExeName,
                                _In_ USHORT ExeLength, // in bytes
                                _In_ BOOLEAN UnicodeExe)
{
    PEXE_ALIAS_LIST AliasList = new EXE_ALIAS_LIST();
    if (AliasList == nullptr)
    {
        return nullptr;
    }

    if (UnicodeExe)
    {
        // Length is in bytes. Add 1 so dividing by WCHAR (2) is always rounding up.
        AliasList->ExeName = new WCHAR[(ExeLength + 1) / sizeof(WCHAR)];
        if (AliasList->ExeName == nullptr)
        {
            delete AliasList;
            return nullptr;
        }
        memmove(AliasList->ExeName, ExeName, ExeLength);
        AliasList->ExeLength = ExeLength;
    }
    else
    {
        AliasList->ExeName = new WCHAR[ExeLength];
        if (AliasList->ExeName == nullptr)
        {
            delete AliasList;
            return nullptr;
        }
        AliasList->ExeLength = (USHORT) ConvertInputToUnicode(g_ciConsoleInformation.CP, (LPSTR) ExeName, ExeLength, AliasList->ExeName, ExeLength);
        AliasList->ExeLength *= 2;
    }
    InitializeListHead(&AliasList->AliasList);
    InsertHeadList(&g_ciConsoleInformation.ExeAliasList, &AliasList->ListLink);
    return AliasList;
}

// Routine Description:
// - This routine searches for the specified exe alias list.  It returns a pointer to the exe list if found, nullptr if not found.
PEXE_ALIAS_LIST FindExe(_In_ LPVOID ExeName,
                        _In_ USHORT ExeLength, // in bytes
                        _In_ BOOLEAN UnicodeExe)
{
    LPWSTR UnicodeExeName;
    if (UnicodeExe)
    {
        UnicodeExeName = (PWSTR) ExeName;
    }
    else
    {
        UnicodeExeName = new WCHAR[ExeLength];
        if (UnicodeExeName == nullptr)
            return nullptr;
        ExeLength = (USHORT) ConvertInputToUnicode(g_ciConsoleInformation.CP, (LPSTR) ExeName, ExeLength, UnicodeExeName, ExeLength);
        ExeLength *= 2;
    }
    PLIST_ENTRY const ListHead = &g_ciConsoleInformation.ExeAliasList;
    PLIST_ENTRY ListNext = ListHead->Flink;
    while (ListNext != ListHead)
    {
        PEXE_ALIAS_LIST const AliasList = CONTAINING_RECORD(ListNext, EXE_ALIAS_LIST, ListLink);
        // Length is in bytes. Add 1 so dividing by WCHAR (2) is always rounding up.
        if (AliasList->ExeLength == ExeLength && !_wcsnicmp(AliasList->ExeName, UnicodeExeName, (ExeLength + 1) / sizeof(WCHAR)))
        {
            if (!UnicodeExe)
            {
                delete[] UnicodeExeName;
            }
            return AliasList;
        }
        ListNext = ListNext->Flink;
    }
    if (!UnicodeExe)
    {
        delete[] UnicodeExeName;
    }
    return nullptr;
}


// Routine Description:
// - This routine searches for the specified alias.  If it finds one,
// - it moves it to the head of the list and returns a pointer to the
// - alias. Otherwise it returns nullptr.
PALIAS FindAlias(_In_ PEXE_ALIAS_LIST AliasList, _In_reads_bytes_(AliasLength) const WCHAR *AliasName, _In_ USHORT AliasLength)
{
    PLIST_ENTRY const ListHead = &AliasList->AliasList;
    PLIST_ENTRY ListNext = ListHead->Flink;
    while (ListNext != ListHead)
    {
        PALIAS const Alias = CONTAINING_RECORD(ListNext, ALIAS, ListLink);
        // Length is in bytes. Add 1 so dividing by WCHAR (2) is always rounding up.
        if (Alias->SourceLength == AliasLength && !_wcsnicmp(Alias->Source, AliasName, (AliasLength + 1) / sizeof(WCHAR)))
        {
            if (ListNext != ListHead->Flink)
            {
                RemoveEntryList(ListNext);
                InsertHeadList(ListHead, ListNext);
            }
            return Alias;
        }
        ListNext = ListNext->Flink;
    }

    return nullptr;
}

// Routine Description:
// - This routine creates an alias and inserts it into the exe alias list.
NTSTATUS AddAlias(_In_ PEXE_ALIAS_LIST ExeAliasList,
                  _In_reads_bytes_(SourceLength) const WCHAR *Source,
                  _In_ USHORT SourceLength,
                  _In_reads_bytes_(TargetLength) const WCHAR *Target,
                  _In_ USHORT TargetLength)
{
    PALIAS const Alias = new ALIAS();
    if (Alias == nullptr)
    {
        return STATUS_NO_MEMORY;
    }

    // Length is in bytes. Add 1 so dividing by WCHAR (2) is always rounding up.
    Alias->Source = new WCHAR[(SourceLength + 1) / sizeof(WCHAR)];
    if (Alias->Source == nullptr)
    {
        delete Alias;
        return STATUS_NO_MEMORY;
    }

    // Length is in bytes. Add 1 so dividing by WCHAR (2) is always rounding up.
    Alias->Target = new WCHAR[(TargetLength + 1) / sizeof(WCHAR)];
    if (Alias->Target == nullptr)
    {
        delete[] Alias->Source;
        delete Alias;
        return STATUS_NO_MEMORY;
    }

    Alias->SourceLength = SourceLength;
    Alias->TargetLength = TargetLength;
    memmove(Alias->Source, Source, SourceLength);
    memmove(Alias->Target, Target, TargetLength);
    InsertHeadList(&ExeAliasList->AliasList, &Alias->ListLink);
    return STATUS_SUCCESS;
}


// Routine Description:
// - This routine replaces an existing target with a new target.
NTSTATUS ReplaceAlias(_In_ PALIAS Alias, _In_reads_bytes_(TargetLength) const WCHAR *Target, _In_ USHORT TargetLength)
{
    // Length is in bytes. Add 1 so dividing by WCHAR (2) is always rounding up.
    WCHAR* const NewTarget = new WCHAR[(TargetLength + 1) / sizeof(WCHAR)];
    if (NewTarget == nullptr)
    {
        return STATUS_NO_MEMORY;
    }

    delete[] Alias->Target;
    Alias->Target = NewTarget;
    Alias->TargetLength = TargetLength;
    memmove(Alias->Target, Target, TargetLength);

    return STATUS_SUCCESS;
}


// Routine Description:
// - This routine removes an alias.
NTSTATUS RemoveAlias(_In_ PALIAS Alias)
{
    RemoveEntryList(&Alias->ListLink);
    delete[] Alias->Source;
    delete[] Alias->Target;
    delete Alias;
    return STATUS_SUCCESS;
}

void FreeAliasList(_In_ PEXE_ALIAS_LIST ExeAliasList)
{
    PLIST_ENTRY const ListHead = &ExeAliasList->AliasList;
    PLIST_ENTRY ListNext = ListHead->Flink;
    while (ListNext != ListHead)
    {
        PALIAS const Alias = CONTAINING_RECORD(ListNext, ALIAS, ListLink);
        ListNext = ListNext->Flink;
        RemoveAlias(Alias);
    }
    RemoveEntryList(&ExeAliasList->ListLink);
    delete[] ExeAliasList->ExeName;
    delete ExeAliasList;
}

void FreeAliasBuffers()
{
    PLIST_ENTRY const ListHead = &g_ciConsoleInformation.ExeAliasList;
    PLIST_ENTRY ListNext = ListHead->Flink;
    while (ListNext != ListHead)
    {
        PEXE_ALIAS_LIST const AliasList = CONTAINING_RECORD(ListNext, EXE_ALIAS_LIST, ListLink);
        ListNext = ListNext->Flink;
        FreeAliasList(AliasList);
    }
}

// Routine Description:
// - This routine adds a command line alias to the global set.
// Arguments:
// - m - message containing api parameters
// - ReplyStatus - Indicates whether to reply to the dll port.
// Return Value:
NTSTATUS SrvAddConsoleAlias(_Inout_ PCONSOLE_API_MSG m, _In_opt_ PBOOL const /*ReplyPending*/)
{
    PCONSOLE_ADDALIAS_MSG const a = &m->u.consoleMsgL3.AddConsoleAliasW;

    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::AddConsoleAlias, a->Unicode);

    // Read the input buffer and validate the strings.
    PVOID Buffer;
    ULONG BufferSize;
    NTSTATUS Status = NTSTATUS_FROM_HRESULT(m->GetInputBuffer(&Buffer, &BufferSize));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    PVOID InputTarget = nullptr;
    PVOID InputExeName;
    PVOID InputSource = nullptr;

    if (IsValidStringBuffer(a->Unicode,
                            Buffer,
                            BufferSize,
                            3,
                            (ULONG) a->ExeLength,
                            &InputExeName,
                            (ULONG) a->SourceLength,
                            &InputSource,
                            (ULONG) a->TargetLength,
                            &InputTarget) == FALSE)
    {
        return STATUS_INVALID_PARAMETER;
    }

    CONSOLE_INFORMATION *Console;
    Status = RevalidateConsole(&Console);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    if (a->SourceLength == 0)
    {
        UnlockConsole();
        return STATUS_INVALID_PARAMETER;
    }

    WCHAR *Source;
    WCHAR *Target;
    if (a->Unicode)
    {
        Source = (WCHAR*) InputSource;
        Target = (WCHAR*) InputTarget;
    }
    else
    {
        Source = new WCHAR[a->SourceLength];
        if (Source == nullptr)
        {
            UnlockConsole();
            return STATUS_NO_MEMORY;
        }
        Target = new WCHAR[a->TargetLength];
        if (Target == nullptr)
        {
            delete[] Source;
            UnlockConsole();
            return STATUS_NO_MEMORY;
        }
        a->SourceLength = (USHORT) ConvertInputToUnicode(g_ciConsoleInformation.CP, (CHAR*) InputSource, a->SourceLength, Source, a->SourceLength);
        a->SourceLength *= 2;
        a->TargetLength = (USHORT) ConvertInputToUnicode(g_ciConsoleInformation.CP, (CHAR*) InputTarget, a->TargetLength, Target, a->TargetLength);
        a->TargetLength *= 2;
    }

    // find specified exe.  if it's not there, add it if we're not removing an alias.
    PEXE_ALIAS_LIST ExeAliasList = FindExe(InputExeName, a->ExeLength, a->Unicode);
    if (ExeAliasList)
    {
        PALIAS Alias = FindAlias(ExeAliasList, Source, a->SourceLength);
        if (a->TargetLength)
        {
            if (Alias)
            {
                Status = ReplaceAlias(Alias, Target, a->TargetLength);
            }
            else
            {
                Status = AddAlias(ExeAliasList, Source, a->SourceLength, Target, a->TargetLength);
            }
        }
        else
        {
            if (Alias)
            {
                Status = RemoveAlias(Alias);
            }
        }
    }
    else
    {
        if (a->TargetLength)
        {
            ExeAliasList = AddExeAliasList(InputExeName, a->ExeLength, a->Unicode);
            if (ExeAliasList)
            {
                Status = AddAlias(ExeAliasList, Source, a->SourceLength, Target, a->TargetLength);
            }
            else
            {
                Status = STATUS_NO_MEMORY;
            }
        }
    }
    UnlockConsole();
    if (!a->Unicode)
    {
        delete[] Source;
        delete[] Target;
    }
    return Status;
}

// Routine Description:
// - This routine get a command line alias from the global set.
// Arguments:
// - m - message containing api parameters
// - ReplyStatus - Indicates whether to reply to the dll port.
// Return Value:
NTSTATUS SrvGetConsoleAlias(_Inout_ PCONSOLE_API_MSG m, _In_opt_ PBOOL const /*ReplyPending*/)
{
    PCONSOLE_GETALIAS_MSG const a = &m->u.consoleMsgL3.GetConsoleAliasW;

    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::GetConsoleAlias, a->Unicode);

    PVOID InputBuffer;
    ULONG InputBufferSize;
    NTSTATUS Status = NTSTATUS_FROM_HRESULT(m->GetInputBuffer(&InputBuffer, &InputBufferSize));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    PVOID InputExe;
    PVOID InputSource = nullptr;
    if (IsValidStringBuffer(a->Unicode, InputBuffer, InputBufferSize, 2, a->ExeLength, &InputExe, a->SourceLength, &InputSource) == FALSE)
    {
        return STATUS_INVALID_PARAMETER;
    }

    PVOID OutputBuffer;
    ULONG OutputBufferSize;
    Status = NTSTATUS_FROM_HRESULT(m->GetOutputBuffer(&OutputBuffer, &OutputBufferSize));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    if (OutputBufferSize > USHORT_MAX)
    {
        return STATUS_INVALID_PARAMETER;
    }

    a->TargetLength = (USHORT) OutputBufferSize;

    CONSOLE_INFORMATION *Console;

    Status = RevalidateConsole(&Console);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    LPWSTR Source;
    LPWSTR Target;
    if (a->Unicode)
    {
        Source = (PWSTR) InputSource;
        Target = (PWSTR) OutputBuffer;
    }
    else
    {
        Source = new WCHAR[a->SourceLength];
        if (Source == nullptr)
        {
            UnlockConsole();
            return STATUS_NO_MEMORY;
        }
        Target = new WCHAR[a->TargetLength];
        if (Target == nullptr)
        {
            delete[] Source;
            UnlockConsole();
            return STATUS_NO_MEMORY;
        }
        a->TargetLength = (USHORT) (a->TargetLength * sizeof(WCHAR));
        a->SourceLength = (USHORT) ConvertInputToUnicode(g_ciConsoleInformation.CP, (LPSTR) InputSource, a->SourceLength, Source, a->SourceLength);
        a->SourceLength *= 2;
    }

    PEXE_ALIAS_LIST const ExeAliasList = FindExe(InputExe, a->ExeLength, a->Unicode);
    if (ExeAliasList)
    {
        PALIAS const Alias = FindAlias(ExeAliasList, Source, a->SourceLength);
        if (Alias)
        {
            if (Alias->TargetLength + sizeof(WCHAR) > a->TargetLength)
            {
                Status = STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
                a->TargetLength = Alias->TargetLength + sizeof(WCHAR);
                memmove(Target, Alias->Target, Alias->TargetLength);
                Target[Alias->TargetLength / sizeof(WCHAR)] = L'\0';
            }
        }
        else
        {
            Status = STATUS_UNSUCCESSFUL;
        }
    }
    else
    {
        Status = STATUS_UNSUCCESSFUL;
    }
    if (!a->Unicode)
    {
        if (NT_SUCCESS(Status))
        {
            #pragma prefast(suppress:26019, "ConvertToOem is aware of buffer boundaries")
            a->TargetLength = (USHORT) ConvertToOem(g_ciConsoleInformation.CP,
                                                    (PWSTR) Target,
                                                    a->TargetLength / sizeof(WCHAR),
                                                    (LPSTR) OutputBuffer,
                                                    a->TargetLength);
        }
        delete[] Source;
        delete[] Target;
    }
    UnlockConsole();

    if (NT_SUCCESS(Status))
    {
        SetReplyInformation(m, a->TargetLength);
    }

    return Status;
}

NTSTATUS SrvGetConsoleAliasesLength(_Inout_ PCONSOLE_API_MSG m, _In_opt_ PBOOL const /*ReplyPending*/)
{
    PCONSOLE_GETALIASESLENGTH_MSG const a = &m->u.consoleMsgL3.GetConsoleAliasesLengthW;

    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::GetConsoleAliasesLength, a->Unicode);

    ULONG ExeNameLength;
    PVOID ExeName;
    NTSTATUS Status = NTSTATUS_FROM_HRESULT(m->GetInputBuffer(&ExeName, &ExeNameLength));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    if (ExeNameLength > USHORT_MAX)
    {
        return STATUS_INVALID_PARAMETER;
    }

    CONSOLE_INFORMATION *Console;
    Status = RevalidateConsole(&Console);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    a->AliasesLength = 0;
    PEXE_ALIAS_LIST const ExeAliasList = FindExe(ExeName, (USHORT) ExeNameLength, a->Unicode);
    if (ExeAliasList)
    {
        PLIST_ENTRY const ListHead = &ExeAliasList->AliasList;
        PLIST_ENTRY ListNext = ListHead->Flink;
        while (ListNext != ListHead)
        {
            PALIAS Alias = CONTAINING_RECORD(ListNext, ALIAS, ListLink);
            a->AliasesLength += Alias->SourceLength + Alias->TargetLength + (2 * sizeof(WCHAR));    // + 2 is for = and term null
            ListNext = ListNext->Flink;
        }
    }
    if (!a->Unicode)
    {
        a->AliasesLength /= sizeof(WCHAR);
    }
    UnlockConsole();
    return STATUS_SUCCESS;
}

VOID ClearAliases()
{
    PEXE_ALIAS_LIST const ExeAliasList = FindExe(L"cmd.exe", 14, TRUE);
    if (ExeAliasList == nullptr)
    {
        return;
    }

    PLIST_ENTRY const ListHead = &ExeAliasList->AliasList;
    PLIST_ENTRY ListNext = ListHead->Flink;
    while (ListNext != ListHead)
    {
        PALIAS const Alias = CONTAINING_RECORD(ListNext, ALIAS, ListLink);
        ListNext = ListNext->Flink;
        RemoveAlias(Alias);
    }
}

NTSTATUS SrvGetConsoleAliases(_Inout_ PCONSOLE_API_MSG m, _In_opt_ PBOOL const /*ReplyPending*/)
{
    PCONSOLE_GETALIASES_MSG const a = &m->u.consoleMsgL3.GetConsoleAliasesW;

    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::GetConsoleAliases, a->Unicode);

    PVOID ExeName;
    ULONG ExeNameLength;
    NTSTATUS Status = NTSTATUS_FROM_HRESULT(m->GetInputBuffer(&ExeName, &ExeNameLength));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    if (ExeNameLength > USHORT_MAX)
    {
        return STATUS_INVALID_PARAMETER;
    }

    PVOID OutputBuffer;
    DWORD AliasesBufferLength;
    Status = NTSTATUS_FROM_HRESULT(m->GetOutputBuffer(&OutputBuffer, &AliasesBufferLength));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    CONSOLE_INFORMATION *Console;
    Status = RevalidateConsole(&Console);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    LPWSTR AliasesBufferPtrW = nullptr;
    LPSTR AliasesBufferPtrA = nullptr;

    if (a->Unicode)
    {
        AliasesBufferPtrW = (PWSTR) OutputBuffer;
    }
    else
    {
        AliasesBufferPtrA = (LPSTR) OutputBuffer;
    }
    a->AliasesBufferLength = 0;
    PEXE_ALIAS_LIST const ExeAliasList = FindExe(ExeName, (USHORT) ExeNameLength, a->Unicode);
    if (ExeAliasList)
    {
        PLIST_ENTRY const ListHead = &ExeAliasList->AliasList;
        PLIST_ENTRY ListNext = ListHead->Flink;
        while (ListNext != ListHead)
        {
            PALIAS const Alias = CONTAINING_RECORD(ListNext, ALIAS, ListLink);
            if (a->Unicode)
            {
                if ((a->AliasesBufferLength + Alias->SourceLength + Alias->TargetLength + (2 * sizeof(WCHAR))) <= AliasesBufferLength)
                {
                    memmove(AliasesBufferPtrW, Alias->Source, Alias->SourceLength);
                    AliasesBufferPtrW += Alias->SourceLength / sizeof(WCHAR);
                    *AliasesBufferPtrW++ = (WCHAR)'=';
                    memmove(AliasesBufferPtrW, Alias->Target, Alias->TargetLength);
                    AliasesBufferPtrW += Alias->TargetLength / sizeof(WCHAR);
                    *AliasesBufferPtrW++ = (WCHAR)'\0';
                    a->AliasesBufferLength += Alias->SourceLength + Alias->TargetLength + (2 * sizeof(WCHAR));  // + 2 is for = and term null
                }
                else
                {
                    UnlockConsole();
                    return STATUS_BUFFER_OVERFLOW;
                }
            }
            else
            {
                if ((a->AliasesBufferLength + ((Alias->SourceLength + Alias->TargetLength) / sizeof(WCHAR)) + (2 * sizeof(CHAR))) <= AliasesBufferLength)
                {
                    USHORT SourceLength, TargetLength;
                    SourceLength = (USHORT) ConvertToOem(g_ciConsoleInformation.CP,
                                                         Alias->Source,
                                                         Alias->SourceLength / sizeof(WCHAR),
                                                         AliasesBufferPtrA,
                                                         Alias->SourceLength);
                    AliasesBufferPtrA += SourceLength;
                    *AliasesBufferPtrA++ = '=';
                    TargetLength = (USHORT) ConvertToOem(g_ciConsoleInformation.CP,
                                                         Alias->Target,
                                                         Alias->TargetLength / sizeof(WCHAR),
                                                         AliasesBufferPtrA,
                                                         Alias->TargetLength);
                    AliasesBufferPtrA += TargetLength;
                    *AliasesBufferPtrA++ = '\0';
                    a->AliasesBufferLength += SourceLength + TargetLength + (2 * sizeof(CHAR)); // + 2 is for = and term null
                }
                else
                {
                    UnlockConsole();
                    return STATUS_BUFFER_OVERFLOW;
                }
            }
            ListNext = ListNext->Flink;
        }
    }
    UnlockConsole();

    SetReplyInformation(m, a->AliasesBufferLength);

    return STATUS_SUCCESS;
}

NTSTATUS SrvGetConsoleAliasExesLength(_Inout_ PCONSOLE_API_MSG m, _In_opt_ PBOOL const /*ReplyPending*/)
{
    PCONSOLE_GETALIASEXESLENGTH_MSG const a = &m->u.consoleMsgL3.GetConsoleAliasExesLengthW;

    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::GetConsoleAliasExesLength, a->Unicode);

    CONSOLE_INFORMATION *Console;
    NTSTATUS Status = RevalidateConsole(&Console);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    a->AliasExesLength = 0;
    PLIST_ENTRY const ListHead = &g_ciConsoleInformation.ExeAliasList;
    PLIST_ENTRY ListNext = ListHead->Flink;
    while (ListNext != ListHead)
    {
        PEXE_ALIAS_LIST const AliasList = CONTAINING_RECORD(ListNext, EXE_ALIAS_LIST, ListLink);
        a->AliasExesLength += AliasList->ExeLength + (1 * sizeof(WCHAR));   // + 1 for term null
        ListNext = ListNext->Flink;
    }
    if (!a->Unicode)
    {
        a->AliasExesLength /= sizeof(WCHAR);
    }
    UnlockConsole();
    return STATUS_SUCCESS;
}

NTSTATUS SrvGetConsoleAliasExes(_Inout_ PCONSOLE_API_MSG m, _In_opt_ PBOOL const /*ReplyPending*/)
{
    PCONSOLE_GETALIASEXES_MSG const a = &m->u.consoleMsgL3.GetConsoleAliasExesW;

    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::GetConsoleAliasExes, a->Unicode);

    PVOID Buffer;
    DWORD AliasExesBufferLength;
    NTSTATUS Status = NTSTATUS_FROM_HRESULT(m->GetOutputBuffer(&Buffer, &AliasExesBufferLength));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    CONSOLE_INFORMATION *Console;
    Status = RevalidateConsole(&Console);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    LPWSTR AliasExesBufferPtrW = nullptr;
    LPSTR AliasExesBufferPtrA = nullptr;
    if (a->Unicode)
    {
        AliasExesBufferPtrW = (PWSTR) Buffer;
    }
    else
    {
        AliasExesBufferPtrA = (LPSTR) Buffer;
    }
    a->AliasExesBufferLength = 0;
    PLIST_ENTRY const ListHead = &g_ciConsoleInformation.ExeAliasList;
    PLIST_ENTRY ListNext = ListHead->Flink;
    while (ListNext != ListHead)
    {
        PEXE_ALIAS_LIST const AliasList = CONTAINING_RECORD(ListNext, EXE_ALIAS_LIST, ListLink);
        if (a->Unicode)
        {
            if ((a->AliasExesBufferLength + AliasList->ExeLength + (1 * sizeof(WCHAR))) <= AliasExesBufferLength)
            {
                memmove(AliasExesBufferPtrW, AliasList->ExeName, AliasList->ExeLength);
                AliasExesBufferPtrW += AliasList->ExeLength / sizeof(WCHAR);
                *AliasExesBufferPtrW++ = (WCHAR)'\0';
                a->AliasExesBufferLength += AliasList->ExeLength + (1 * sizeof(WCHAR)); // + 1 is term null
            }
            else
            {
                UnlockConsole();
                return STATUS_BUFFER_OVERFLOW;
            }
        }
        else
        {
            if ((a->AliasExesBufferLength + (AliasList->ExeLength / sizeof(WCHAR)) + (1 * sizeof(CHAR))) <= AliasExesBufferLength)
            {
                USHORT Length;
                Length = (USHORT) ConvertToOem(g_ciConsoleInformation.CP,
                                               AliasList->ExeName,
                                               AliasList->ExeLength / sizeof(WCHAR),
                                               AliasExesBufferPtrA,
                                               AliasList->ExeLength);
                AliasExesBufferPtrA += Length;
                *AliasExesBufferPtrA++ = (WCHAR)'\0';
                a->AliasExesBufferLength += Length + (1 * sizeof(CHAR));    // + 1 is term null
            }
            else
            {
                UnlockConsole();
                return STATUS_BUFFER_OVERFLOW;
            }
        }

        ListNext = ListNext->Flink;
    }

    SetReplyInformation(m, a->AliasExesBufferLength);

    UnlockConsole();
    return STATUS_SUCCESS;
}

#define MAX_ARGS 9

// Routine Description:
// - This routine matches the input string with an alias and copies the alias to the input buffer.
// Arguments:
// - pwchSource - string to match
// - cbSource - length of pwchSource in bytes
// - pwchTarget - where to store matched string
// - pcbTarget - on input, contains size of pwchTarget.  On output, contains length of alias stored in pwchTarget.
// - SourceIsCommandLine - if true, source buffer is a command line, where
//                         the first blank separate token is to be check for an alias, and if
//                         it matches, replaced with the value of the alias.  if false, then
//                         the source string is a null terminated alias name.
// - LineCount - aliases can contain multiple commands.  $T is the command separator
// Return Value:
// - SUCCESS - match was found and alias was copied to buffer.
NTSTATUS MatchAndCopyAlias(_In_reads_bytes_(cbSource) PWCHAR pwchSource,
                           _In_ USHORT cbSource,
                           _Out_writes_bytes_(*pcbTarget) PWCHAR pwchTarget,
                           _Inout_ PUSHORT pcbTarget,
                           _In_reads_bytes_(cbExe) PWCHAR pwchExe,
                           _In_ USHORT cbExe,
                           _Out_ PDWORD pcLines)
{
    NTSTATUS Status = STATUS_SUCCESS;

    // Alloc of exename may have failed.
    if (pwchExe == nullptr)
    {
        return STATUS_UNSUCCESSFUL;
    }

    // Find exe.
    PEXE_ALIAS_LIST const ExeAliasList = FindExe(pwchExe, cbExe, TRUE);
    if (ExeAliasList == nullptr)
    {
        return STATUS_UNSUCCESSFUL;
    }

    // Find first blank.
    PWCHAR Tmp = pwchSource;
    USHORT SourceUpToFirstBlank = 0; // in chars
    #pragma prefast(suppress:26019, "Legacy. This is bounded appropriately by cbSource.")
    for (; *Tmp != (WCHAR)' ' && SourceUpToFirstBlank < (USHORT) (cbSource / sizeof(WCHAR)); Tmp++, SourceUpToFirstBlank++)
    {
        /* Do nothing */
    }

    // find char past first blank
    USHORT j = SourceUpToFirstBlank;
    while (j < (USHORT) (cbSource / sizeof(WCHAR)) && *Tmp == (WCHAR)' ')
    {
        Tmp++;
        j++;
    }

    LPWSTR SourcePtr = Tmp;
    USHORT const SourceRemainderLength = (USHORT) ((cbSource / sizeof(WCHAR)) - j); // in chars

    // find alias
    PALIAS const Alias = FindAlias(ExeAliasList, pwchSource, (USHORT) (SourceUpToFirstBlank * sizeof(WCHAR)));
    if (Alias == nullptr)
    {
        return STATUS_UNSUCCESSFUL;
    }

    // Length is in bytes. Add 1 so dividing by WCHAR (2) is always rounding up.
    PWCHAR const TmpBuffer = new WCHAR[(*pcbTarget + 1) / sizeof(WCHAR)];
    if (!TmpBuffer)
    {
        return STATUS_NO_MEMORY;
    }

    // count args in target
    USHORT ArgCount = 0;
    *pcLines = 1;
    Tmp = Alias->Target;
    for (USHORT i = 0; (USHORT) (i + 1) < (USHORT) (Alias->TargetLength / sizeof(WCHAR)); i++)
    {
        if (*Tmp == (WCHAR)'$' && *(Tmp + 1) >= (WCHAR)'1' && *(Tmp + 1) <= (WCHAR)'9')
        {
            USHORT ArgNum = *(Tmp + 1) - (WCHAR)'0';

            if (ArgNum > ArgCount)
            {
                ArgCount = ArgNum;
            }

            Tmp++;
            i++;
        }
        else if (*Tmp == (WCHAR)'$' && *(Tmp + 1) == (WCHAR)'*')
        {
            if (ArgCount == 0)
            {
                ArgCount = 1;
            }

            Tmp++;
            i++;
        }

        Tmp++;
    }

    // Package up space separated strings in source into array of args.
    USHORT NumSourceArgs = 0;
    Tmp = SourcePtr;
    LPWSTR Args[MAX_ARGS];
    USHORT ArgsLength[MAX_ARGS];    // in bytes
    for (USHORT i = 0, j = 0; i < ArgCount; i++)
    {
        if (j < SourceRemainderLength)
        {
            Args[NumSourceArgs] = Tmp;
            ArgsLength[NumSourceArgs] = 0;
            while (j++ < SourceRemainderLength && *Tmp++ != (WCHAR)' ')
            {
                ArgsLength[NumSourceArgs] += sizeof(WCHAR);
            }

            while (j < SourceRemainderLength && *Tmp == (WCHAR)' ')
            {
                j++;
                Tmp++;
            }

            NumSourceArgs++;
        }
        else
        {
            break;
        }
    }

    // Put together the target string.
    PWCHAR Buffer = TmpBuffer;
    USHORT NewTargetLength = 2 * sizeof(WCHAR);    // for CRLF
    PWCHAR TargetAlias = Alias->Target;
    for (USHORT i = 0; i < (USHORT) (Alias->TargetLength / sizeof(WCHAR)); i++)
    {
        if (NewTargetLength >= *pcbTarget)
        {
            Status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        if (*TargetAlias == (WCHAR)'$' && (USHORT) (i + 1) < (USHORT) (Alias->TargetLength / sizeof(WCHAR)))
        {
            TargetAlias++;
            i++;
            if (*TargetAlias >= (WCHAR)'1' && *TargetAlias <= (WCHAR)'9')
            {
                // do numbered parameter substitution
                USHORT ArgNumber;

                ArgNumber = (USHORT) (*TargetAlias - (WCHAR)'1');
                if (ArgNumber < NumSourceArgs)
                {
                    if ((NewTargetLength + ArgsLength[ArgNumber]) <= *pcbTarget)
                    {
                        memmove(Buffer, Args[ArgNumber], ArgsLength[ArgNumber]);
                        Buffer += ArgsLength[ArgNumber] / sizeof(WCHAR);
                        NewTargetLength += ArgsLength[ArgNumber];
                    }
                    else
                    {
                        Status = STATUS_BUFFER_TOO_SMALL;
                        break;
                    }
                }
            }
            else if (*TargetAlias == (WCHAR)'*')
            {
                // Do * parameter substitution.
                if (NumSourceArgs)
                {
                    if ((USHORT) (NewTargetLength + (SourceRemainderLength * sizeof(WCHAR))) <= *pcbTarget)
                    {
                        memmove(Buffer, Args[0], SourceRemainderLength * sizeof(WCHAR));
                        Buffer += SourceRemainderLength;
                        NewTargetLength += SourceRemainderLength * sizeof(WCHAR);
                    }
                    else
                    {
                        Status = STATUS_BUFFER_TOO_SMALL;
                        break;
                    }
                }
            }
            else if (*TargetAlias == (WCHAR)'l' || *TargetAlias == (WCHAR)'L')
            {
                // Do < substitution.
                *Buffer++ = (WCHAR)'<';
                NewTargetLength += sizeof(WCHAR);
            }
            else if (*TargetAlias == (WCHAR)'g' || *TargetAlias == (WCHAR)'G')
            {
                // Do > substitution.
                *Buffer++ = (WCHAR)'>';
                NewTargetLength += sizeof(WCHAR);
            }
            else if (*TargetAlias == (WCHAR)'b' || *TargetAlias == (WCHAR)'B')
            {
                // Do | substitution.
                *Buffer++ = (WCHAR)'|';
                NewTargetLength += sizeof(WCHAR);
            }
            else if (*TargetAlias == (WCHAR)'t' || *TargetAlias == (WCHAR)'T')
            {
                // do newline substitution
                if ((USHORT) (NewTargetLength + (sizeof(WCHAR) * 2)) > *pcbTarget)
                {
                    Status = STATUS_BUFFER_TOO_SMALL;
                    break;
                }

                *pcLines += 1;
                *Buffer++ = UNICODE_CARRIAGERETURN;
                *Buffer++ = UNICODE_LINEFEED;
                NewTargetLength += sizeof(WCHAR) * 2;
            }
            else
            {
                // copy $X
                *Buffer++ = (WCHAR)'$';
                NewTargetLength += sizeof(WCHAR);
                *Buffer++ = *TargetAlias;
                NewTargetLength += sizeof(WCHAR);
            }
            TargetAlias++;
        }
        else
        {
            // copy char
            *Buffer++ = *TargetAlias++;
            NewTargetLength += sizeof(WCHAR);
        }
    }

    __analysis_assume(!NT_SUCCESS(Status) || NewTargetLength <= *pcbTarget);
    if (NT_SUCCESS(Status))
    {
        // We pre-reserve space for these two characters so we know there's enough room here.
        ASSERT(NewTargetLength <= *pcbTarget);
        *Buffer++ = UNICODE_CARRIAGERETURN;
        *Buffer++ = UNICODE_LINEFEED;

        memmove(pwchTarget, TmpBuffer, NewTargetLength);
    }

    delete[] TmpBuffer;
    *pcbTarget = NewTargetLength;

    return Status;
}

NTSTATUS SrvExpungeConsoleCommandHistory(_In_ PCONSOLE_API_MSG const m, _In_opt_ PBOOL const /*ReplyPending*/)
{
    PCONSOLE_EXPUNGECOMMANDHISTORY_MSG const a = &m->u.consoleMsgL3.ExpungeConsoleCommandHistoryW;
    PVOID ExeName;
    ULONG ExeNameLength;
    NTSTATUS Status = NTSTATUS_FROM_HRESULT(m->GetInputBuffer(&ExeName, &ExeNameLength));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    CONSOLE_INFORMATION *Console;
    Status = RevalidateConsole(&Console);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    EmptyCommandHistory(FindExeCommandHistory(ExeName, ExeNameLength, a->Unicode));
    UnlockConsole();
    return STATUS_SUCCESS;
}

NTSTATUS SrvSetConsoleNumberOfCommands(_In_ PCONSOLE_API_MSG const m, _In_opt_ PBOOL const /*ReplyPending*/)
{
    PCONSOLE_SETNUMBEROFCOMMANDS_MSG const a = &m->u.consoleMsgL3.SetConsoleNumberOfCommandsW;
    PVOID ExeName;
    ULONG ExeNameLength;
    NTSTATUS Status = NTSTATUS_FROM_HRESULT(m->GetInputBuffer(&ExeName, &ExeNameLength));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    CONSOLE_INFORMATION *Console;
    Status = RevalidateConsole(&Console);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    ReallocCommandHistory(FindExeCommandHistory(ExeName, ExeNameLength, a->Unicode), a->NumCommands);

    UnlockConsole();
    return STATUS_SUCCESS;
}

NTSTATUS SrvGetConsoleCommandHistoryLength(_Inout_ PCONSOLE_API_MSG m, _In_opt_ PBOOL const /*ReplyPending*/)
{
    PCONSOLE_GETCOMMANDHISTORYLENGTH_MSG const a = &m->u.consoleMsgL3.GetConsoleCommandHistoryLengthW;

    PVOID ExeName;
    ULONG ExeNameLength;
    NTSTATUS Status = NTSTATUS_FROM_HRESULT(m->GetInputBuffer(&ExeName, &ExeNameLength));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    CONSOLE_INFORMATION *Console;
    Status = RevalidateConsole(&Console);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    a->CommandHistoryLength = 0;

    PCOMMAND_HISTORY const CommandHistory = FindExeCommandHistory(ExeName, ExeNameLength, a->Unicode);
    if (CommandHistory)
    {
        for (SHORT i = 0; i < CommandHistory->NumberOfCommands; i++)
        {
            a->CommandHistoryLength += CommandHistory->Commands[i]->CommandLength + sizeof(WCHAR);
        }
    }

    if (!a->Unicode)
    {
        a->CommandHistoryLength /= sizeof(WCHAR);
    }

    UnlockConsole();
    return STATUS_SUCCESS;
}

NTSTATUS SrvGetConsoleCommandHistory(_Inout_ PCONSOLE_API_MSG m, _In_opt_ PBOOL const /*ReplyPending*/)
{
    PCONSOLE_GETCOMMANDHISTORY_MSG const a = &m->u.consoleMsgL3.GetConsoleCommandHistoryW;

    PVOID ExeName;
    ULONG ExeNameLength;
    NTSTATUS Status = NTSTATUS_FROM_HRESULT(m->GetInputBuffer(&ExeName, &ExeNameLength));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    PVOID OutputBuffer;
    Status = NTSTATUS_FROM_HRESULT(m->GetOutputBuffer(&OutputBuffer, &a->CommandBufferLength));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    CONSOLE_INFORMATION *Console;
    Status = RevalidateConsole(&Console);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    PWCHAR CommandBufferW = nullptr;
    PCHAR CommandBufferA = nullptr;
    if (a->Unicode)
    {
        CommandBufferW = (PWCHAR)OutputBuffer;
    }
    else
    {
        CommandBufferA = (PCHAR) OutputBuffer;
    }
    PCOMMAND_HISTORY const CommandHistory = FindExeCommandHistory(ExeName, ExeNameLength, a->Unicode);
    ULONG CommandHistoryLength = 0;
    ULONG NewCommandHistoryLength = 0;
    if (CommandHistory)
    {
        for (SHORT i = 0; i < CommandHistory->NumberOfCommands; i++)
        {
            if (a->Unicode)
            {
                if (SUCCEEDED(ULongAdd(CommandHistoryLength, CommandHistory->Commands[i]->CommandLength, &NewCommandHistoryLength)) &&
                    SUCCEEDED(ULongAdd(NewCommandHistoryLength, sizeof(WCHAR), &NewCommandHistoryLength)) &&
                    NewCommandHistoryLength <= a->CommandBufferLength)
                {
                    memmove(CommandBufferW, CommandHistory->Commands[i]->Command, CommandHistory->Commands[i]->CommandLength);
                    CommandBufferW += CommandHistory->Commands[i]->CommandLength / sizeof(WCHAR);
                    *CommandBufferW++ = (WCHAR)'\0';
                    CommandHistoryLength = NewCommandHistoryLength;
                }
                else
                {
                    Status = STATUS_BUFFER_OVERFLOW;
                    break;
                }
            }
            else
            {
                USHORT Length;
                Length = (USHORT) ConvertToOem(g_ciConsoleInformation.CP,
                                               CommandHistory->Commands[i]->Command,
                                               CommandHistory->Commands[i]->CommandLength / sizeof(WCHAR),
                                               CommandBufferA,
                                               a->CommandBufferLength - CommandHistoryLength);

                if (SUCCEEDED(ULongAdd(CommandHistoryLength, Length, &NewCommandHistoryLength)) &&
                    SUCCEEDED(ULongAdd(NewCommandHistoryLength, sizeof(CHAR), &NewCommandHistoryLength)) &&
                    NewCommandHistoryLength <= a->CommandBufferLength)
                {

                    CommandBufferA += Length;
                    *CommandBufferA++ = '\0';
                    CommandHistoryLength = NewCommandHistoryLength;
                }
                else
                {
                    Status = STATUS_BUFFER_OVERFLOW;
                    break;
                }
            }
        }
    }
    a->CommandBufferLength = CommandHistoryLength;
    UnlockConsole();

    if (NT_SUCCESS(Status))
    {
        SetReplyInformation(m, CommandHistoryLength);
    }

    return Status;
}

PCOMMAND_HISTORY ReallocCommandHistory(_In_opt_ PCOMMAND_HISTORY CurrentCommandHistory, _In_ DWORD const NumCommands)
{
    // To protect ourselves from overflow and general arithmetic errors, a limit of SHORT_MAX is put on the size of the command history.
    if (CurrentCommandHistory == nullptr || CurrentCommandHistory->MaximumNumberOfCommands == (SHORT)NumCommands || NumCommands > SHORT_MAX)
    {
        return CurrentCommandHistory;
    }

    PCOMMAND_HISTORY const History = (PCOMMAND_HISTORY) new BYTE[sizeof(COMMAND_HISTORY) + NumCommands * sizeof(PCOMMAND)];
    if (History == nullptr)
    {
        return CurrentCommandHistory;
    }

    *History = *CurrentCommandHistory;
    History->Flags |= CLE_RESET;
    History->NumberOfCommands = min(History->NumberOfCommands, (SHORT)NumCommands);
    History->LastAdded = History->NumberOfCommands - 1;
    History->LastDisplayed = History->NumberOfCommands - 1;
    History->FirstCommand = 0;
    History->MaximumNumberOfCommands = (SHORT)NumCommands;
    int i;
    for (i = 0; i < History->NumberOfCommands; i++)
    {
        History->Commands[i] = CurrentCommandHistory->Commands[COMMAND_NUM_TO_INDEX(i, CurrentCommandHistory)];
    }
    for (; i < CurrentCommandHistory->NumberOfCommands; i++)
    {
        #pragma prefast(suppress:6001, "Confused by 0 length array being used. This is fine until 0-size array is refactored.")
        delete[](CurrentCommandHistory->Commands[COMMAND_NUM_TO_INDEX(i, CurrentCommandHistory)]);
    }

    RemoveEntryList(&CurrentCommandHistory->ListLink);
    InitializeListHead(&History->PopupList);
    InsertHeadList(&g_ciConsoleInformation.CommandHistoryList, &History->ListLink);

    delete[] CurrentCommandHistory;
    return History;
}

PCOMMAND_HISTORY FindExeCommandHistory(_In_reads_(AppNameLength) PVOID AppName, _In_ DWORD AppNameLength, _In_ BOOLEAN const Unicode)
{
    PWCHAR AppNamePtr = nullptr;
    if (!Unicode)
    {
        AppNamePtr = new WCHAR[AppNameLength];
        if (AppNamePtr == nullptr)
        {
            return nullptr;
        }
        AppNameLength = ConvertInputToUnicode(g_ciConsoleInformation.CP, (PSTR)AppName, AppNameLength, AppNamePtr, AppNameLength);
        AppNameLength *= 2;
    }
    else
    {
        AppNamePtr = (PWCHAR)AppName;
    }

    PLIST_ENTRY const ListHead = &g_ciConsoleInformation.CommandHistoryList;
    PLIST_ENTRY ListNext = ListHead->Flink;
    while (ListNext != ListHead)
    {
        PCOMMAND_HISTORY const History = CONTAINING_RECORD(ListNext, COMMAND_HISTORY, ListLink);
        ListNext = ListNext->Flink;

        if (History->Flags & CLE_ALLOCATED && !_wcsnicmp(History->AppName, AppNamePtr, (USHORT) AppNameLength / sizeof(WCHAR)))
        {
            if (!Unicode)
            {
                delete[] AppNamePtr;
            }
            return History;
        }
    }
    if (!Unicode)
    {
        delete[] AppNamePtr;
    }
    return nullptr;
}

// Routine Description:
// - This routine returns the LRU command history buffer, or the command history buffer that corresponds to the app name.
// Arguments:
// - Console - pointer to console.
// Return Value:
// - Pointer to command history buffer.  if none are available, returns nullptr.
PCOMMAND_HISTORY AllocateCommandHistory(_In_reads_bytes_(cbAppName) PCWSTR pwszAppName, _In_ const DWORD cbAppName, _In_ HANDLE hProcess)
{
    // Reuse a history buffer.  The buffer must be !CLE_ALLOCATED.
    // If possible, the buffer should have the same app name.
    PLIST_ENTRY const ListHead = &g_ciConsoleInformation.CommandHistoryList;
    PLIST_ENTRY ListNext = ListHead->Blink;
    PCOMMAND_HISTORY BestCandidate = nullptr;
    PCOMMAND_HISTORY History = nullptr;
    BOOL SameApp = FALSE;
    while (ListNext != ListHead)
    {
        History = CONTAINING_RECORD(ListNext, COMMAND_HISTORY, ListLink);
        ListNext = ListNext->Blink;

        if ((History->Flags & CLE_ALLOCATED) == 0)
        {
            // use LRU history buffer with same app name
            if (History->AppName && !_wcsnicmp(History->AppName, pwszAppName, (USHORT) cbAppName / sizeof(WCHAR)))
            {
                BestCandidate = History;
                SameApp = TRUE;
                break;
            }

            // second best choice is LRU history buffer
            if (BestCandidate == nullptr)
            {
                BestCandidate = History;
            }
        }
    }

    // if there isn't a free buffer for the app name and the maximum number of
    // command history buffers hasn't been allocated, allocate a new one.
    if (!SameApp && g_ciConsoleInformation.NumCommandHistories < g_ciConsoleInformation.GetNumberOfHistoryBuffers())
    {
        size_t Size, TotalSize;

        if (FAILED(SizeTMult(g_ciConsoleInformation.GetHistoryBufferSize(), sizeof(PCOMMAND), &Size)))
        {
            return nullptr;
        }

        if (FAILED(SizeTAdd(sizeof(COMMAND_HISTORY), Size, &TotalSize)))
        {
            return nullptr;
        }

        History = (PCOMMAND_HISTORY) new BYTE[TotalSize];
        if (History == nullptr)
        {
            return nullptr;
        }

        // Length is in bytes. Add 1 so dividing by WCHAR (2) is always rounding up.
        History->AppName = new WCHAR[(cbAppName + 1) / sizeof(WCHAR)];
        if (History->AppName == nullptr)
        {
            delete[] History;
            return nullptr;
        }

        memmove(History->AppName, pwszAppName, cbAppName);
        History->Flags = CLE_ALLOCATED;
        History->NumberOfCommands = 0;
        History->LastAdded = -1;
        History->LastDisplayed = -1;
        History->FirstCommand = 0;
        History->MaximumNumberOfCommands = (SHORT)g_ciConsoleInformation.GetHistoryBufferSize();
        InsertHeadList(&g_ciConsoleInformation.CommandHistoryList, &History->ListLink);
        g_ciConsoleInformation.NumCommandHistories += 1;
        History->ProcessHandle = hProcess;
        InitializeListHead(&History->PopupList);
        return History;
    }

    // If the app name doesn't match, copy in the new app name and free the old commands.
    if (BestCandidate)
    {
        History = BestCandidate;
        ASSERT(CLE_NO_POPUPS(History));
        if (!SameApp)
        {
            SHORT i;
            if (History->AppName)
            {
                delete[] History->AppName;
            }

            for (i = 0; i < History->NumberOfCommands; i++)
            {
                delete[] History->Commands[i];
            }

            History->NumberOfCommands = 0;
            History->LastAdded = -1;
            History->LastDisplayed = -1;
            History->FirstCommand = 0;
            // Length is in bytes. Add 1 so dividing by WCHAR (2) is always rounding up.
            History->AppName = new WCHAR[(cbAppName + 1) / sizeof(WCHAR)];
            if (History->AppName == nullptr)
            {
                History->Flags &= ~CLE_ALLOCATED;
                return nullptr;
            }

            memmove(History->AppName, pwszAppName, cbAppName);
        }

        History->ProcessHandle = hProcess;
        History->Flags |= CLE_ALLOCATED;

        // move to the front of the list
        RemoveEntryList(&BestCandidate->ListLink);
        InsertHeadList(&g_ciConsoleInformation.CommandHistoryList, &BestCandidate->ListLink);
    }

    return BestCandidate;
}

NTSTATUS BeginPopup(_In_ PSCREEN_INFORMATION ScreenInfo, _In_ PCOMMAND_HISTORY CommandHistory, _In_ COORD PopupSize)
{
    // determine popup dimensions
    COORD Size = PopupSize;
    Size.X += 2;    // add borders
    Size.Y += 2;    // add borders
    if (Size.X >= (SHORT)(ScreenInfo->GetScreenWindowSizeX()))
    {
        Size.X = (SHORT)(ScreenInfo->GetScreenWindowSizeX());
    }
    if (Size.Y >= (SHORT)(ScreenInfo->GetScreenWindowSizeY()))
    {
        Size.Y = (SHORT)(ScreenInfo->GetScreenWindowSizeY());
    }

    // make sure there's enough room for the popup borders

    if (Size.X < 2 || Size.Y < 2)
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    // determine origin.  center popup on window
    COORD Origin;
    Origin.X = (SHORT)((ScreenInfo->GetScreenWindowSizeX() - Size.X) / 2 + ScreenInfo->BufferViewport.Left);
    Origin.Y = (SHORT)((ScreenInfo->GetScreenWindowSizeY() - Size.Y) / 2 + ScreenInfo->BufferViewport.Top);

    // allocate a popup structure
    PCLE_POPUP const Popup = new CLE_POPUP();
    if (Popup == nullptr)
    {
        return STATUS_NO_MEMORY;
    }

    // allocate a buffer
    Popup->OldScreenSize = ScreenInfo->ScreenBufferSize;
    Popup->OldContents = new CHAR_INFO[Popup->OldScreenSize.X * Size.Y];
    if (Popup->OldContents == nullptr)
    {
        delete Popup;
        return STATUS_NO_MEMORY;
    }

    // fill in popup structure
    InsertHeadList(&CommandHistory->PopupList, &Popup->ListLink);
    Popup->Region.Left = Origin.X;
    Popup->Region.Top = Origin.Y;
    Popup->Region.Right = (SHORT)(Origin.X + Size.X - 1);
    Popup->Region.Bottom = (SHORT)(Origin.Y + Size.Y - 1);
    Popup->Attributes = ScreenInfo->GetPopupAttributes()->GetLegacyAttributes();
    Popup->BottomIndex = COMMAND_INDEX_TO_NUM(CommandHistory->LastDisplayed, CommandHistory);

    // copy old contents
    SMALL_RECT TargetRect;
    TargetRect.Left = 0;
    TargetRect.Top = Popup->Region.Top;
    TargetRect.Right = Popup->OldScreenSize.X - 1;
    TargetRect.Bottom = Popup->Region.Bottom;
    ReadScreenBuffer(ScreenInfo, Popup->OldContents, &TargetRect);

    g_ciConsoleInformation.PopupCount++;
    DrawCommandListBorder(Popup, ScreenInfo);
    return STATUS_SUCCESS;
}

NTSTATUS EndPopup(_In_ PSCREEN_INFORMATION ScreenInfo, _In_ PCOMMAND_HISTORY CommandHistory)
{
    ASSERT(!CLE_NO_POPUPS(CommandHistory));
    if (CLE_NO_POPUPS(CommandHistory))
    {
        return STATUS_UNSUCCESSFUL;
    }

    PCLE_POPUP const Popup = CONTAINING_RECORD(CommandHistory->PopupList.Flink, CLE_POPUP, ListLink);

    // restore previous contents to screen
    COORD Size;
    Size.X = Popup->OldScreenSize.X;
    Size.Y = (SHORT)(Popup->Region.Bottom - Popup->Region.Top + 1);

    SMALL_RECT SourceRect;
    SourceRect.Left = 0;
    SourceRect.Top = Popup->Region.Top;
    SourceRect.Right = Popup->OldScreenSize.X - 1;
    SourceRect.Bottom = Popup->Region.Bottom;

    WriteScreenBuffer(ScreenInfo, Popup->OldContents, &SourceRect);
    WriteToScreen(ScreenInfo, &SourceRect);

    // Free popup structure.
    RemoveEntryList(&Popup->ListLink);
    delete[] Popup->OldContents;
    delete Popup;
    g_ciConsoleInformation.PopupCount--;

    return STATUS_SUCCESS;
}

void CleanUpPopups(_In_ PCOOKED_READ_DATA const CookedReadData)
{
    PCOMMAND_HISTORY const CommandHistory = CookedReadData->CommandHistory;
    if (CommandHistory == nullptr)
    {
        return;
    }

    while (!CLE_NO_POPUPS(CommandHistory))
    {
        EndPopup(CookedReadData->pScreenInfo, CommandHistory);
    }
}

CommandLine::CommandLine()
{

}

CommandLine::~CommandLine()
{

}

CommandLine& CommandLine::Instance()
{
    static CommandLine c;
    return c;
}

bool CommandLine::IsEditLineEmpty() const
{
    const COOKED_READ_DATA* const pTyped = g_ciConsoleInformation.lpCookedReadData;

    if (nullptr == pTyped)
    {
        // If the cooked read data pointer is null, there is no edit line data and therefore it's empty.
        return true;
    }
    else if (0 == pTyped->NumberOfVisibleChars)
    {
        // If we had a valid pointer, but there are no visible characters for the edit line, then it's empty.
        // Someone started editing and back spaced the whole line out so it exists, but has no data.
        return true;
    }
    else
    {
        return false;
    }
}

void CommandLine::Hide(_In_ bool const fUpdateFields)
{
    if (!IsEditLineEmpty())
    {
        PCOOKED_READ_DATA CookedReadData = g_ciConsoleInformation.lpCookedReadData;
        DeleteCommandLine(CookedReadData, fUpdateFields);
    }
}

void CommandLine::Show()
{
    if (!IsEditLineEmpty())
    {
        PCOOKED_READ_DATA CookedReadData = g_ciConsoleInformation.lpCookedReadData;
        RedrawCommandLine(CookedReadData);
    }
}

void DeleteCommandLine(_Inout_ PCOOKED_READ_DATA const pCookedReadData, _In_ const BOOL fUpdateFields)
{
    DWORD CharsToWrite = pCookedReadData->NumberOfVisibleChars;
    COORD Coord = pCookedReadData->OriginalCursorPosition;

    // catch the case where the current command has scrolled off the top of the screen.
    if (Coord.Y < 0)
    {
        CharsToWrite += pCookedReadData->pScreenInfo->ScreenBufferSize.X * Coord.Y;
        CharsToWrite += pCookedReadData->OriginalCursorPosition.X;   // account for prompt
        pCookedReadData->OriginalCursorPosition.X = 0;
        pCookedReadData->OriginalCursorPosition.Y = 0;
        Coord.X = 0;
        Coord.Y = 0;
    }

    if (!CheckBisectStringW(pCookedReadData->BackupLimit,
                            CharsToWrite,
                            pCookedReadData->pScreenInfo->ScreenBufferSize.X - pCookedReadData->OriginalCursorPosition.X))
    {
        CharsToWrite++;
    }

    FillOutput(pCookedReadData->pScreenInfo,
               (WCHAR)' ',
               Coord,
               CONSOLE_FALSE_UNICODE,    // faster than real unicode
               &CharsToWrite);

    if (fUpdateFields)
    {
        pCookedReadData->BufPtr = pCookedReadData->BackupLimit;
        pCookedReadData->BytesRead = 0;
        pCookedReadData->CurrentPosition = 0;
        pCookedReadData->NumberOfVisibleChars = 0;
    }

    pCookedReadData->pScreenInfo->SetCursorPosition(pCookedReadData->OriginalCursorPosition, TRUE);
}

void RedrawCommandLine(_Inout_ PCOOKED_READ_DATA const pCookedReadData)
{
    if (pCookedReadData->Echo)
    {
        // Draw the command line
        pCookedReadData->OriginalCursorPosition = pCookedReadData->pScreenInfo->TextInfo->GetCursor()->GetPosition();

        SHORT ScrollY = 0;
        #pragma prefast(suppress:28931, "Status is not unused. It's used in debug assertions.")
        NTSTATUS Status = WriteCharsLegacy(pCookedReadData->pScreenInfo,
                                           pCookedReadData->BackupLimit,
                                           pCookedReadData->BackupLimit,
                                           pCookedReadData->BackupLimit,
                                           &pCookedReadData->BytesRead,
                                           &pCookedReadData->NumberOfVisibleChars,
                                           pCookedReadData->OriginalCursorPosition.X,
                                           WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                           &ScrollY);
        ASSERT(NT_SUCCESS(Status));

        pCookedReadData->OriginalCursorPosition.Y += ScrollY;

        // Move the cursor back to the right position
        COORD CursorPosition = pCookedReadData->OriginalCursorPosition;
        CursorPosition.X += (SHORT)RetrieveTotalNumberOfSpaces(pCookedReadData->OriginalCursorPosition.X,
                                                               pCookedReadData->BackupLimit,
                                                               pCookedReadData->CurrentPosition);
        if (CheckBisectStringW(pCookedReadData->BackupLimit,
                               pCookedReadData->CurrentPosition,
                               pCookedReadData->pScreenInfo->ScreenBufferSize.X - pCookedReadData->OriginalCursorPosition.X))
        {
            CursorPosition.X++;
        }
        Status = AdjustCursorPosition(pCookedReadData->pScreenInfo, CursorPosition, TRUE, nullptr);
        ASSERT(NT_SUCCESS(Status));
    }
}

NTSTATUS RetrieveNthCommand(_In_ PCOMMAND_HISTORY CommandHistory, _In_ SHORT Index, // index, not command number
                            _In_reads_bytes_(BufferSize)
                            PWCHAR Buffer, _In_ ULONG BufferSize, _Out_ PULONG CommandSize)
{
    ASSERT(Index < CommandHistory->NumberOfCommands);
    CommandHistory->LastDisplayed = Index;
    PCOMMAND const CommandRecord = CommandHistory->Commands[Index];
    if (CommandRecord->CommandLength > (USHORT) BufferSize)
    {
        *CommandSize = (USHORT) BufferSize; // room for CRLF?
    }
    else
    {
        *CommandSize = CommandRecord->CommandLength;
    }

    memmove(Buffer, CommandRecord->Command, *CommandSize);
    return STATUS_SUCCESS;
}

// Routine Description:
// - This routine copies the commandline specified by Index into the cooked read buffer
void SetCurrentCommandLine(_In_ PCOOKED_READ_DATA const CookedReadData, _In_ SHORT Index) // index, not command number
{
    DeleteCommandLine(CookedReadData, TRUE);
    #pragma prefast(suppress:28931, "Status is not unused. Used by assertions.")
    NTSTATUS Status = RetrieveNthCommand(CookedReadData->CommandHistory, Index, CookedReadData->BackupLimit, CookedReadData->BufferSize, &CookedReadData->BytesRead);
    ASSERT(NT_SUCCESS(Status));
    ASSERT(CookedReadData->BackupLimit == CookedReadData->BufPtr);
    if (CookedReadData->Echo)
    {
        SHORT ScrollY = 0;
        Status = WriteCharsLegacy(CookedReadData->pScreenInfo,
                                  CookedReadData->BackupLimit,
                                  CookedReadData->BufPtr,
                                  CookedReadData->BufPtr,
                                  &CookedReadData->BytesRead,
                                  &CookedReadData->NumberOfVisibleChars,
                                  CookedReadData->OriginalCursorPosition.X,
                                  WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                  &ScrollY);
        ASSERT(NT_SUCCESS(Status));
        CookedReadData->OriginalCursorPosition.Y += ScrollY;
    }

    DWORD const CharsToWrite = CookedReadData->BytesRead / sizeof(WCHAR);
    CookedReadData->CurrentPosition = CharsToWrite;
    CookedReadData->BufPtr = CookedReadData->BackupLimit + CharsToWrite;
}

bool IsCommandLinePopupKey(_In_ PKEY_EVENT_RECORD const pKeyEvent)
{
    if (!(pKeyEvent->dwControlKeyState & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED | RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED)))
    {
        switch (pKeyEvent->wVirtualKeyCode)
        {
            case VK_ESCAPE:
            case VK_PRIOR:
            case VK_NEXT:
            case VK_END:
            case VK_HOME:
            case VK_LEFT:
            case VK_UP:
            case VK_RIGHT:
            case VK_DOWN:
            case VK_F2:
            case VK_F4:
            case VK_F7:
            case VK_F9:
                return true;
            default:
                break;
        }
    }

    // Extended key handling
    if (g_ciConsoleInformation.GetExtendedEditKey()  && ParseEditKeyInfo(pKeyEvent))
    {
        return pKeyEvent->uChar.UnicodeChar == 0;
    }

    return false;
}

bool IsCommandLineEditingKey(_In_ PKEY_EVENT_RECORD const pKeyEvent)
{
    if (!(pKeyEvent->dwControlKeyState & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED | RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED)))
    {
        switch (pKeyEvent->wVirtualKeyCode)
        {
            case VK_ESCAPE:
            case VK_PRIOR:
            case VK_NEXT:
            case VK_END:
            case VK_HOME:
            case VK_LEFT:
            case VK_UP:
            case VK_RIGHT:
            case VK_DOWN:
            case VK_INSERT:
            case VK_DELETE:
            case VK_F1:
            case VK_F2:
            case VK_F3:
            case VK_F4:
            case VK_F5:
            case VK_F6:
            case VK_F7:
            case VK_F8:
            case VK_F9:
                return true;
            default:
                break;
        }
    }
    if ((pKeyEvent->dwControlKeyState & (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED)))
    {
        switch (pKeyEvent->wVirtualKeyCode)
        {
            case VK_END:
            case VK_HOME:
            case VK_LEFT:
            case VK_RIGHT:
                return true;
            default:
                break;
        }
    }

    // Extended edit key handling
    if (g_ciConsoleInformation.GetExtendedEditKey()  && ParseEditKeyInfo(pKeyEvent))
    {
        // If wUnicodeChar is specified in KeySubst,
        // the key should be handled as a normal key.
        // Basically this is for VK_BACK keys.
        return pKeyEvent->uChar.UnicodeChar == 0;
    }

    if ((pKeyEvent->dwControlKeyState & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED)))
    {
        switch (pKeyEvent->wVirtualKeyCode)
        {
            case VK_F7:
            case VK_F10:
                return true;
            default:
                break;
        }
    }
    return false;
}

// Routine Description:
// - This routine handles the command list popup.  It returns when we're out of input or the user has selected a command line.
// Return Value:
// - CONSOLE_STATUS_WAIT - we ran out of input, so a wait block was created
// - CONSOLE_STATUS_READ_COMPLETE - user hit return
NTSTATUS ProcessCommandListInput(_In_ PCOOKED_READ_DATA const pCookedReadData, _In_ PCONSOLE_API_MSG WaitReplyMessage, _In_ BOOLEAN WaitRoutine)
{
    PCOMMAND_HISTORY const pCommandHistory = pCookedReadData->CommandHistory;
    PCLE_POPUP const Popup = CONTAINING_RECORD(pCommandHistory->PopupList.Flink, CLE_POPUP, ListLink);
    NTSTATUS Status = STATUS_SUCCESS;
    for (;;)
    {
        WCHAR Char;
        BOOLEAN CommandLinePopupKeys = FALSE;

        Status = GetChar(pCookedReadData->pInputInfo,
                         &Char,
                         TRUE,
                         pCookedReadData->pInputReadHandleData,
                         WaitReplyMessage,
                         (ConsoleWaitRoutine)CookedReadWaitRoutine,
                         pCookedReadData,
                         sizeof(*pCookedReadData),
                         WaitRoutine,
                         nullptr,
                         &CommandLinePopupKeys,
                         nullptr,
                         nullptr);
        if (!NT_SUCCESS(Status))
        {
            if (Status != CONSOLE_STATUS_WAIT)
            {
                pCookedReadData->BytesRead = 0;
            }
            return Status;
        }

        SHORT Index;
        if (CommandLinePopupKeys)
        {
            switch (Char)
            {
                case VK_F9:
                {
                    // prompt the user to enter the desired command number. copy that command to the command line.
                    COORD PopupSize;
                    if (pCookedReadData->CommandHistory &&
                        pCookedReadData->pScreenInfo->ScreenBufferSize.X >= MINIMUM_COMMAND_PROMPT_SIZE + 2)
                    {
                        // 2 is for border
                        PopupSize.X = COMMAND_NUMBER_PROMPT_LENGTH + COMMAND_NUMBER_LENGTH;
                        PopupSize.Y = 1;
                        Status = BeginPopup(pCookedReadData->pScreenInfo, pCookedReadData->CommandHistory, PopupSize);
                        if (NT_SUCCESS(Status))
                        {
                            // CommandNumberPopup does EndPopup call
                            return CommandNumberPopup(pCookedReadData, WaitReplyMessage, WaitRoutine);
                        }
                    }
                    break;
                }
                case VK_ESCAPE:
                    EndPopup(pCookedReadData->pScreenInfo, pCommandHistory);
                    pCookedReadData->pInputReadHandleData->IncrementReadCount();
                    return CONSOLE_STATUS_WAIT_NO_BLOCK;
                case VK_UP:
                    UpdateCommandListPopup(-1, &Popup->CurrentCommand, pCommandHistory, Popup, pCookedReadData->pScreenInfo, 0);
                    break;
                case VK_DOWN:
                    UpdateCommandListPopup(1, &Popup->CurrentCommand, pCommandHistory, Popup, pCookedReadData->pScreenInfo, 0);
                    break;
                case VK_END:
                    // Move waaay forward, UpdateCommandListPopup() can handle it.
                    UpdateCommandListPopup((SHORT)(pCommandHistory->NumberOfCommands),
                                           &Popup->CurrentCommand,
                                           pCommandHistory,
                                           Popup,
                                           pCookedReadData->pScreenInfo,
                                           0);
                    break;
                case VK_HOME:
                    // Move waaay back, UpdateCommandListPopup() can handle it.
                    UpdateCommandListPopup((SHORT)-(pCommandHistory->NumberOfCommands),
                                           &Popup->CurrentCommand,
                                           pCommandHistory,
                                           Popup,
                                           pCookedReadData->pScreenInfo,
                                           0);
                    break;
                case VK_PRIOR:
                    UpdateCommandListPopup((SHORT)-POPUP_SIZE_Y(Popup),
                                           &Popup->CurrentCommand,
                                           pCommandHistory,
                                           Popup,
                                           pCookedReadData->pScreenInfo,
                                           0);
                    break;
                case VK_NEXT:
                    UpdateCommandListPopup(POPUP_SIZE_Y(Popup),
                                           &Popup->CurrentCommand,
                                           pCommandHistory,
                                           Popup,
                                           pCookedReadData->pScreenInfo, 0);
                    break;
                case VK_LEFT:
                case VK_RIGHT:
                    Index = Popup->CurrentCommand;
                    EndPopup(pCookedReadData->pScreenInfo, pCommandHistory);
                    SetCurrentCommandLine(pCookedReadData, Index);
                    pCookedReadData->pInputReadHandleData->IncrementReadCount();
                    return CONSOLE_STATUS_WAIT_NO_BLOCK;
                default:
                    break;
            }
        }
        else if (Char == UNICODE_CARRIAGERETURN)
        {
            ULONG i, lStringLength;
            DWORD LineCount = 1;
            Index = Popup->CurrentCommand;
            EndPopup(pCookedReadData->pScreenInfo, pCommandHistory);
            SetCurrentCommandLine(pCookedReadData, Index);
            lStringLength = pCookedReadData->BytesRead;
            ProcessCookedReadInput(pCookedReadData, UNICODE_CARRIAGERETURN, 0, &Status);
            // complete read
            if (pCookedReadData->Echo)
            {
                // check for alias
                i = pCookedReadData->BufferSize;
                if (NT_SUCCESS(MatchAndCopyAlias(pCookedReadData->BackupLimit,
                                                 (USHORT) lStringLength,
                                                 pCookedReadData->BackupLimit,
                                                 (PUSHORT) & i,
                                                 pCookedReadData->ExeName,
                                                 pCookedReadData->ExeNameLength,
                                                 &LineCount)))
                {
                    pCookedReadData->BytesRead = i;
                }
            }

            SetReplyStatus(WaitReplyMessage, STATUS_SUCCESS);
            PCONSOLE_READCONSOLE_MSG const a = &WaitReplyMessage->u.consoleMsgL1.ReadConsole;
            if (pCookedReadData->BytesRead > pCookedReadData->UserBufferSize || LineCount > 1)
            {
                if (LineCount > 1)
                {
                    PWSTR Tmp;
                    SetFlag(pCookedReadData->pInputReadHandleData->InputHandleFlags, INPUT_READ_HANDLE_DATA::HandleFlags::MultiLineInput);
                    for (Tmp = pCookedReadData->BackupLimit; *Tmp != UNICODE_LINEFEED; Tmp++)
                        ASSERT(Tmp < (pCookedReadData->BackupLimit + pCookedReadData->BytesRead));
                    a->NumBytes = (ULONG) (Tmp - pCookedReadData->BackupLimit + 1) * sizeof(*Tmp);
                }
                else
                {
                    a->NumBytes = pCookedReadData->UserBufferSize;
                }
                SetFlag(pCookedReadData->pInputReadHandleData->InputHandleFlags, INPUT_READ_HANDLE_DATA::HandleFlags::InputPending);
                pCookedReadData->pInputReadHandleData->BufPtr = pCookedReadData->BackupLimit;
                pCookedReadData->pInputReadHandleData->BytesAvailable = pCookedReadData->BytesRead - a->NumBytes;
                pCookedReadData->pInputReadHandleData->CurrentBufPtr = (PWCHAR)((PBYTE) pCookedReadData->BackupLimit + a->NumBytes);
                memmove(pCookedReadData->UserBuffer, pCookedReadData->BackupLimit, a->NumBytes);
            }
            else
            {
                a->NumBytes = pCookedReadData->BytesRead;
                memmove(pCookedReadData->UserBuffer, pCookedReadData->BackupLimit, a->NumBytes);
            }

            if (!a->Unicode)
            {
                PCHAR TransBuffer;

                // If ansi, translate string.
                TransBuffer = (PCHAR) new BYTE[a->NumBytes];
                if (TransBuffer == nullptr)
                {
                    return STATUS_NO_MEMORY;
                }

                a->NumBytes = (ULONG) ConvertToOem(g_ciConsoleInformation.CP,
                                                   pCookedReadData->UserBuffer,
                                                   a->NumBytes / sizeof(WCHAR),
                                                   TransBuffer,
                                                   a->NumBytes);
                memmove(pCookedReadData->UserBuffer, TransBuffer, a->NumBytes);
                delete[] TransBuffer;
            }

            return CONSOLE_STATUS_READ_COMPLETE;
        }
        else
        {
            Index = FindMatchingCommand(pCookedReadData->CommandHistory, &Char, 1 * sizeof(WCHAR), Popup->CurrentCommand, FMCFL_JUST_LOOKING);
            if (Index != -1)
            {
                UpdateCommandListPopup((SHORT)(Index - Popup->CurrentCommand),
                                       &Popup->CurrentCommand,
                                       pCommandHistory,
                                       Popup,
                                       pCookedReadData->pScreenInfo,
                                       UCLP_WRAP);
            }
        }
    }
}

// Routine Description:
// - This routine handles the delete from cursor to char char popup.  It returns when we're out of input or the user has entered a char.
// Return Value:
// - CONSOLE_STATUS_WAIT - we ran out of input, so a wait block was created
// - CONSOLE_STATUS_READ_COMPLETE - user hit return
NTSTATUS ProcessCopyFromCharInput(_In_ PCOOKED_READ_DATA const pCookedReadData, _In_ PCONSOLE_API_MSG WaitReplyMessage, _In_ BOOLEAN WaitRoutine)
{
    NTSTATUS Status = STATUS_SUCCESS;
    for (;;)
    {
        WCHAR Char;
        BOOLEAN CommandLinePopupKeys = FALSE;
        Status = GetChar(pCookedReadData->pInputInfo,
                         &Char,
                         TRUE,
                         pCookedReadData->pInputReadHandleData,
                         WaitReplyMessage,
                         (ConsoleWaitRoutine) CookedReadWaitRoutine,
                         pCookedReadData,
                         sizeof(*pCookedReadData),
                         WaitRoutine,
                         nullptr,
                         &CommandLinePopupKeys,
                         nullptr,
                         nullptr);
        if (!NT_SUCCESS(Status))
        {
            if (Status != CONSOLE_STATUS_WAIT)
            {
                pCookedReadData->BytesRead = 0;
            }

            return Status;
        }

        if (CommandLinePopupKeys)
        {
            switch (Char)
            {
                case VK_ESCAPE:
                    EndPopup(pCookedReadData->pScreenInfo, pCookedReadData->CommandHistory);
                    pCookedReadData->pInputReadHandleData->IncrementReadCount();
                    return CONSOLE_STATUS_WAIT_NO_BLOCK;
            }
        }

        EndPopup(pCookedReadData->pScreenInfo, pCookedReadData->CommandHistory);

        int i;  // char index (not byte)
        // delete from cursor up to specified char
        for (i = pCookedReadData->CurrentPosition + 1; i < (int)(pCookedReadData->BytesRead / sizeof(WCHAR)); i++)
        {
            if (pCookedReadData->BackupLimit[i] == Char)
            {
                break;
            }
        }

        if (i != (int)(pCookedReadData->BytesRead / sizeof(WCHAR) + 1))
        {
            COORD CursorPosition;

            // save cursor position
            CursorPosition = pCookedReadData->pScreenInfo->TextInfo->GetCursor()->GetPosition();

            // Delete commandline.
            DeleteCommandLine(pCookedReadData, FALSE);

            // Delete chars.
            memmove(&pCookedReadData->BackupLimit[pCookedReadData->CurrentPosition],
                          &pCookedReadData->BackupLimit[i],
                          pCookedReadData->BytesRead - (i * sizeof(WCHAR)));
            pCookedReadData->BytesRead -= (i - pCookedReadData->CurrentPosition) * sizeof(WCHAR);

            // Write commandline.
            if (pCookedReadData->Echo)
            {
                Status = WriteCharsLegacy(pCookedReadData->pScreenInfo,
                                          pCookedReadData->BackupLimit,
                                          pCookedReadData->BackupLimit,
                                          pCookedReadData->BackupLimit,
                                          &pCookedReadData->BytesRead,
                                          &pCookedReadData->NumberOfVisibleChars,
                                          pCookedReadData->OriginalCursorPosition.X,
                                          WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                          nullptr);
                ASSERT(NT_SUCCESS(Status));
            }

            // restore cursor position
            Status = pCookedReadData->pScreenInfo->SetCursorPosition(CursorPosition, TRUE);
            ASSERT(NT_SUCCESS(Status));
        }

        pCookedReadData->pInputReadHandleData->IncrementReadCount();
        return CONSOLE_STATUS_WAIT_NO_BLOCK;
    }
}

// Routine Description:
// - This routine handles the delete char popup.  It returns when we're out of input or the user has entered a char.
// Return Value:
// - CONSOLE_STATUS_WAIT - we ran out of input, so a wait block was created
// - CONSOLE_STATUS_READ_COMPLETE - user hit return
NTSTATUS ProcessCopyToCharInput(_In_ PCOOKED_READ_DATA const pCookedReadData, _In_ PCONSOLE_API_MSG WaitReplyMessage, _In_ BOOLEAN WaitRoutine)
{
    NTSTATUS Status = STATUS_SUCCESS;
    for (;;)
    {
        WCHAR Char;
        BOOLEAN CommandLinePopupKeys = FALSE;
        Status = GetChar(pCookedReadData->pInputInfo,
                         &Char,
                         TRUE,
                         pCookedReadData->pInputReadHandleData,
                         WaitReplyMessage,
                         (ConsoleWaitRoutine) CookedReadWaitRoutine,
                         pCookedReadData,
                         sizeof(*pCookedReadData),
                         WaitRoutine,
                         nullptr,
                         &CommandLinePopupKeys,
                         nullptr,
                         nullptr);
        if (!NT_SUCCESS(Status))
        {
            if (Status != CONSOLE_STATUS_WAIT)
            {
                pCookedReadData->BytesRead = 0;
            }
            return Status;
        }

        if (CommandLinePopupKeys)
        {
            switch (Char)
            {
                case VK_ESCAPE:
                    EndPopup(pCookedReadData->pScreenInfo, pCookedReadData->CommandHistory);
                    pCookedReadData->pInputReadHandleData->IncrementReadCount();
                    return CONSOLE_STATUS_WAIT_NO_BLOCK;
            }
        }

        EndPopup(pCookedReadData->pScreenInfo, pCookedReadData->CommandHistory);

        // copy up to specified char
        PCOMMAND const LastCommand = GetLastCommand(pCookedReadData->CommandHistory);
        if (LastCommand)
        {
            int i, j;

            // find specified char in last command
            for (i = pCookedReadData->CurrentPosition + 1; i < (int)(LastCommand->CommandLength / sizeof(WCHAR)); i++)
            {
                if (LastCommand->Command[i] == Char)
                {
                    break;
                }
            }

            // If we found it, copy up to it.
            if (i < (int)(LastCommand->CommandLength / sizeof(WCHAR)) &&
                (USHORT) (LastCommand->CommandLength / sizeof(WCHAR)) > (USHORT) pCookedReadData->CurrentPosition)
            {
                j = i - pCookedReadData->CurrentPosition;
                ASSERT(j > 0);
                memmove(pCookedReadData->BufPtr, &LastCommand->Command[pCookedReadData->CurrentPosition], j * sizeof(WCHAR));
                pCookedReadData->CurrentPosition += j;
                j *= sizeof(WCHAR);
                pCookedReadData->BytesRead = max(pCookedReadData->BytesRead, pCookedReadData->CurrentPosition * sizeof(WCHAR));
                if (pCookedReadData->Echo)
                {
                    DWORD NumSpaces;
                    SHORT ScrollY = 0;

                    Status = WriteCharsLegacy(pCookedReadData->pScreenInfo,
                                              pCookedReadData->BackupLimit,
                                              pCookedReadData->BufPtr,
                                              pCookedReadData->BufPtr,
                                              (PDWORD) & j,
                                              &NumSpaces,
                                              pCookedReadData->OriginalCursorPosition.X,
                                              WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                              &ScrollY);
                    ASSERT(NT_SUCCESS(Status));
                    pCookedReadData->OriginalCursorPosition.Y += ScrollY;
                    pCookedReadData->NumberOfVisibleChars += NumSpaces;
                }

                pCookedReadData->BufPtr += j / sizeof(WCHAR);
            }
        }

        pCookedReadData->pInputReadHandleData->IncrementReadCount();
        return CONSOLE_STATUS_WAIT_NO_BLOCK;
    }
}

// Routine Description:
// - This routine handles the command number selection popup.
// Return Value:
// - CONSOLE_STATUS_WAIT - we ran out of input, so a wait block was created
// - CONSOLE_STATUS_READ_COMPLETE - user hit return
NTSTATUS ProcessCommandNumberInput(_In_ PCOOKED_READ_DATA const pCookedReadData, _In_ PCONSOLE_API_MSG WaitReplyMessage, _In_ BOOLEAN WaitRoutine)
{
    PCOMMAND_HISTORY const CommandHistory = pCookedReadData->CommandHistory;
    PCLE_POPUP const Popup = CONTAINING_RECORD(CommandHistory->PopupList.Flink, CLE_POPUP, ListLink);
    NTSTATUS Status = STATUS_SUCCESS;
    for (;;)
    {
        WCHAR Char;
        BOOLEAN CommandLinePopupKeys;

        Status = GetChar(pCookedReadData->pInputInfo,
                         &Char,
                         TRUE,
                         pCookedReadData->pInputReadHandleData,
                         WaitReplyMessage,
                         (ConsoleWaitRoutine)CookedReadWaitRoutine,
                         pCookedReadData,
                         sizeof(*pCookedReadData),
                         WaitRoutine,
                         nullptr,
                         &CommandLinePopupKeys,
                         nullptr,
                         nullptr);
        if (!NT_SUCCESS(Status))
        {
            if (Status != CONSOLE_STATUS_WAIT)
            {
                pCookedReadData->BytesRead = 0;
            }
            return Status;
        }

        if (Char >= (WCHAR)0x30 && Char <= (WCHAR)0x39)
        {
            if (Popup->NumberRead < 5)
            {
                DWORD CharsToWrite = sizeof(WCHAR);
                const TextAttribute* pRealAttributes = pCookedReadData->pScreenInfo->GetAttributes();
                pCookedReadData->pScreenInfo->SetAttributes(&Popup->Attributes);
                DWORD NumSpaces;
                Status = WriteCharsLegacy(pCookedReadData->pScreenInfo,
                                          Popup->NumberBuffer,
                                          &Popup->NumberBuffer[Popup->NumberRead],
                                          &Char,
                                          &CharsToWrite,
                                          &NumSpaces,
                                          pCookedReadData->OriginalCursorPosition.X,
                                          WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                          nullptr);
                ASSERT(NT_SUCCESS(Status));
                pCookedReadData->pScreenInfo->SetAttributes(pRealAttributes);
                Popup->NumberBuffer[Popup->NumberRead] = Char;
                Popup->NumberRead += 1;
            }
        }
        else if (Char == UNICODE_BACKSPACE)
        {
            if (Popup->NumberRead > 0)
            {
                DWORD CharsToWrite = sizeof(WCHAR);
                const TextAttribute* pRealAttributes = pCookedReadData->pScreenInfo->GetAttributes();
                pCookedReadData->pScreenInfo->SetAttributes(&Popup->Attributes);
                DWORD NumSpaces;
                Status = WriteCharsLegacy(pCookedReadData->pScreenInfo,
                                          Popup->NumberBuffer,
                                          &Popup->NumberBuffer[Popup->NumberRead],
                                          &Char,
                                          &CharsToWrite,
                                          &NumSpaces,
                                          pCookedReadData->OriginalCursorPosition.X,
                                          WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                          nullptr);

                ASSERT(NT_SUCCESS(Status));
                pCookedReadData->pScreenInfo->SetAttributes(pRealAttributes);
                Popup->NumberBuffer[Popup->NumberRead] = (WCHAR)' ';
                Popup->NumberRead -= 1;
            }
        }
        else if (Char == (WCHAR)VK_ESCAPE)
        {
            EndPopup(pCookedReadData->pScreenInfo, pCookedReadData->CommandHistory);
            if (!CLE_NO_POPUPS(CommandHistory))
            {
                EndPopup(pCookedReadData->pScreenInfo, pCookedReadData->CommandHistory);
            }

            // Note that CookedReadData's OriginalCursorPosition is the position before ANY text was entered on the edit line.
            // We want to use the position before the cursor was moved for this popup handler specifically, which may be *anywhere* in the edit line
            // and will be synchronized with the pointers in the CookedReadData structure (BufPtr, etc.)
            pCookedReadData->pScreenInfo->SetCursorPosition(pCookedReadData->BeforeDialogCursorPosition, TRUE);
        }
        else if (Char == UNICODE_CARRIAGERETURN)
        {
            CHAR NumberBuffer[6];
            int i;

            // This is guaranteed above.
            __analysis_assume(Popup->NumberRead < 6);
            for (i = 0; i < Popup->NumberRead; i++)
            {
                ASSERT(i < ARRAYSIZE(NumberBuffer));
                NumberBuffer[i] = (CHAR) Popup->NumberBuffer[i];
            }
            NumberBuffer[i] = 0;

            SHORT CommandNumber = (SHORT)atoi(NumberBuffer);
            if ((WORD) CommandNumber >= (WORD) pCookedReadData->CommandHistory->NumberOfCommands)
            {
                CommandNumber = (SHORT)(pCookedReadData->CommandHistory->NumberOfCommands - 1);
            }

            EndPopup(pCookedReadData->pScreenInfo, pCookedReadData->CommandHistory);
            if (!CLE_NO_POPUPS(CommandHistory))
            {
                EndPopup(pCookedReadData->pScreenInfo, pCookedReadData->CommandHistory);
            }
            SetCurrentCommandLine(pCookedReadData, COMMAND_NUM_TO_INDEX(CommandNumber, pCookedReadData->CommandHistory));
        }
        pCookedReadData->pInputReadHandleData->IncrementReadCount();
        return CONSOLE_STATUS_WAIT_NO_BLOCK;
    }
}

// Routine Description:
// - This routine handles the command list popup.  It puts up the popup, then calls ProcessCommandListInput to get and process input.
// Return Value:
// - CONSOLE_STATUS_WAIT - we ran out of input, so a wait block was created
// - STATUS_SUCCESS - read was fully completed (user hit return)
NTSTATUS CommandListPopup(_In_ PCOOKED_READ_DATA const CookedReadData, _In_ PCONSOLE_API_MSG WaitReplyMessage, _In_ BOOLEAN WaitRoutine)
{
    PCOMMAND_HISTORY const CommandHistory = CookedReadData->CommandHistory;
    PCLE_POPUP const Popup = CONTAINING_RECORD(CommandHistory->PopupList.Flink, CLE_POPUP, ListLink);

    SHORT const CurrentCommand = COMMAND_INDEX_TO_NUM(CommandHistory->LastDisplayed, CommandHistory);

    if (CurrentCommand < (SHORT)(CommandHistory->NumberOfCommands - POPUP_SIZE_Y(Popup)))
    {
        Popup->BottomIndex = (SHORT)(max(CurrentCommand, POPUP_SIZE_Y(Popup) - 1));
    }
    else
    {
        Popup->BottomIndex = (SHORT)(CommandHistory->NumberOfCommands - 1);
    }
    Popup->CurrentCommand = CommandHistory->LastDisplayed;
    DrawCommandListPopup(Popup, CommandHistory->LastDisplayed, CommandHistory, CookedReadData->pScreenInfo);
    Popup->PopupInputRoutine = (PCLE_POPUP_INPUT_ROUTINE) ProcessCommandListInput;
    return ProcessCommandListInput(CookedReadData, WaitReplyMessage, WaitRoutine);
}

VOID DrawPromptPopup(_In_ PCLE_POPUP Popup, _In_ PSCREEN_INFORMATION ScreenInfo, _In_reads_(PromptLength) PWCHAR Prompt, _In_ ULONG PromptLength)
{
    // Draw empty popup.
    COORD WriteCoord;
    WriteCoord.X = (SHORT)(Popup->Region.Left + 1);
    WriteCoord.Y = (SHORT)(Popup->Region.Top + 1);
    ULONG lStringLength = POPUP_SIZE_X(Popup);
    for (SHORT i = 0; i < POPUP_SIZE_Y(Popup); i++)
    {
        FillOutput(ScreenInfo, Popup->Attributes.GetLegacyAttributes(), WriteCoord, CONSOLE_ATTRIBUTE, &lStringLength);
        FillOutput(ScreenInfo, (WCHAR)' ', WriteCoord, CONSOLE_FALSE_UNICODE,   // faster that real unicode
                   &lStringLength);

        WriteCoord.Y += 1;
    }

    WriteCoord.X = (SHORT)(Popup->Region.Left + 1);
    WriteCoord.Y = (SHORT)(Popup->Region.Top + 1);

    // write prompt to screen
    lStringLength = PromptLength;
    if (lStringLength > (ULONG) POPUP_SIZE_X(Popup))
    {
        lStringLength = (ULONG) (POPUP_SIZE_X(Popup));
    }

    WriteOutputString(ScreenInfo, Prompt, WriteCoord, CONSOLE_REAL_UNICODE, &lStringLength, nullptr);
}

// Routine Description:
// - This routine handles the "delete up to this char" popup.  It puts up the popup, then calls ProcessCopyFromCharInput to get and process input.
// Return Value:
// - CONSOLE_STATUS_WAIT - we ran out of input, so a wait block was created
// - STATUS_SUCCESS - read was fully completed (user hit return)
NTSTATUS CopyFromCharPopup(_In_ PCOOKED_READ_DATA CookedReadData, _In_ PCONSOLE_API_MSG WaitReplyMessage, _In_ BOOLEAN WaitRoutine)
{
    WCHAR ItemString[70];
    int ItemLength = 0;
    LANGID LangId;
    NTSTATUS Status = GetConsoleLangId(g_ciConsoleInformation.OutputCP, &LangId);
    if (NT_SUCCESS(Status))
    {
        ItemLength = LoadStringEx(g_hInstance, ID_CONSOLE_MSGCMDLINEF4, ItemString, ARRAYSIZE(ItemString), LangId);
    }

    if (!NT_SUCCESS(Status) || ItemLength == 0)
    {
        ItemLength = LoadStringW(g_hInstance, ID_CONSOLE_MSGCMDLINEF4, ItemString, ARRAYSIZE(ItemString));
    }

    PCOMMAND_HISTORY const CommandHistory = CookedReadData->CommandHistory;
    PCLE_POPUP const Popup = CONTAINING_RECORD(CommandHistory->PopupList.Flink, CLE_POPUP, ListLink);

    DrawPromptPopup(Popup, CookedReadData->pScreenInfo, ItemString, ItemLength);
    Popup->PopupInputRoutine = (PCLE_POPUP_INPUT_ROUTINE) ProcessCopyFromCharInput;

    return ProcessCopyFromCharInput(CookedReadData, WaitReplyMessage, WaitRoutine);
}

// Routine Description:
// - This routine handles the "copy up to this char" popup.  It puts up the popup, then calls ProcessCopyToCharInput to get and process input.
// Return Value:
// - CONSOLE_STATUS_WAIT - we ran out of input, so a wait block was created
// - STATUS_SUCCESS - read was fully completed (user hit return)
NTSTATUS CopyToCharPopup(_In_ PCOOKED_READ_DATA CookedReadData, _In_ PCONSOLE_API_MSG WaitReplyMessage, _In_ BOOLEAN WaitRoutine)
{
    WCHAR ItemString[70];
    int ItemLength = 0;
    LANGID LangId;

    NTSTATUS Status = GetConsoleLangId(g_ciConsoleInformation.OutputCP, &LangId);
    if (NT_SUCCESS(Status))
    {
        ItemLength = LoadStringEx(g_hInstance, ID_CONSOLE_MSGCMDLINEF2, ItemString, ARRAYSIZE(ItemString), LangId);
    }

    if (!NT_SUCCESS(Status) || ItemLength == 0)
    {
        ItemLength = LoadStringW(g_hInstance, ID_CONSOLE_MSGCMDLINEF2, ItemString, ARRAYSIZE(ItemString));
    }

    PCOMMAND_HISTORY const CommandHistory = CookedReadData->CommandHistory;
    PCLE_POPUP const Popup = CONTAINING_RECORD(CommandHistory->PopupList.Flink, CLE_POPUP, ListLink);
    DrawPromptPopup(Popup, CookedReadData->pScreenInfo, ItemString, ItemLength);
    Popup->PopupInputRoutine = (PCLE_POPUP_INPUT_ROUTINE) ProcessCopyToCharInput;
    return ProcessCopyToCharInput(CookedReadData, WaitReplyMessage, WaitRoutine);
}

// Routine Description:
// - This routine handles the "enter command number" popup.  It puts up the popup, then calls ProcessCommandNumberInput to get and process input.
// Return Value:
// - CONSOLE_STATUS_WAIT - we ran out of input, so a wait block was created
// - STATUS_SUCCESS - read was fully completed (user hit return)
NTSTATUS CommandNumberPopup(_In_ PCOOKED_READ_DATA const CookedReadData, _In_ PCONSOLE_API_MSG WaitReplyMessage, _In_ BOOLEAN WaitRoutine)
{
    WCHAR ItemString[70];
    int ItemLength = 0;
    LANGID LangId;

    NTSTATUS Status = GetConsoleLangId(g_ciConsoleInformation.OutputCP, &LangId);
    if (NT_SUCCESS(Status))
    {
        ItemLength = LoadStringEx(g_hInstance, ID_CONSOLE_MSGCMDLINEF9, ItemString, ARRAYSIZE(ItemString), LangId);
    }
    if (!NT_SUCCESS(Status) || ItemLength == 0)
    {
        ItemLength = LoadStringW(g_hInstance, ID_CONSOLE_MSGCMDLINEF9, ItemString, ARRAYSIZE(ItemString));
    }

    PCOMMAND_HISTORY const CommandHistory = CookedReadData->CommandHistory;
    PCLE_POPUP const Popup = CONTAINING_RECORD(CommandHistory->PopupList.Flink, CLE_POPUP, ListLink);

    if (ItemLength > POPUP_SIZE_X(Popup) - COMMAND_NUMBER_LENGTH)
    {
        ItemLength = POPUP_SIZE_X(Popup) - COMMAND_NUMBER_LENGTH;
    }
    DrawPromptPopup(Popup, CookedReadData->pScreenInfo, ItemString, ItemLength);

    // Save the original cursor position in case the user cancels out of the dialog
    CookedReadData->BeforeDialogCursorPosition = CookedReadData->pScreenInfo->TextInfo->GetCursor()->GetPosition();

    // Move the cursor into the dialog so the user can type multiple characters for the command number
    COORD CursorPosition;
    CursorPosition.X = (SHORT)(Popup->Region.Right - MINIMUM_COMMAND_PROMPT_SIZE);
    CursorPosition.Y = (SHORT)(Popup->Region.Top + 1);
    CookedReadData->pScreenInfo->SetCursorPosition(CursorPosition, TRUE);

    // Prepare the popup
    Popup->NumberRead = 0;
    Popup->PopupInputRoutine = (PCLE_POPUP_INPUT_ROUTINE) ProcessCommandNumberInput;

    // Transfer control to the handler routine
    return ProcessCommandNumberInput(CookedReadData, WaitReplyMessage, WaitRoutine);
}


PCOMMAND GetLastCommand(_In_ PCOMMAND_HISTORY CommandHistory)
{
    if (CommandHistory->NumberOfCommands == 0)
    {
        return nullptr;
    }
    else
    {
        return CommandHistory->Commands[CommandHistory->LastDisplayed];
    }
}

void EmptyCommandHistory(_In_opt_ PCOMMAND_HISTORY CommandHistory)
{
    if (CommandHistory == nullptr)
    {
        return;
    }

    for (SHORT i = 0; i < CommandHistory->NumberOfCommands; i++)
    {
        delete[] CommandHistory->Commands[i];
    }

    CommandHistory->NumberOfCommands = 0;
    CommandHistory->LastAdded = -1;
    CommandHistory->LastDisplayed = -1;
    CommandHistory->FirstCommand = 0;
    CommandHistory->Flags = CLE_RESET;
}

BOOL AtFirstCommand(_In_ PCOMMAND_HISTORY CommandHistory)
{
    if (CommandHistory == nullptr)
    {
        return FALSE;
    }

    if (CommandHistory->Flags & CLE_RESET)
    {
        return FALSE;
    }

    SHORT i = (SHORT)(CommandHistory->LastDisplayed - 1);
    if (i == -1)
    {
        i = (SHORT)(CommandHistory->NumberOfCommands - 1);
    }

    return (i == CommandHistory->LastAdded);
}

BOOL AtLastCommand(_In_ PCOMMAND_HISTORY CommandHistory)
{
    if (CommandHistory == nullptr)
    {
        return FALSE;
    }
    else
    {
        return (CommandHistory->LastDisplayed == CommandHistory->LastAdded);
    }
}

// TODO: [MSFT:4586207] Clean up this mess -- needs helpers. http://osgvsowi/4586207
// Routine Description:
// - This routine process command line editing keys.
// Return Value:
// - CONSOLE_STATUS_WAIT - CommandListPopup ran out of input
// - CONSOLE_STATUS_READ_COMPLETE - user hit <enter> in CommandListPopup
// - STATUS_SUCCESS - everything's cool
NTSTATUS ProcessCommandLine(_In_ PCOOKED_READ_DATA pCookedReadData,
                            _In_ WCHAR wch,
                            _In_ const DWORD dwKeyState,
                            _In_opt_ PCONSOLE_API_MSG pWaitReplyMessage,
                            _In_ const BOOLEAN fWaitRoutine)
{
    COORD CurrentPosition = { 0 };
    DWORD CharsToWrite;
    NTSTATUS Status;
    SHORT ScrollY = 0;
    BOOL fStartFromDelim;

    BOOL UpdateCursorPosition = FALSE;
    if (wch == VK_F7 && (dwKeyState & (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED | RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED)) == 0)
    {
        COORD PopupSize;

        if (pCookedReadData->CommandHistory && pCookedReadData->CommandHistory->NumberOfCommands)
        {
            PopupSize.X = 40;
            PopupSize.Y = 10;
            Status = BeginPopup(pCookedReadData->pScreenInfo, pCookedReadData->CommandHistory, PopupSize);
            if (NT_SUCCESS(Status))
            {
                // CommandListPopup does EndPopup call
                return CommandListPopup(pCookedReadData, pWaitReplyMessage, fWaitRoutine);
            }
        }
    }
    else
    {
        switch (wch)
        {
            case VK_ESCAPE:
                DeleteCommandLine(pCookedReadData, TRUE);
                break;
            case VK_UP:
            case VK_DOWN:
            case VK_F5:
                if (wch == VK_F5)
                    wch = VK_UP;
                // for doskey compatibility, buffer isn't circular
                if (wch == VK_UP && !AtFirstCommand(pCookedReadData->CommandHistory) || wch == VK_DOWN && !AtLastCommand(pCookedReadData->CommandHistory))
                {
                    DeleteCommandLine(pCookedReadData, TRUE);
                    Status = RetrieveCommand(pCookedReadData->CommandHistory,
                                             wch,
                                             pCookedReadData->BackupLimit,
                                             pCookedReadData->BufferSize,
                                             &pCookedReadData->BytesRead);
                    ASSERT(pCookedReadData->BackupLimit == pCookedReadData->BufPtr);
                    if (pCookedReadData->Echo)
                    {
                        Status = WriteCharsLegacy(pCookedReadData->pScreenInfo,
                                                  pCookedReadData->BackupLimit,
                                                  pCookedReadData->BufPtr,
                                                  pCookedReadData->BufPtr,
                                                  &pCookedReadData->BytesRead,
                                                  &pCookedReadData->NumberOfVisibleChars,
                                                  pCookedReadData->OriginalCursorPosition.X,
                                                  WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                                  &ScrollY);
                        ASSERT(NT_SUCCESS(Status));
                        pCookedReadData->OriginalCursorPosition.Y += ScrollY;
                    }
                    CharsToWrite = pCookedReadData->BytesRead / sizeof(WCHAR);
                    pCookedReadData->CurrentPosition = CharsToWrite;
                    pCookedReadData->BufPtr = pCookedReadData->BackupLimit + CharsToWrite;
                }
                break;
            case VK_PRIOR:
            case VK_NEXT:
                if (pCookedReadData->CommandHistory && pCookedReadData->CommandHistory->NumberOfCommands)
                {
                    // display oldest or newest command
                    SHORT CommandNumber;
                    if (wch == VK_PRIOR)
                    {
                        CommandNumber = 0;
                    }
                    else
                    {
                        CommandNumber = (SHORT)(pCookedReadData->CommandHistory->NumberOfCommands - 1);
                    }
                    DeleteCommandLine(pCookedReadData, TRUE);
                    Status = RetrieveNthCommand(pCookedReadData->CommandHistory,
                                                COMMAND_NUM_TO_INDEX(CommandNumber, pCookedReadData->CommandHistory),
                                                pCookedReadData->BackupLimit,
                                                pCookedReadData->BufferSize,
                                                &pCookedReadData->BytesRead);
                    ASSERT(pCookedReadData->BackupLimit == pCookedReadData->BufPtr);
                    if (pCookedReadData->Echo)
                    {
                        Status = WriteCharsLegacy(pCookedReadData->pScreenInfo,
                                                  pCookedReadData->BackupLimit,
                                                  pCookedReadData->BufPtr,
                                                  pCookedReadData->BufPtr,
                                                  &pCookedReadData->BytesRead,
                                                  &pCookedReadData->NumberOfVisibleChars,
                                                  pCookedReadData->OriginalCursorPosition.X,
                                                  WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                                  &ScrollY);
                        ASSERT(NT_SUCCESS(Status));
                        pCookedReadData->OriginalCursorPosition.Y += ScrollY;
                    }
                    CharsToWrite = pCookedReadData->BytesRead / sizeof(WCHAR);
                    pCookedReadData->CurrentPosition = CharsToWrite;
                    pCookedReadData->BufPtr = pCookedReadData->BackupLimit + CharsToWrite;
                }
                break;
            case VK_END:
                if (dwKeyState & (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED))
                {
                    DeleteCommandLine(pCookedReadData, FALSE);
                    pCookedReadData->BytesRead = pCookedReadData->CurrentPosition * sizeof(WCHAR);
                    if (pCookedReadData->Echo)
                    {
                        Status = WriteCharsLegacy(pCookedReadData->pScreenInfo,
                                                  pCookedReadData->BackupLimit,
                                                  pCookedReadData->BackupLimit,
                                                  pCookedReadData->BackupLimit,
                                                  &pCookedReadData->BytesRead,
                                                  &pCookedReadData->NumberOfVisibleChars,
                                                  pCookedReadData->OriginalCursorPosition.X,
                                                  WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                                  nullptr);
                        ASSERT(NT_SUCCESS(Status));
                    }
                }
                else
                {
                    pCookedReadData->CurrentPosition = pCookedReadData->BytesRead / sizeof(WCHAR);
                    pCookedReadData->BufPtr = pCookedReadData->BackupLimit + pCookedReadData->CurrentPosition;
                    CurrentPosition.X = (SHORT)(pCookedReadData->OriginalCursorPosition.X + pCookedReadData->NumberOfVisibleChars);
                    CurrentPosition.Y = pCookedReadData->OriginalCursorPosition.Y;
                    if (CheckBisectProcessW(pCookedReadData->pScreenInfo,
                                            pCookedReadData->BackupLimit,
                                            pCookedReadData->CurrentPosition,
                                            pCookedReadData->pScreenInfo->ScreenBufferSize.X - pCookedReadData->OriginalCursorPosition.X,
                                            pCookedReadData->OriginalCursorPosition.X,
                                            TRUE))
                    {
                        CurrentPosition.X++;
                    }
                    UpdateCursorPosition = TRUE;
                }
                break;
            case VK_HOME:
                if (dwKeyState & (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED))
                {
                    DeleteCommandLine(pCookedReadData, FALSE);
                    pCookedReadData->BytesRead -= pCookedReadData->CurrentPosition * sizeof(WCHAR);
                    pCookedReadData->CurrentPosition = 0;
                    memmove(pCookedReadData->BackupLimit, pCookedReadData->BufPtr, pCookedReadData->BytesRead);
                    pCookedReadData->BufPtr = pCookedReadData->BackupLimit;
                    if (pCookedReadData->Echo)
                    {
                        Status = WriteCharsLegacy(pCookedReadData->pScreenInfo,
                                                  pCookedReadData->BackupLimit,
                                                  pCookedReadData->BackupLimit,
                                                  pCookedReadData->BackupLimit,
                                                  &pCookedReadData->BytesRead,
                                                  &pCookedReadData->NumberOfVisibleChars,
                                                  pCookedReadData->OriginalCursorPosition.X,
                                                  WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                                  nullptr);
                        ASSERT(NT_SUCCESS(Status));
                    }
                    CurrentPosition = pCookedReadData->OriginalCursorPosition;
                    UpdateCursorPosition = TRUE;
                }
                else
                {
                    pCookedReadData->CurrentPosition = 0;
                    pCookedReadData->BufPtr = pCookedReadData->BackupLimit;
                    CurrentPosition = pCookedReadData->OriginalCursorPosition;
                    UpdateCursorPosition = TRUE;
                }
                break;
            case VK_LEFT:
                if (dwKeyState & (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED))
                {
                    PWCHAR LastWord;
                    BOOL NonSpaceCharSeen = FALSE;
                    if (pCookedReadData->BufPtr != pCookedReadData->BackupLimit)
                    {
                        if (!g_ciConsoleInformation.GetExtendedEditKey())
                        {
                            LastWord = pCookedReadData->BufPtr - 1;
                            while (LastWord != pCookedReadData->BackupLimit)
                            {
                                if (!IS_WORD_DELIM(*LastWord))
                                {
                                    NonSpaceCharSeen = TRUE;
                                }
                                else
                                {
                                    if (NonSpaceCharSeen)
                                    {
                                        break;
                                    }
                                }
                                LastWord--;
                            }
                            if (LastWord != pCookedReadData->BackupLimit)
                            {
                                pCookedReadData->BufPtr = LastWord + 1;
                            }
                            else
                            {
                                pCookedReadData->BufPtr = LastWord;
                            }
                        }
                        else
                        {
                            // A bit better word skipping.
                            LastWord = pCookedReadData->BufPtr - 1;
                            if (LastWord != pCookedReadData->BackupLimit)
                            {
                                if (*LastWord == L' ')
                                {
                                    // Skip spaces, until the non-space character is found.
                                    while (--LastWord != pCookedReadData->BackupLimit)
                                    {
                                        ASSERT(LastWord > pCookedReadData->BackupLimit);
                                        if (*LastWord != L' ')
                                        {
                                            break;
                                        }
                                    }
                                }
                                if (LastWord != pCookedReadData->BackupLimit)
                                {
                                    if (IS_WORD_DELIM(*LastWord))
                                    {
                                        // Skip WORD_DELIMs until space or non WORD_DELIM is found.
                                        while (--LastWord != pCookedReadData->BackupLimit)
                                        {
                                            ASSERT(LastWord > pCookedReadData->BackupLimit);
                                            if (*LastWord == L' ' || !IS_WORD_DELIM(*LastWord))
                                            {
                                                break;
                                            }
                                        }
                                    }
                                    else
                                    {
                                        // Skip the regular words
                                        while (--LastWord != pCookedReadData->BackupLimit)
                                        {
                                            ASSERT(LastWord > pCookedReadData->BackupLimit);
                                            if (IS_WORD_DELIM(*LastWord))
                                            {
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                            ASSERT(LastWord >= pCookedReadData->BackupLimit);
                            if (LastWord != pCookedReadData->BackupLimit)
                            {
                                /*
                                 * LastWord is currently pointing to the last character
                                 * of the previous word, unless it backed up to the beginning
                                 * of the buffer.
                                 * Let's increment LastWord so that it points to the expeced
                                 * insertion point.
                                 */
                                ++LastWord;
                            }
                            pCookedReadData->BufPtr = LastWord;
                        }
                        pCookedReadData->CurrentPosition = (ULONG) (pCookedReadData->BufPtr - pCookedReadData->BackupLimit);
                        CurrentPosition = pCookedReadData->OriginalCursorPosition;
                        CurrentPosition.X = (SHORT)(CurrentPosition.X +
                                                    RetrieveTotalNumberOfSpaces(pCookedReadData->OriginalCursorPosition.X,
                                                                                pCookedReadData->BackupLimit, pCookedReadData->CurrentPosition));
                        if (CheckBisectStringW(pCookedReadData->BackupLimit,
                                               pCookedReadData->CurrentPosition + 1,
                                               pCookedReadData->pScreenInfo->ScreenBufferSize.X - pCookedReadData->OriginalCursorPosition.X))
                        {
                            CurrentPosition.X++;
                        }

                        UpdateCursorPosition = TRUE;
                    }
                }
                else
                {
                    if (pCookedReadData->BufPtr != pCookedReadData->BackupLimit)
                    {
                        pCookedReadData->BufPtr--;
                        pCookedReadData->CurrentPosition--;
                        CurrentPosition.X = pCookedReadData->pScreenInfo->TextInfo->GetCursor()->GetPosition().X;
                        CurrentPosition.Y = pCookedReadData->pScreenInfo->TextInfo->GetCursor()->GetPosition().Y;
                        CurrentPosition.X = (SHORT)(CurrentPosition.X -
                                                    RetrieveNumberOfSpaces(pCookedReadData->OriginalCursorPosition.X,
                                                                           pCookedReadData->BackupLimit,
                                                                           pCookedReadData->CurrentPosition));
                        if (CheckBisectProcessW(pCookedReadData->pScreenInfo,
                                                pCookedReadData->BackupLimit,
                                                pCookedReadData->CurrentPosition + 2,
                                                pCookedReadData->pScreenInfo->ScreenBufferSize.X - pCookedReadData->OriginalCursorPosition.X,
                                                pCookedReadData->OriginalCursorPosition.X,
                                                TRUE))
                        {
                            if ((CurrentPosition.X == -2) || (CurrentPosition.X == -1))
                            {
                                CurrentPosition.X--;
                            }
                        }

                        UpdateCursorPosition = TRUE;
                    }
                }
                break;
            case VK_RIGHT:
            case VK_F1:
                // we don't need to check for end of buffer here because we've
                // already done it.
                if (dwKeyState & (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED))
                {
                    if (wch != VK_F1)
                    {
                        if (pCookedReadData->CurrentPosition < (pCookedReadData->BytesRead / sizeof(WCHAR)))
                        {
                            PWCHAR NextWord = pCookedReadData->BufPtr;

                            if (!g_ciConsoleInformation.GetExtendedEditKey())
                            {
                                SHORT i;

                                for (i = (SHORT)(pCookedReadData->CurrentPosition); i < (SHORT)((pCookedReadData->BytesRead - 1) / sizeof(WCHAR)); i++)
                                {
                                    if (IS_WORD_DELIM(*NextWord))
                                    {
                                        i++;
                                        NextWord++;
                                        while ((i < (SHORT)((pCookedReadData->BytesRead - 1) / sizeof(WCHAR))) && IS_WORD_DELIM(*NextWord))
                                        {
                                            i++;
                                            NextWord++;
                                        }

                                        break;
                                    }

                                    NextWord++;
                                }
                            }
                            else
                            {
                                // A bit better word skipping.
                                PWCHAR BufLast = pCookedReadData->BackupLimit + pCookedReadData->BytesRead / sizeof(WCHAR);

                                ASSERT(NextWord < BufLast);
                                if (*NextWord == L' ')
                                {
                                    // If the current character is space, skip to the next non-space character.
                                    while (NextWord < BufLast)
                                    {
                                        if (*NextWord != L' ')
                                        {
                                            break;
                                        }
                                        ++NextWord;
                                    }
                                }
                                else
                                {
                                    // Skip the body part.
                                    BOOL fStartFromDelim = IS_WORD_DELIM(*NextWord);

                                    while (++NextWord < BufLast)
                                    {
                                        if (fStartFromDelim != IS_WORD_DELIM(*NextWord))
                                        {
                                            break;
                                        }
                                    }

                                    // Skip the space block.
                                    if (NextWord < BufLast && *NextWord == L' ')
                                    {
                                        while (++NextWord < BufLast)
                                        {
                                            if (*NextWord != L' ')
                                            {
                                                break;
                                            }
                                        }
                                    }
                                }
                            }

                            pCookedReadData->BufPtr = NextWord;
                            pCookedReadData->CurrentPosition = (ULONG) (pCookedReadData->BufPtr - pCookedReadData->BackupLimit);
                            CurrentPosition = pCookedReadData->OriginalCursorPosition;
                            CurrentPosition.X = (SHORT)(CurrentPosition.X +
                                                        RetrieveTotalNumberOfSpaces(pCookedReadData->OriginalCursorPosition.X,
                                                                                    pCookedReadData->BackupLimit,
                                                                                    pCookedReadData->CurrentPosition));
                            if (CheckBisectStringW(pCookedReadData->BackupLimit,
                                                   pCookedReadData->CurrentPosition + 1,
                                                   pCookedReadData->pScreenInfo->ScreenBufferSize.X - pCookedReadData->OriginalCursorPosition.X))
                            {
                                CurrentPosition.X++;
                            }
                            UpdateCursorPosition = TRUE;
                        }
                    }
                }
                else
                {
                    // If not at the end of the line, move cursor position right.
                    if (pCookedReadData->CurrentPosition < (pCookedReadData->BytesRead / sizeof(WCHAR)))
                    {
                        CurrentPosition = pCookedReadData->pScreenInfo->TextInfo->GetCursor()->GetPosition();
                        CurrentPosition.X = (SHORT)(CurrentPosition.X +
                                                    RetrieveNumberOfSpaces(pCookedReadData->OriginalCursorPosition.X,
                                                                           pCookedReadData->BackupLimit,
                                                                           pCookedReadData->CurrentPosition));
                        if (CheckBisectProcessW(pCookedReadData->pScreenInfo,
                                                pCookedReadData->BackupLimit,
                                                pCookedReadData->CurrentPosition + 2,
                                                pCookedReadData->pScreenInfo->ScreenBufferSize.X - pCookedReadData->OriginalCursorPosition.X,
                                                pCookedReadData->OriginalCursorPosition.X,
                                                TRUE))
                        {
                            if (CurrentPosition.X == (pCookedReadData->pScreenInfo->ScreenBufferSize.X - 1))
                                CurrentPosition.X++;
                        }

                        pCookedReadData->BufPtr++;
                        pCookedReadData->CurrentPosition++;
                        UpdateCursorPosition = TRUE;

                        // if at the end of the line, copy a character from the same position in the last command
                    }
                    else if (pCookedReadData->CommandHistory)
                    {
                        PCOMMAND LastCommand;
                        DWORD NumSpaces;
                        LastCommand = GetLastCommand(pCookedReadData->CommandHistory);
                        if (LastCommand && (USHORT) (LastCommand->CommandLength / sizeof(WCHAR)) > (USHORT) pCookedReadData->CurrentPosition)
                        {
                            *pCookedReadData->BufPtr = LastCommand->Command[pCookedReadData->CurrentPosition];
                            pCookedReadData->BytesRead += sizeof(WCHAR);
                            pCookedReadData->CurrentPosition++;
                            if (pCookedReadData->Echo)
                            {
                                CharsToWrite = sizeof(WCHAR);
                                Status = WriteCharsLegacy(pCookedReadData->pScreenInfo,
                                                          pCookedReadData->BackupLimit,
                                                          pCookedReadData->BufPtr,
                                                          pCookedReadData->BufPtr,
                                                          &CharsToWrite,
                                                          &NumSpaces,
                                                          pCookedReadData->OriginalCursorPosition.X,
                                                          WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                                          &ScrollY);
                                ASSERT(NT_SUCCESS(Status));
                                pCookedReadData->OriginalCursorPosition.Y += ScrollY;
                                pCookedReadData->NumberOfVisibleChars += NumSpaces;
                            }
                            pCookedReadData->BufPtr += 1;
                        }
                    }
                }
                break;

            case VK_F2:
                // copy the previous command to the current command, up to but
                // not including the character specified by the user.  the user
                // is prompted via popup to enter a character.
                if (pCookedReadData->CommandHistory)
                {
                    COORD PopupSize;

                    PopupSize.X = COPY_TO_CHAR_PROMPT_LENGTH + 2;
                    PopupSize.Y = 1;
                    Status = BeginPopup(pCookedReadData->pScreenInfo, pCookedReadData->CommandHistory, PopupSize);
                    if (NT_SUCCESS(Status))
                    {
                        // CopyToCharPopup does EndPopup call
                        return CopyToCharPopup(pCookedReadData, pWaitReplyMessage, fWaitRoutine);
                    }
                }
                break;

            case VK_F3:
                // Copy the remainder of the previous command to the current command.
                if (pCookedReadData->CommandHistory)
                {
                    PCOMMAND LastCommand;
                    DWORD NumSpaces, cchCount;

                    LastCommand = GetLastCommand(pCookedReadData->CommandHistory);
                    if (LastCommand && (USHORT) (LastCommand->CommandLength / sizeof(WCHAR)) > (USHORT) pCookedReadData->CurrentPosition)
                    {
                        cchCount = (LastCommand->CommandLength / sizeof(WCHAR)) - pCookedReadData->CurrentPosition;

#pragma prefast(suppress:__WARNING_POTENTIAL_BUFFER_OVERFLOW_HIGH_PRIORITY, "This is fine")
                        memmove(pCookedReadData->BufPtr, &LastCommand->Command[pCookedReadData->CurrentPosition], cchCount * sizeof(WCHAR));
                        pCookedReadData->CurrentPosition += cchCount;
                        cchCount *= sizeof(WCHAR);
                        pCookedReadData->BytesRead = max(LastCommand->CommandLength, pCookedReadData->BytesRead);
                        if (pCookedReadData->Echo)
                        {
                            Status = WriteCharsLegacy(pCookedReadData->pScreenInfo,
                                                      pCookedReadData->BackupLimit,
                                                      pCookedReadData->BufPtr,
                                                      pCookedReadData->BufPtr,
                                                      &cchCount,
                                                      (PULONG) & NumSpaces,
                                                      pCookedReadData->OriginalCursorPosition.X,
                                                      WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                                      &ScrollY);
                            ASSERT(NT_SUCCESS(Status));
                            pCookedReadData->OriginalCursorPosition.Y += ScrollY;
                            pCookedReadData->NumberOfVisibleChars += NumSpaces;
                        }
                        pCookedReadData->BufPtr += cchCount / sizeof(WCHAR);
                    }

                }
                break;

            case VK_F4:
                // Delete the current command from cursor position to the
                // letter specified by the user. The user is prompted via
                // popup to enter a character.
                if (pCookedReadData->CommandHistory)
                {
                    COORD PopupSize;

                    PopupSize.X = COPY_FROM_CHAR_PROMPT_LENGTH + 2;
                    PopupSize.Y = 1;
                    Status = BeginPopup(pCookedReadData->pScreenInfo, pCookedReadData->CommandHistory, PopupSize);
                    if (NT_SUCCESS(Status))
                    {
                        // CopyFromCharPopup does EndPopup call
                        return CopyFromCharPopup(pCookedReadData, pWaitReplyMessage, fWaitRoutine);
                    }
                }
                break;
            case VK_F6:
            {
                // place a ctrl-z in the current command line
                DWORD NumSpaces = 0;

                *pCookedReadData->BufPtr = (WCHAR)0x1a;  // ctrl-z
                pCookedReadData->BytesRead += sizeof(WCHAR);
                pCookedReadData->CurrentPosition++;
                if (pCookedReadData->Echo)
                {
                    CharsToWrite = sizeof(WCHAR);
                    Status = WriteCharsLegacy(pCookedReadData->pScreenInfo,
                                              pCookedReadData->BackupLimit,
                                              pCookedReadData->BufPtr,
                                              pCookedReadData->BufPtr,
                                              &CharsToWrite,
                                              &NumSpaces,
                                              pCookedReadData->OriginalCursorPosition.X,
                                              WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                              &ScrollY);
                    ASSERT(NT_SUCCESS(Status));
                    pCookedReadData->OriginalCursorPosition.Y += ScrollY;
                    pCookedReadData->NumberOfVisibleChars += NumSpaces;
                }
                pCookedReadData->BufPtr += 1;
                break;
            }
            case VK_F7:
                if (dwKeyState & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED))
                {
                    if (pCookedReadData->CommandHistory)
                    {
                        EmptyCommandHistory(pCookedReadData->CommandHistory);
                        pCookedReadData->CommandHistory->Flags |= CLE_ALLOCATED;
                    }
                }
                break;

            case VK_F8:
                if (pCookedReadData->CommandHistory)
                {
                    SHORT i;

                    // Cycles through the stored commands that start with the characters in the current command.
                    i = FindMatchingCommand(pCookedReadData->CommandHistory,
                                            pCookedReadData->BackupLimit,
                                            pCookedReadData->CurrentPosition * sizeof(WCHAR),
                                            pCookedReadData->CommandHistory->LastDisplayed,
                                            0);
                    if (i != -1)
                    {
                        SHORT CurrentPos;
                        COORD CursorPosition;

                        // save cursor position
                        CurrentPos = (SHORT)pCookedReadData->CurrentPosition;
                        CursorPosition = pCookedReadData->pScreenInfo->TextInfo->GetCursor()->GetPosition();

                        DeleteCommandLine(pCookedReadData, TRUE);
                        Status = RetrieveNthCommand(pCookedReadData->CommandHistory,
                                                    i,
                                                    pCookedReadData->BackupLimit,
                                                    pCookedReadData->BufferSize,
                                                    &pCookedReadData->BytesRead);
                        ASSERT(pCookedReadData->BackupLimit == pCookedReadData->BufPtr);
                        if (pCookedReadData->Echo)
                        {
                            Status = WriteCharsLegacy(pCookedReadData->pScreenInfo,
                                                      pCookedReadData->BackupLimit,
                                                      pCookedReadData->BufPtr,
                                                      pCookedReadData->BufPtr,
                                                      &pCookedReadData->BytesRead,
                                                      &pCookedReadData->NumberOfVisibleChars,
                                                      pCookedReadData->OriginalCursorPosition.X,
                                                      WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                                      &ScrollY);
                            ASSERT(NT_SUCCESS(Status));
                            pCookedReadData->OriginalCursorPosition.Y += ScrollY;
                        }
                        CursorPosition.Y += ScrollY;

                        // restore cursor position
                        pCookedReadData->BufPtr = pCookedReadData->BackupLimit + CurrentPos;
                        pCookedReadData->CurrentPosition = CurrentPos;
                        Status = pCookedReadData->pScreenInfo->SetCursorPosition(CursorPosition, TRUE);
                        ASSERT(NT_SUCCESS(Status));
                    }
                }
                break;
            case VK_F9:
            {
                // prompt the user to enter the desired command number. copy that command to the command line.
                COORD PopupSize;

                if (pCookedReadData->CommandHistory &&
                    pCookedReadData->CommandHistory->NumberOfCommands &&
                    pCookedReadData->pScreenInfo->ScreenBufferSize.X >= MINIMUM_COMMAND_PROMPT_SIZE + 2)
                {   // 2 is for border
                    PopupSize.X = COMMAND_NUMBER_PROMPT_LENGTH + COMMAND_NUMBER_LENGTH;
                    PopupSize.Y = 1;
                    Status = BeginPopup(pCookedReadData->pScreenInfo, pCookedReadData->CommandHistory, PopupSize);
                    if (NT_SUCCESS(Status))
                    {
                        // CommandNumberPopup does EndPopup call
                        return CommandNumberPopup(pCookedReadData, pWaitReplyMessage, fWaitRoutine);
                    }
                }
                break;
            }
            case VK_F10:
                if (dwKeyState & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED))
                {
                    ClearAliases();
                }
                break;
            case VK_INSERT:
                pCookedReadData->InsertMode = !pCookedReadData->InsertMode;
                pCookedReadData->pScreenInfo->SetCursorDBMode((BOOLEAN) (pCookedReadData->InsertMode != g_ciConsoleInformation.GetInsertMode()));
                break;
            case VK_DELETE:
                if (!AT_EOL(pCookedReadData))
                {
                    COORD CursorPosition;

                    fStartFromDelim = IS_WORD_DELIM(*pCookedReadData->BufPtr);

del_repeat:
                    // save cursor position
                    CursorPosition = pCookedReadData->pScreenInfo->TextInfo->GetCursor()->GetPosition();

                    // Delete commandline.
#pragma prefast(suppress:__WARNING_BUFFER_OVERFLOW, "Not sure why prefast is getting confused here")
                    DeleteCommandLine(pCookedReadData, FALSE);

                    // Delete char.
                    pCookedReadData->BytesRead -= sizeof(WCHAR);
                    memmove(pCookedReadData->BufPtr,
                                  pCookedReadData->BufPtr + 1,
                                  pCookedReadData->BytesRead - (pCookedReadData->CurrentPosition * sizeof(WCHAR)));

                    {
                        PWCHAR buf = (PWCHAR)((PBYTE) pCookedReadData->BackupLimit + pCookedReadData->BytesRead);
                        *buf = (WCHAR)' ';
                    }

                    // Write commandline.
                    if (pCookedReadData->Echo)
                    {
                        Status = WriteCharsLegacy(pCookedReadData->pScreenInfo,
                                                  pCookedReadData->BackupLimit,
                                                  pCookedReadData->BackupLimit,
                                                  pCookedReadData->BackupLimit,
                                                  &pCookedReadData->BytesRead,
                                                  &pCookedReadData->NumberOfVisibleChars,
                                                  pCookedReadData->OriginalCursorPosition.X,
                                                  WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                                  nullptr);
                        ASSERT(NT_SUCCESS(Status));
                    }

                    // restore cursor position
                    if (CheckBisectProcessW(pCookedReadData->pScreenInfo,
                                            pCookedReadData->BackupLimit,
                                            pCookedReadData->CurrentPosition + 1,
                                            pCookedReadData->pScreenInfo->ScreenBufferSize.X - pCookedReadData->OriginalCursorPosition.X,
                                            pCookedReadData->OriginalCursorPosition.X,
                                            TRUE))
                    {
                        CursorPosition.X++;
                    }
                    CurrentPosition = CursorPosition;
                    if (pCookedReadData->Echo)
                    {
                        Status = AdjustCursorPosition(pCookedReadData->pScreenInfo, CurrentPosition, TRUE, nullptr);
                        ASSERT(NT_SUCCESS(Status));
                    }

                    // If Ctrl key is pressed, delete a word.
                    // If the start point was word delimiter, just remove delimiters portion only.
                    if ((dwKeyState & CTRL_PRESSED) && !AT_EOL(pCookedReadData) && fStartFromDelim ^ !IS_WORD_DELIM(*pCookedReadData->BufPtr))
                    {
                        goto del_repeat;
                    }
                }
                break;
            default:
                ASSERT(FALSE);
                break;
        }
    }

    if (UpdateCursorPosition && pCookedReadData->Echo)
    {
        Status = AdjustCursorPosition(pCookedReadData->pScreenInfo, CurrentPosition, TRUE, nullptr);
        ASSERT(NT_SUCCESS(Status));
    }

    return STATUS_SUCCESS;
}

PCOMMAND RemoveCommand(_In_ PCOMMAND_HISTORY CommandHistory, _In_ SHORT iDel)
{
    SHORT iFirst = CommandHistory->FirstCommand;
    SHORT iLast = CommandHistory->LastAdded;
    SHORT iDisp = CommandHistory->LastDisplayed;

    if (CommandHistory->NumberOfCommands == 0)
    {
        return nullptr;
    }

    SHORT const nDel = COMMAND_INDEX_TO_NUM(iDel, CommandHistory);
    if ((nDel < COMMAND_INDEX_TO_NUM(iFirst, CommandHistory)) || (nDel > COMMAND_INDEX_TO_NUM(iLast, CommandHistory)))
    {
        return nullptr;
    }

    if (iDisp == iDel)
    {
        CommandHistory->LastDisplayed = -1;
    }

    PCOMMAND* const ppcFirst = &(CommandHistory->Commands[iFirst]);
    PCOMMAND* const ppcDel = &(CommandHistory->Commands[iDel]);
    PCOMMAND const pcmdDel = *ppcDel;

    if (iDel < iLast)
    {
        memmove(ppcDel, ppcDel + 1, (iLast - iDel) * sizeof(PCOMMAND));
        if ((iDisp > iDel) && (iDisp <= iLast))
        {
            COMMAND_IND_DEC(iDisp, CommandHistory);
        }
        COMMAND_IND_DEC(iLast, CommandHistory);
    }
    else if (iFirst <= iDel)
    {
        memmove(ppcFirst + 1, ppcFirst, (iDel - iFirst) * sizeof(PCOMMAND));
        if ((iDisp >= iFirst) && (iDisp < iDel))
        {
            COMMAND_IND_INC(iDisp, CommandHistory);
        }
        COMMAND_IND_INC(iFirst, CommandHistory);
    }

    CommandHistory->FirstCommand = iFirst;
    CommandHistory->LastAdded = iLast;
    CommandHistory->LastDisplayed = iDisp;
    CommandHistory->NumberOfCommands--;
    return pcmdDel;
}


// Routine Description:
// - this routine finds the most recent command that starts with the letters already in the current command.  it returns the array index (no mod needed).
SHORT FindMatchingCommand(_In_ PCOMMAND_HISTORY CommandHistory,
                          _In_reads_bytes_(cbIn) PCWCHAR pwchIn,
                          _In_ ULONG cbIn,
                          _In_ SHORT CommandIndex,  // where to start from
                          _In_ DWORD Flags)
{
    if (CommandHistory->NumberOfCommands == 0)
    {
        return -1;
    }

    if (!(Flags & FMCFL_JUST_LOOKING) && (CommandHistory->Flags & CLE_RESET))
    {
        CommandHistory->Flags &= ~CLE_RESET;
    }
    else
    {
        COMMAND_IND_PREV(CommandIndex, CommandHistory);
    }

    if (cbIn == 0)
    {
        return CommandIndex;
    }

    for (SHORT i = 0; i < CommandHistory->NumberOfCommands; i++)
    {
        PCOMMAND pcmdT = CommandHistory->Commands[CommandIndex];

        if ((!(Flags & FMCFL_EXACT_MATCH) && (cbIn <= pcmdT->CommandLength)) || ((USHORT) cbIn == pcmdT->CommandLength))
        {
            if (!wcsncmp(pcmdT->Command, pwchIn, (USHORT) cbIn / sizeof(WCHAR)))
            {
                return CommandIndex;
            }
        }

        COMMAND_IND_PREV(CommandIndex, CommandHistory);
    }

    return -1;
}

void DrawCommandListBorder(_In_ PCLE_POPUP const Popup, _In_ PSCREEN_INFORMATION const ScreenInfo)
{
    // fill attributes of top line
    COORD WriteCoord;
    WriteCoord.X = Popup->Region.Left;
    WriteCoord.Y = Popup->Region.Top;
    ULONG Length = POPUP_SIZE_X(Popup) + 2;
    FillOutput(ScreenInfo, Popup->Attributes.GetLegacyAttributes(), WriteCoord, CONSOLE_ATTRIBUTE, &Length);

    // draw upper left corner
    Length = 1;
    FillOutput(ScreenInfo, ScreenInfo->LineChar[UPPER_LEFT_CORNER], WriteCoord, CONSOLE_REAL_UNICODE, &Length);

    // draw upper bar
    WriteCoord.X += 1;
    Length = POPUP_SIZE_X(Popup);
    FillOutput(ScreenInfo, ScreenInfo->LineChar[HORIZONTAL_LINE], WriteCoord, CONSOLE_REAL_UNICODE, &Length);

    // draw upper right corner
    WriteCoord.X = Popup->Region.Right;
    Length = 1;
    FillOutput(ScreenInfo, ScreenInfo->LineChar[UPPER_RIGHT_CORNER], WriteCoord, CONSOLE_REAL_UNICODE, &Length);

    for (SHORT i = 0; i < POPUP_SIZE_Y(Popup); i++)
    {
        WriteCoord.Y += 1;
        WriteCoord.X = Popup->Region.Left;

        // fill attributes
        Length = POPUP_SIZE_X(Popup) + 2;
        FillOutput(ScreenInfo, Popup->Attributes.GetLegacyAttributes(), WriteCoord, CONSOLE_ATTRIBUTE, &Length);
        Length = 1;
        FillOutput(ScreenInfo, ScreenInfo->LineChar[VERTICAL_LINE], WriteCoord, CONSOLE_REAL_UNICODE, &Length);
        WriteCoord.X = Popup->Region.Right;
        Length = 1;
        FillOutput(ScreenInfo, ScreenInfo->LineChar[VERTICAL_LINE], WriteCoord, CONSOLE_REAL_UNICODE, &Length);
    }

    // Draw bottom line.
    // Fill attributes of top line.
    WriteCoord.X = Popup->Region.Left;
    WriteCoord.Y = Popup->Region.Bottom;
    Length = POPUP_SIZE_X(Popup) + 2;
    FillOutput(ScreenInfo, Popup->Attributes.GetLegacyAttributes(), WriteCoord, CONSOLE_ATTRIBUTE, &Length);

    // Draw bottom left corner.
    Length = 1;
    WriteCoord.X = Popup->Region.Left;
    FillOutput(ScreenInfo, ScreenInfo->LineChar[BOTTOM_LEFT_CORNER], WriteCoord, CONSOLE_REAL_UNICODE, &Length);

    // Draw lower bar.
    WriteCoord.X += 1;
    Length = POPUP_SIZE_X(Popup);
    FillOutput(ScreenInfo, ScreenInfo->LineChar[HORIZONTAL_LINE], WriteCoord, CONSOLE_REAL_UNICODE, &Length);

    // draw lower right corner
    WriteCoord.X = Popup->Region.Right;
    Length = 1;
    FillOutput(ScreenInfo, ScreenInfo->LineChar[BOTTOM_RIGHT_CORNER], WriteCoord, CONSOLE_REAL_UNICODE, &Length);
}

void UpdateHighlight(_In_ PCLE_POPUP Popup,
                     _In_ SHORT OldCurrentCommand, // command number, not index
                     _In_ SHORT NewCurrentCommand,
                     _In_ PSCREEN_INFORMATION ScreenInfo)
{
    SHORT TopIndex;
    if (Popup->BottomIndex < POPUP_SIZE_Y(Popup))
    {
        TopIndex = 0;
    }
    else
    {
        TopIndex = (SHORT)(Popup->BottomIndex - POPUP_SIZE_Y(Popup) + 1);
    }
    const WORD PopupLegacyAttributes = Popup->Attributes.GetLegacyAttributes();
    COORD WriteCoord;
    WriteCoord.X = (SHORT)(Popup->Region.Left + 1);
    ULONG lStringLength = POPUP_SIZE_X(Popup);

    WriteCoord.Y = (SHORT)(Popup->Region.Top + 1 + OldCurrentCommand - TopIndex);
    FillOutput(ScreenInfo, PopupLegacyAttributes, WriteCoord, CONSOLE_ATTRIBUTE, &lStringLength);

    // highlight new command
    WriteCoord.Y = (SHORT)(Popup->Region.Top + 1 + NewCurrentCommand - TopIndex);

    // inverted attributes
    WORD const Attributes = (WORD) (((PopupLegacyAttributes << 4) & 0xf0) | ((PopupLegacyAttributes >> 4) & 0x0f));
    FillOutput(ScreenInfo, Attributes, WriteCoord, CONSOLE_ATTRIBUTE, &lStringLength);
}

void DrawCommandListPopup(_In_ PCLE_POPUP const Popup,
                          _In_ SHORT const CurrentCommand,
                          _In_ PCOMMAND_HISTORY const CommandHistory,
                          _In_ PSCREEN_INFORMATION const ScreenInfo)
{
    // draw empty popup
    COORD WriteCoord;
    WriteCoord.X = (SHORT)(Popup->Region.Left + 1);
    WriteCoord.Y = (SHORT)(Popup->Region.Top + 1);
    ULONG lStringLength = POPUP_SIZE_X(Popup);
    for (SHORT i = 0; i < POPUP_SIZE_Y(Popup); ++i)
    {
        FillOutput(ScreenInfo, Popup->Attributes.GetLegacyAttributes(), WriteCoord, CONSOLE_ATTRIBUTE, &lStringLength);
        FillOutput(ScreenInfo, (WCHAR)' ', WriteCoord, CONSOLE_FALSE_UNICODE,   // faster than real unicode
                   &lStringLength);
        WriteCoord.Y += 1;
    }

    WriteCoord.Y = (SHORT)(Popup->Region.Top + 1);
    SHORT i = max((SHORT)(Popup->BottomIndex - POPUP_SIZE_Y(Popup) + 1), 0);
    for (; i <= Popup->BottomIndex; i++)
    {
        CHAR CommandNumber[COMMAND_NUMBER_SIZE];
        // Write command number to screen.
        if (0 != _itoa_s(i, CommandNumber, ARRAYSIZE(CommandNumber), 10))
        {
            return;
        }

        PCHAR CommandNumberPtr = CommandNumber;

        size_t CommandNumberLength;
        if (FAILED(StringCchLengthA(CommandNumberPtr, ARRAYSIZE(CommandNumber), &CommandNumberLength)))
        {
            return;
        }
        __assume_bound(CommandNumberLength);

        if (CommandNumberLength + 1 >= ARRAYSIZE(CommandNumber))
        {
            return;
        }

        CommandNumber[CommandNumberLength] = ':';
        CommandNumber[CommandNumberLength + 1] = ' ';
        CommandNumberLength += 2;
        if (CommandNumberLength > (ULONG) POPUP_SIZE_X(Popup))
        {
            CommandNumberLength = (ULONG) POPUP_SIZE_X(Popup);
        }

        WriteCoord.X = (SHORT)(Popup->Region.Left + 1);
        WriteOutputString(ScreenInfo, CommandNumberPtr, WriteCoord, CONSOLE_ASCII, (PULONG) & CommandNumberLength, nullptr);

        // write command to screen
        lStringLength = CommandHistory->Commands[COMMAND_NUM_TO_INDEX(i, CommandHistory)]->CommandLength / sizeof(WCHAR);
        {
            DWORD lTmpStringLength = lStringLength;
            LONG lPopupLength = (LONG) (POPUP_SIZE_X(Popup) - CommandNumberLength);
            LPWSTR lpStr = CommandHistory->Commands[COMMAND_NUM_TO_INDEX(i, CommandHistory)]->Command;
            while (lTmpStringLength--)
            {
                if (IsCharFullWidth(*lpStr++))
                {
                    lPopupLength -= 2;
                }
                else
                {
                    lPopupLength--;
                }

                if (lPopupLength <= 0)
                {
                    lStringLength -= lTmpStringLength;
                    if (lPopupLength < 0)
                    {
                        lStringLength--;
                    }

                    break;
                }
            }
        }

        WriteCoord.X = (SHORT)(WriteCoord.X + CommandNumberLength);
        {
            PWCHAR TransBuffer;

            TransBuffer = new WCHAR[lStringLength];
            if (TransBuffer == nullptr)
            {
                return;
            }

            memmove(TransBuffer, CommandHistory->Commands[COMMAND_NUM_TO_INDEX(i, CommandHistory)]->Command, lStringLength * sizeof(WCHAR));
            WriteOutputString(ScreenInfo, TransBuffer, WriteCoord, CONSOLE_REAL_UNICODE, &lStringLength, nullptr);
            delete[] TransBuffer;
        }

        // write attributes to screen
        if (COMMAND_NUM_TO_INDEX(i, CommandHistory) == CurrentCommand)
        {
            WriteCoord.X = (SHORT)(Popup->Region.Left + 1);
            WORD PopupLegacyAttributes = Popup->Attributes.GetLegacyAttributes();
            // inverted attributes
            WORD const Attributes = (WORD) (((PopupLegacyAttributes << 4) & 0xf0) | ((PopupLegacyAttributes >> 4) & 0x0f));
            lStringLength = POPUP_SIZE_X(Popup);
            FillOutput(ScreenInfo, Attributes, WriteCoord, CONSOLE_ATTRIBUTE, &lStringLength);
        }

        WriteCoord.Y += 1;
    }
}

void UpdateCommandListPopup(_In_ SHORT Delta,
                            _Inout_ PSHORT CurrentCommand,   // real index, not command #
                            _In_ PCOMMAND_HISTORY const CommandHistory,
                            _In_ PCLE_POPUP Popup,
                            _In_ PSCREEN_INFORMATION const ScreenInfo,
                            _In_ DWORD const Flags)
{
    if (Delta == 0)
    {
        return;
    }
    SHORT const Size = POPUP_SIZE_Y(Popup);

    SHORT CurCmdNum;
    SHORT NewCmdNum;

    if (Flags & UCLP_WRAP)
    {
        CurCmdNum = *CurrentCommand;
        NewCmdNum = CurCmdNum + Delta;
        NewCmdNum = COMMAND_INDEX_TO_NUM(NewCmdNum, CommandHistory);
        CurCmdNum = COMMAND_INDEX_TO_NUM(CurCmdNum, CommandHistory);
    }
    else
    {
        CurCmdNum = COMMAND_INDEX_TO_NUM(*CurrentCommand, CommandHistory);
        NewCmdNum = CurCmdNum + Delta;
        if (NewCmdNum >= CommandHistory->NumberOfCommands)
        {
            NewCmdNum = (SHORT)(CommandHistory->NumberOfCommands - 1);
        }
        else if (NewCmdNum < 0)
        {
            NewCmdNum = 0;
        }
    }
    Delta = NewCmdNum - CurCmdNum;

    BOOL Scroll = FALSE;
    // determine amount to scroll, if any
    if (NewCmdNum <= Popup->BottomIndex - Size)
    {
        Popup->BottomIndex += Delta;
        if (Popup->BottomIndex < (SHORT)(Size - 1))
        {
            Popup->BottomIndex = (SHORT)(Size - 1);
        }
        Scroll = TRUE;
    }
    else if (NewCmdNum > Popup->BottomIndex)
    {
        Popup->BottomIndex += Delta;
        if (Popup->BottomIndex >= CommandHistory->NumberOfCommands)
        {
            Popup->BottomIndex = (SHORT)(CommandHistory->NumberOfCommands - 1);
        }
        Scroll = TRUE;
    }

    // write commands to popup
    if (Scroll)
    {
        DrawCommandListPopup(Popup, COMMAND_NUM_TO_INDEX(NewCmdNum, CommandHistory), CommandHistory, ScreenInfo);
    }
    else
    {
        UpdateHighlight(Popup, COMMAND_INDEX_TO_NUM((*CurrentCommand), CommandHistory), NewCmdNum, ScreenInfo);
    }

    *CurrentCommand = COMMAND_NUM_TO_INDEX(NewCmdNum, CommandHistory);
}

// Routine Description:
// - This routine marks the command history buffer freed.
// Arguments:
// - Console - pointer to console.
// - ProcessHandle - handle to client process.
// Return Value:
// - <none>
PCOMMAND_HISTORY FindCommandHistory(_In_ const HANDLE hProcess)
{
    PLIST_ENTRY const ListHead = &g_ciConsoleInformation.CommandHistoryList;
    PLIST_ENTRY ListNext = ListHead->Flink;
    while (ListNext != ListHead)
    {
        PCOMMAND_HISTORY const History = CONTAINING_RECORD(ListNext, COMMAND_HISTORY, ListLink);
        ListNext = ListNext->Flink;
        if (History->ProcessHandle == hProcess)
        {
            ASSERT(History->Flags & CLE_ALLOCATED);
            return History;
        }
    }

    return nullptr;
}

// Routine Description:
// - This routine marks the command history buffer freed.
// Arguments:
// - hProcess - handle to client process.
// Return Value:
// - <none>
void FreeCommandHistory(_In_ HANDLE const hProcess)
{
    PCOMMAND_HISTORY const History = FindCommandHistory(hProcess);
    if (History)
    {
        History->Flags &= ~CLE_ALLOCATED;
        History->ProcessHandle = nullptr;
    }
}

void FreeCommandHistoryBuffers()
{
    PLIST_ENTRY const ListHead = &g_ciConsoleInformation.CommandHistoryList;
    PLIST_ENTRY ListNext = ListHead->Flink;
    while (ListNext != ListHead)
    {
        PCOMMAND_HISTORY History = CONTAINING_RECORD(ListNext, COMMAND_HISTORY, ListLink);
        ListNext = ListNext->Flink;

        if (History)
        {

            RemoveEntryList(&History->ListLink);
            if (History->AppName)
            {
                delete[] History->AppName;
                History->AppName = nullptr;
            }

            for (SHORT i = 0; i < History->NumberOfCommands; i++)
            {
                delete[] History->Commands[i];
                History->Commands[i] = nullptr;
            }

            delete[] History;
            History = nullptr;
        }
    }
}

void ResizeCommandHistoryBuffers(_In_ UINT const cCommands)
{
    ASSERT(cCommands <= SHORT_MAX);
    g_ciConsoleInformation.SetHistoryBufferSize(cCommands);

    PLIST_ENTRY const ListHead = &g_ciConsoleInformation.CommandHistoryList;
    PLIST_ENTRY ListNext = ListHead->Flink;
    while (ListNext != ListHead)
    {
        PCOMMAND_HISTORY const History = CONTAINING_RECORD(ListNext, COMMAND_HISTORY, ListLink);
        ListNext = ListNext->Flink;

        PCOMMAND_HISTORY const NewHistory = ReallocCommandHistory(History, cCommands);
        PCOOKED_READ_DATA const CookedReadData = g_ciConsoleInformation.lpCookedReadData;
        if (CookedReadData && CookedReadData->CommandHistory == History)
        {
            CookedReadData->CommandHistory = NewHistory;
        }
    }
}



// Routine Description:
// - This routine is called when escape is entered or a command is added.
void ResetCommandHistory(_In_opt_ PCOMMAND_HISTORY CommandHistory)
{
    if (CommandHistory == nullptr)
    {
        return;
    }
    CommandHistory->LastDisplayed = CommandHistory->LastAdded;
    CommandHistory->Flags |= CLE_RESET;
}

NTSTATUS AddCommand(_In_ PCOMMAND_HISTORY pCmdHistory,
                    _In_reads_bytes_(cbCommand) PCWCHAR pwchCommand,
                    _In_ const USHORT cbCommand,
                    _In_ const BOOL fHistoryNoDup)
{
    if (pCmdHistory == nullptr || pCmdHistory->MaximumNumberOfCommands == 0)
    {
        return STATUS_NO_MEMORY;
    }

    ASSERT(pCmdHistory->Flags & CLE_ALLOCATED);

    if (cbCommand == 0)
    {
        return STATUS_SUCCESS;
    }

    if (pCmdHistory->NumberOfCommands == 0 ||
        pCmdHistory->Commands[pCmdHistory->LastAdded]->CommandLength != cbCommand ||
        memcmp(pCmdHistory->Commands[pCmdHistory->LastAdded]->Command, pwchCommand, cbCommand))
    {

        PCOMMAND pCmdReuse = nullptr;

        if (fHistoryNoDup)
        {
            SHORT i;
            i = FindMatchingCommand(pCmdHistory, pwchCommand, cbCommand, pCmdHistory->LastDisplayed, FMCFL_EXACT_MATCH);
            if (i != -1)
            {
                pCmdReuse = RemoveCommand(pCmdHistory, i);
            }
        }

        // find free record.  if all records are used, free the lru one.
        if (pCmdHistory->NumberOfCommands < pCmdHistory->MaximumNumberOfCommands)
        {
            pCmdHistory->LastAdded += 1;
            pCmdHistory->NumberOfCommands++;
        }
        else
        {
            COMMAND_IND_INC(pCmdHistory->LastAdded, pCmdHistory);
            COMMAND_IND_INC(pCmdHistory->FirstCommand, pCmdHistory);
            delete[] pCmdHistory->Commands[pCmdHistory->LastAdded];
            if (pCmdHistory->LastDisplayed == pCmdHistory->LastAdded)
            {
                pCmdHistory->LastDisplayed = -1;
            }
        }

        // TODO: Fix Commands history accesses. See: http://osgvsowi/614402
        if (pCmdHistory->LastDisplayed == -1 ||
            pCmdHistory->Commands[pCmdHistory->LastDisplayed]->CommandLength != cbCommand ||
            memcmp(pCmdHistory->Commands[pCmdHistory->LastDisplayed]->Command, pwchCommand, cbCommand))
        {
            ResetCommandHistory(pCmdHistory);
        }

        // add command to array
        PCOMMAND* const ppCmd = &pCmdHistory->Commands[pCmdHistory->LastAdded];
        if (pCmdReuse)
        {
            *ppCmd = pCmdReuse;
        }
        else
        {
            *ppCmd = (PCOMMAND) new BYTE[cbCommand + sizeof(COMMAND)];
            if (*ppCmd == nullptr)
            {
                COMMAND_IND_PREV(pCmdHistory->LastAdded, pCmdHistory);
                pCmdHistory->NumberOfCommands -= 1;
                return STATUS_NO_MEMORY;
            }
            (*ppCmd)->CommandLength = cbCommand;
            memmove((*ppCmd)->Command, pwchCommand, cbCommand);
        }
    }
    pCmdHistory->Flags |= CLE_RESET; // remember that we've returned a cmd
    return STATUS_SUCCESS;
}

NTSTATUS RetrieveCommand(_In_ PCOMMAND_HISTORY CommandHistory,
                         _In_ WORD VirtualKeyCode,
                         _In_reads_bytes_(BufferSize) PWCHAR Buffer,
                         _In_ ULONG BufferSize,
                         _Out_ PULONG CommandSize)
{
    if (CommandHistory == nullptr)
    {
        return STATUS_UNSUCCESSFUL;
    }

    ASSERT(CommandHistory->Flags & CLE_ALLOCATED);

    if (CommandHistory->NumberOfCommands == 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (CommandHistory->NumberOfCommands == 1)
    {
        CommandHistory->LastDisplayed = 0;
    }
    else if (VirtualKeyCode == VK_UP)
    {
        // if this is the first time for this read that a command has
        // been retrieved, return the current command.  otherwise, return
        // the previous command.
        if (CommandHistory->Flags & CLE_RESET)
        {
            CommandHistory->Flags &= ~CLE_RESET;
        }
        else
        {
            COMMAND_IND_PREV(CommandHistory->LastDisplayed, CommandHistory);
        }
    }
    else
    {
        COMMAND_IND_NEXT(CommandHistory->LastDisplayed, CommandHistory);
    }

    return RetrieveNthCommand(CommandHistory, CommandHistory->LastDisplayed, Buffer, BufferSize, CommandSize);
}

NTSTATUS SrvGetConsoleTitle(_Inout_ PCONSOLE_API_MSG m, _In_opt_ PBOOL const /*ReplyPending*/)
{
    PCONSOLE_GETTITLE_MSG const a = &m->u.consoleMsgL2.GetConsoleTitle;

    Telemetry::Instance().LogApiCall(a->Original ? Telemetry::ApiCall::GetConsoleOriginalTitle : Telemetry::ApiCall::GetConsoleTitle, a->Unicode);

    PVOID Buffer;
    NTSTATUS Status = NTSTATUS_FROM_HRESULT(m->GetOutputBuffer(&Buffer, &a->TitleLength));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    CONSOLE_INFORMATION *Console;
    Status = RevalidateConsole(&Console);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    LPWSTR Title;
    ULONG TitleLength;
    if (a->Original)
    {
        Title = g_ciConsoleInformation.OriginalTitle;
        TitleLength = (ULONG) wcslen(g_ciConsoleInformation.OriginalTitle);
    }
    else
    {
        Title = g_ciConsoleInformation.Title;
        TitleLength = (ULONG) wcslen(g_ciConsoleInformation.Title);
    }

    // a->TitleLength contains length in bytes.
    if (a->Unicode)
    {
        StringCbCopyW((PWSTR) Buffer, a->TitleLength, Title);
        SetReplyInformation((PCONSOLE_API_MSG) m, (wcslen((PWSTR) Buffer) + 1) * sizeof(WCHAR));
        a->TitleLength = TitleLength;
    }
    else
    {
        a->TitleLength = (USHORT) ConvertToOem(g_uiOEMCP, Title, TitleLength, (LPSTR) Buffer, a->TitleLength);
        ((char *)Buffer)[a->TitleLength] = '\0';
        SetReplyInformation(m, a->TitleLength + 1);
    }

    UnlockConsole();
    return STATUS_SUCCESS;
}

NTSTATUS SrvSetConsoleTitle(_Inout_ PCONSOLE_API_MSG m, _In_opt_ PBOOL const /*ReplyPending*/)
{
    PCONSOLE_SETTITLE_MSG const a = &m->u.consoleMsgL2.SetConsoleTitle;

    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::SetConsoleTitle, a->Unicode);

    PVOID Buffer;
    ULONG cbOriginalLength;

    NTSTATUS Status = NTSTATUS_FROM_HRESULT(m->GetInputBuffer(&Buffer, &cbOriginalLength));
    if (NT_SUCCESS(Status))
    {
        CONSOLE_INFORMATION *Console;
        Status = RevalidateConsole(&Console);
        if (NT_SUCCESS(Status))
        {
            Status = DoSrvSetConsoleTitle(Buffer, cbOriginalLength, a->Unicode);
            UnlockConsole();
        }
    }

    return Status;
}

NTSTATUS DoSrvSetConsoleTitle(_In_ PVOID const Buffer, _In_ ULONG const cbOriginalLength, _In_ BOOLEAN const fUnicode)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG cbTitleLength;
    if (fUnicode)
    {
        if (FAILED(ULongAdd(cbOriginalLength, sizeof(WCHAR), &cbTitleLength)))
        {
            Status = STATUS_UNSUCCESSFUL;
        }
    }
    else
    {
        if (FAILED(ULongMult(cbOriginalLength, sizeof(WCHAR), &cbTitleLength)))
        {
            Status = STATUS_UNSUCCESSFUL;
        }

        if (NT_SUCCESS(Status))
        {
            if (FAILED(ULongAdd(cbTitleLength, sizeof(WCHAR), &cbTitleLength)))
            {
                Status = STATUS_UNSUCCESSFUL;
            }
        }
    }

    if (NT_SUCCESS(Status))
    {
        // Length is in bytes. Add 1 so dividing by WCHAR (2) is always rounding up.
        LPWSTR const NewTitle = new WCHAR[(cbTitleLength + 1) / sizeof(WCHAR)];
        if (NewTitle != nullptr)
        {
            if (cbOriginalLength == 0)
            {
                NewTitle[0] = L'\0';
            }
            else
            {
                if (!fUnicode)
                {
                    // convert title to unicode
                    cbTitleLength = (USHORT) ConvertInputToUnicode(g_uiOEMCP, (LPSTR) Buffer, cbOriginalLength, NewTitle, cbOriginalLength);
                    // ConvertInputToUnicode doesn't guarantee nullptr-termination.
                    NewTitle[cbTitleLength] = 0;
                }
                else
                {
                    memmove(NewTitle, Buffer, cbOriginalLength);
                    NewTitle[cbOriginalLength / sizeof(WCHAR)] = 0;
                }
            }
            delete[] g_ciConsoleInformation.Title;
            g_ciConsoleInformation.Title = NewTitle;

            if(g_ciConsoleInformation.pWindow->PostUpdateTitleWithCopy(g_ciConsoleInformation.Title))
            {
                Status = STATUS_SUCCESS;
            }
            else
            {
                Status = STATUS_UNSUCCESSFUL;
            }
        }
        else
        {
            Status = STATUS_NO_MEMORY;
        }
    }
    else
    {
        Status = STATUS_INVALID_PARAMETER;
    }

    return Status;
}

UINT LoadStringEx(_In_ HINSTANCE hModule, _In_ UINT wID, _Out_writes_(cchBufferMax) LPWSTR lpBuffer, _In_ UINT cchBufferMax, _In_ WORD wLangId)
{
    // Make sure the parms are valid.
    if (lpBuffer == nullptr)
    {
        return 0;
    }

    UINT cch = 0;

    // String Tables are broken up into 16 string segments.  Find the segment containing the string we are interested in.
    HANDLE const hResInfo = FindResourceEx(hModule, RT_STRING, (LPTSTR) ((LONG_PTR) (((USHORT) wID >> 4) + 1)), wLangId);
    if (hResInfo != nullptr)
    {
        // Load that segment.
        HANDLE const hStringSeg = (HRSRC) LoadResource(hModule, (HRSRC) hResInfo);

        // Lock the resource.
        LPTSTR lpsz;
        if (hStringSeg != nullptr && (lpsz = (LPTSTR) LockResource(hStringSeg)) != nullptr)
        {
            // Move past the other strings in this segment. (16 strings in a segment -> & 0x0F)
            wID &= 0x0F;
            for (;;)
            {
                cch = *((WCHAR *)lpsz++);   // PASCAL like string count
                // first WCHAR is count of WCHARs
                if (wID-- == 0)
                {
                    break;
                }

                lpsz += cch;    // Step to start if next string
            }

            // chhBufferMax == 0 means return a pointer to the read-only resource buffer.
            if (cchBufferMax == 0)
            {
                *(LPTSTR *) lpBuffer = lpsz;
            }
            else
            {
                // Account for the nullptr
                cchBufferMax--;

                // Don't copy more than the max allowed.
                if (cch > cchBufferMax)
                    cch = cchBufferMax;

                // Copy the string into the buffer.
                memmove(lpBuffer, lpsz, cch * sizeof(WCHAR));
            }
        }
    }

    // Append a nullptr.
    if (cchBufferMax != 0)
    {
        lpBuffer[cch] = 0;
    }

    return cch;
}
