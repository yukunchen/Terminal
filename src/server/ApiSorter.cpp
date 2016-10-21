/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "ApiSorter.h"

#include "..\host\directio.h"
#include "..\host\getset.h"
#include "..\host\stream.h"
#include "..\host\srvinit.h"

#define CONSOLE_API_STRUCT(Routine, Struct) { Routine, sizeof(Struct) }
#define CONSOLE_API_NO_PARAMETER(Routine) { Routine, 0 }

typedef struct _CONSOLE_API_DESCRIPTOR
{
    PCONSOLE_API_ROUTINE Routine;
    ULONG RequiredSize;
} CONSOLE_API_DESCRIPTOR, *PCONSOLE_API_DESCRIPTOR;

typedef struct _CONSOLE_API_LAYER_DESCRIPTOR
{
    const CONSOLE_API_DESCRIPTOR *Descriptor;
    ULONG Count;
} CONSOLE_API_LAYER_DESCRIPTOR, *PCONSOLE_API_LAYER_DESCRIPTOR;

NTSTATUS SrvDeprecatedAPI(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL ReplyPending);

const CONSOLE_API_DESCRIPTOR ConsoleApiLayer1[] = {
    CONSOLE_API_STRUCT(SrvGetConsoleCP, CONSOLE_GETCP_MSG),
    CONSOLE_API_STRUCT(SrvGetConsoleMode, CONSOLE_MODE_MSG),
    CONSOLE_API_STRUCT(SrvSetConsoleMode, CONSOLE_MODE_MSG),
    CONSOLE_API_STRUCT(SrvGetConsoleNumberOfInputEvents, CONSOLE_GETNUMBEROFINPUTEVENTS_MSG),
    CONSOLE_API_STRUCT(SrvGetConsoleInput, CONSOLE_GETCONSOLEINPUT_MSG),
    CONSOLE_API_STRUCT(SrvReadConsole, CONSOLE_READCONSOLE_MSG),
    CONSOLE_API_STRUCT(SrvWriteConsole, CONSOLE_WRITECONSOLE_MSG),
    CONSOLE_API_NO_PARAMETER(SrvDeprecatedAPI), // SrvConsoleNotifyLastClose
    CONSOLE_API_STRUCT(SrvGetConsoleLangId, CONSOLE_LANGID_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_MAPBITMAP_MSG),
};

const CONSOLE_API_DESCRIPTOR ConsoleApiLayer2[] = {
    CONSOLE_API_STRUCT(SrvFillConsoleOutput, CONSOLE_FILLCONSOLEOUTPUT_MSG),
    CONSOLE_API_STRUCT(SrvGenerateConsoleCtrlEvent, CONSOLE_CTRLEVENT_MSG),
    CONSOLE_API_NO_PARAMETER(SrvSetConsoleActiveScreenBuffer),
    CONSOLE_API_NO_PARAMETER(SrvFlushConsoleInputBuffer),
    CONSOLE_API_STRUCT(SrvSetConsoleCP, CONSOLE_SETCP_MSG),
    CONSOLE_API_STRUCT(SrvGetConsoleCursorInfo, CONSOLE_GETCURSORINFO_MSG),
    CONSOLE_API_STRUCT(SrvSetConsoleCursorInfo, CONSOLE_SETCURSORINFO_MSG),
    CONSOLE_API_STRUCT(SrvGetConsoleScreenBufferInfo, CONSOLE_SCREENBUFFERINFO_MSG),
    CONSOLE_API_STRUCT(SrvSetScreenBufferInfo, CONSOLE_SCREENBUFFERINFO_MSG),
    CONSOLE_API_STRUCT(SrvSetConsoleScreenBufferSize, CONSOLE_SETSCREENBUFFERSIZE_MSG),
    CONSOLE_API_STRUCT(SrvSetConsoleCursorPosition, CONSOLE_SETCURSORPOSITION_MSG),
    CONSOLE_API_STRUCT(SrvGetLargestConsoleWindowSize, CONSOLE_GETLARGESTWINDOWSIZE_MSG),
    CONSOLE_API_STRUCT(SrvScrollConsoleScreenBuffer, CONSOLE_SCROLLSCREENBUFFER_MSG),
    CONSOLE_API_STRUCT(SrvSetConsoleTextAttribute, CONSOLE_SETTEXTATTRIBUTE_MSG),
    CONSOLE_API_STRUCT(SrvSetConsoleWindowInfo, CONSOLE_SETWINDOWINFO_MSG),
    CONSOLE_API_STRUCT(SrvReadConsoleOutputString, CONSOLE_READCONSOLEOUTPUTSTRING_MSG),
    CONSOLE_API_STRUCT(SrvWriteConsoleInput, CONSOLE_WRITECONSOLEINPUT_MSG),
    CONSOLE_API_STRUCT(SrvWriteConsoleOutput, CONSOLE_WRITECONSOLEOUTPUT_MSG),
    CONSOLE_API_STRUCT(SrvWriteConsoleOutputString, CONSOLE_WRITECONSOLEOUTPUTSTRING_MSG),
    CONSOLE_API_STRUCT(SrvReadConsoleOutput, CONSOLE_READCONSOLEOUTPUT_MSG),
    CONSOLE_API_STRUCT(SrvGetConsoleTitle, CONSOLE_GETTITLE_MSG),
    CONSOLE_API_STRUCT(SrvSetConsoleTitle, CONSOLE_SETTITLE_MSG),
};

const CONSOLE_API_DESCRIPTOR ConsoleApiLayer3[] = {
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_GETNUMBEROFFONTS_MSG),
    CONSOLE_API_STRUCT(SrvGetConsoleMouseInfo, CONSOLE_GETMOUSEINFO_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_GETFONTINFO_MSG),
    CONSOLE_API_STRUCT(SrvGetConsoleFontSize, CONSOLE_GETFONTSIZE_MSG),
    CONSOLE_API_STRUCT(SrvGetConsoleCurrentFont, CONSOLE_CURRENTFONT_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_SETFONT_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_SETICON_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_INVALIDATERECT_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_VDM_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_SETCURSOR_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_SHOWCURSOR_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_MENUCONTROL_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_SETPALETTE_MSG),
    CONSOLE_API_STRUCT(SrvSetConsoleDisplayMode, CONSOLE_SETDISPLAYMODE_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_REGISTERVDM_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_GETHARDWARESTATE_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_SETHARDWARESTATE_MSG),
    CONSOLE_API_STRUCT(SrvGetConsoleDisplayMode, CONSOLE_GETDISPLAYMODE_MSG),
    CONSOLE_API_STRUCT(SrvAddConsoleAlias, CONSOLE_ADDALIAS_MSG),
    CONSOLE_API_STRUCT(SrvGetConsoleAlias, CONSOLE_GETALIAS_MSG),
    CONSOLE_API_STRUCT(SrvGetConsoleAliasesLength, CONSOLE_GETALIASESLENGTH_MSG),
    CONSOLE_API_STRUCT(SrvGetConsoleAliasExesLength, CONSOLE_GETALIASEXESLENGTH_MSG),
    CONSOLE_API_STRUCT(SrvGetConsoleAliases, CONSOLE_GETALIASES_MSG),
    CONSOLE_API_STRUCT(SrvGetConsoleAliasExes, CONSOLE_GETALIASEXES_MSG),
    CONSOLE_API_STRUCT(SrvExpungeConsoleCommandHistory, CONSOLE_EXPUNGECOMMANDHISTORY_MSG),
    CONSOLE_API_STRUCT(SrvSetConsoleNumberOfCommands, CONSOLE_SETNUMBEROFCOMMANDS_MSG),
    CONSOLE_API_STRUCT(SrvGetConsoleCommandHistoryLength, CONSOLE_GETCOMMANDHISTORYLENGTH_MSG),
    CONSOLE_API_STRUCT(SrvGetConsoleCommandHistory, CONSOLE_GETCOMMANDHISTORY_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_SETKEYSHORTCUTS_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_SETMENUCLOSE_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_GETKEYBOARDLAYOUTNAME_MSG),
    CONSOLE_API_STRUCT(SrvGetConsoleWindow, CONSOLE_GETCONSOLEWINDOW_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_CHAR_TYPE_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_LOCAL_EUDC_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_CURSOR_MODE_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_CURSOR_MODE_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_REGISTEROS2_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_SETOS2OEMFORMAT_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_NLS_MODE_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_NLS_MODE_MSG),
    CONSOLE_API_STRUCT(SrvGetConsoleSelectionInfo, CONSOLE_GETSELECTIONINFO_MSG),
    CONSOLE_API_STRUCT(SrvGetConsoleProcessList, CONSOLE_GETCONSOLEPROCESSLIST_MSG),
    CONSOLE_API_STRUCT(SrvGetConsoleHistory, CONSOLE_HISTORY_MSG),
    CONSOLE_API_STRUCT(SrvSetConsoleHistory, CONSOLE_HISTORY_MSG),
    CONSOLE_API_STRUCT(SrvSetConsoleCurrentFont, CONSOLE_CURRENTFONT_MSG),
};

const CONSOLE_API_LAYER_DESCRIPTOR ConsoleApiLayerTable[] = {
    { ConsoleApiLayer1, RTL_NUMBER_OF(ConsoleApiLayer1) },
    { ConsoleApiLayer2, RTL_NUMBER_OF(ConsoleApiLayer2) },
    { ConsoleApiLayer3, RTL_NUMBER_OF(ConsoleApiLayer3) },
};

// Routine Description:
// - This routine validates a user IO and dispatches it to the appropriate worker routine.
// Arguments:
// - Message - Supplies the message representing the user IO.
// Return Value:
// - A pointer to the reply message, if this message is to be completed inline; nullptr if this message will pend now and complete later.
PCONSOLE_API_MSG ApiSorter::ConsoleDispatchRequest(_Inout_ PCONSOLE_API_MSG Message)
{
    // Make sure the indices are valid and retrieve the API descriptor.
    ULONG const LayerNumber = (Message->msgHeader.ApiNumber >> 24) - 1;
    ULONG const ApiNumber = Message->msgHeader.ApiNumber & 0xffffff;

    NTSTATUS Status;
    if ((LayerNumber >= RTL_NUMBER_OF(ConsoleApiLayerTable)) || (ApiNumber >= ConsoleApiLayerTable[LayerNumber].Count))
    {
        Status = STATUS_ILLEGAL_FUNCTION;
        goto Complete;
    }

    CONSOLE_API_DESCRIPTOR const *Descriptor = &ConsoleApiLayerTable[LayerNumber].Descriptor[ApiNumber];

    // Validate the argument size and call the API.
    if ((Message->Descriptor.InputSize < sizeof(CONSOLE_MSG_HEADER)) ||
        (Message->msgHeader.ApiDescriptorSize > sizeof(Message->u)) ||
        (Message->msgHeader.ApiDescriptorSize > Message->Descriptor.InputSize - sizeof(CONSOLE_MSG_HEADER)) || (Message->msgHeader.ApiDescriptorSize < Descriptor->RequiredSize))
    {
        Status = STATUS_ILLEGAL_FUNCTION;
        goto Complete;
    }

    BOOL ReplyPending = FALSE;
    Message->Complete.Write.Data = &Message->u;
    Message->Complete.Write.Size = Message->msgHeader.ApiDescriptorSize;
    Message->State.WriteOffset = Message->msgHeader.ApiDescriptorSize;
    Message->State.ReadOffset = Message->msgHeader.ApiDescriptorSize + sizeof(CONSOLE_MSG_HEADER);

    Status = (*Descriptor->Routine) (Message, &ReplyPending);

    if (!ReplyPending)
    {
        goto Complete;
    }

    return nullptr;

Complete:

    Message->SetReplyStatus(Status);

    return Message;
}
