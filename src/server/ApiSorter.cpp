/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "ApiSorter.h"

#include "ApiDispatchers.h"

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
    CONSOLE_API_STRUCT(ApiDispatchers::ServeGetConsoleCP, CONSOLE_GETCP_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServeGetConsoleMode, CONSOLE_MODE_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServeSetConsoleMode, CONSOLE_MODE_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServeGetNumberOfInputEvents, CONSOLE_GETNUMBEROFINPUTEVENTS_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServeGetConsoleInput, CONSOLE_GETCONSOLEINPUT_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServeReadConsole, CONSOLE_READCONSOLE_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServeWriteConsole, CONSOLE_WRITECONSOLE_MSG),
    CONSOLE_API_NO_PARAMETER(SrvDeprecatedAPI), // ApiDispatchers::ServeConsoleNotifyLastClose
    CONSOLE_API_STRUCT(ApiDispatchers::ServeGetConsoleLangId, CONSOLE_LANGID_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_MAPBITMAP_MSG),
};

const CONSOLE_API_DESCRIPTOR ConsoleApiLayer2[] = {
    CONSOLE_API_STRUCT(ApiDispatchers::ServeFillConsoleOutput, CONSOLE_FILLCONSOLEOUTPUT_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServeGenerateConsoleCtrlEvent, CONSOLE_CTRLEVENT_MSG),
    CONSOLE_API_NO_PARAMETER(ApiDispatchers::ServeSetConsoleActiveScreenBuffer),
    CONSOLE_API_NO_PARAMETER(ApiDispatchers::ServeFlushConsoleInputBuffer),
    CONSOLE_API_STRUCT(ApiDispatchers::ServeSetConsoleCP, CONSOLE_SETCP_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServeGetConsoleCursorInfo, CONSOLE_GETCURSORINFO_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServeSetConsoleCursorInfo, CONSOLE_SETCURSORINFO_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServeGetConsoleScreenBufferInfo, CONSOLE_SCREENBUFFERINFO_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServeSetConsoleScreenBufferInfo, CONSOLE_SCREENBUFFERINFO_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServeSetConsoleScreenBufferSize, CONSOLE_SETSCREENBUFFERSIZE_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServeSetConsoleCursorPosition, CONSOLE_SETCURSORPOSITION_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServeGetLargestConsoleWindowSize, CONSOLE_GETLARGESTWINDOWSIZE_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServeScrollConsoleScreenBuffer, CONSOLE_SCROLLSCREENBUFFER_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServeSetConsoleTextAttribute, CONSOLE_SETTEXTATTRIBUTE_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServeSetConsoleWindowInfo, CONSOLE_SETWINDOWINFO_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServeReadConsoleOutputString, CONSOLE_READCONSOLEOUTPUTSTRING_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServeWriteConsoleInput, CONSOLE_WRITECONSOLEINPUT_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServeWriteConsoleOutput, CONSOLE_WRITECONSOLEOUTPUT_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServeWriteConsoleOutputString, CONSOLE_WRITECONSOLEOUTPUTSTRING_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServeReadConsoleOutput, CONSOLE_READCONSOLEOUTPUT_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServeGetConsoleTitle, CONSOLE_GETTITLE_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServeSetConsoleTitle, CONSOLE_SETTITLE_MSG),
};

const CONSOLE_API_DESCRIPTOR ConsoleApiLayer3[] = {
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_GETNUMBEROFFONTS_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServeGetConsoleMouseInfo, CONSOLE_GETMOUSEINFO_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_GETFONTINFO_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServeGetConsoleFontSize, CONSOLE_GETFONTSIZE_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServeGetConsoleCurrentFont, CONSOLE_CURRENTFONT_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_SETFONT_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_SETICON_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_INVALIDATERECT_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_VDM_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_SETCURSOR_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_SHOWCURSOR_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_MENUCONTROL_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_SETPALETTE_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServeSetConsoleDisplayMode, CONSOLE_SETDISPLAYMODE_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_REGISTERVDM_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_GETHARDWARESTATE_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_SETHARDWARESTATE_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServeGetConsoleDisplayMode, CONSOLE_GETDISPLAYMODE_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServeAddConsoleAlias, CONSOLE_ADDALIAS_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServeGetConsoleAlias, CONSOLE_GETALIAS_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServeGetConsoleAliasesLength, CONSOLE_GETALIASESLENGTH_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServeGetConsoleAliasExesLength, CONSOLE_GETALIASEXESLENGTH_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServeGetConsoleAliases, CONSOLE_GETALIASES_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServeGetConsoleAliasExes, CONSOLE_GETALIASEXES_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServeExpungeConsoleCommandHistory, CONSOLE_EXPUNGECOMMANDHISTORY_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServeSetConsoleNumberOfCommands, CONSOLE_SETNUMBEROFCOMMANDS_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServeGetConsoleCommandHistoryLength, CONSOLE_GETCOMMANDHISTORYLENGTH_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServeGetConsoleCommandHistory, CONSOLE_GETCOMMANDHISTORY_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_SETKEYSHORTCUTS_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_SETMENUCLOSE_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_GETKEYBOARDLAYOUTNAME_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServeGetConsoleWindow, CONSOLE_GETCONSOLEWINDOW_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_CHAR_TYPE_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_LOCAL_EUDC_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_CURSOR_MODE_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_CURSOR_MODE_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_REGISTEROS2_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_SETOS2OEMFORMAT_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_NLS_MODE_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_NLS_MODE_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServeGetConsoleSelectionInfo, CONSOLE_GETSELECTIONINFO_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServeGetConsoleProcessList, CONSOLE_GETCONSOLEPROCESSLIST_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServeGetConsoleHistory, CONSOLE_HISTORY_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServeSetConsoleHistory, CONSOLE_HISTORY_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServeSetConsoleCurrentFont, CONSOLE_CURRENTFONT_MSG)
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
