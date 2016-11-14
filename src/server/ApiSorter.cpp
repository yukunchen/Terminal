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

const CONSOLE_API_DESCRIPTOR ConsoleApiLayer1[] = {
    CONSOLE_API_STRUCT(ApiDispatchers::ServerGetConsoleCP, CONSOLE_GETCP_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerGetConsoleMode, CONSOLE_MODE_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerSetConsoleMode, CONSOLE_MODE_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerGetNumberOfInputEvents, CONSOLE_GETNUMBEROFINPUTEVENTS_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerGetConsoleInput, CONSOLE_GETCONSOLEINPUT_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerReadConsole, CONSOLE_READCONSOLE_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerWriteConsole, CONSOLE_WRITECONSOLE_MSG),
    CONSOLE_API_NO_PARAMETER(ApiDispatchers::ServerDeprecatedApi), // ApiDispatchers::ServerConsoleNotifyLastClose
    CONSOLE_API_STRUCT(ApiDispatchers::ServerGetConsoleLangId, CONSOLE_LANGID_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerDeprecatedApi, CONSOLE_MAPBITMAP_MSG),
};

const CONSOLE_API_DESCRIPTOR ConsoleApiLayer2[] = {
    CONSOLE_API_STRUCT(ApiDispatchers::ServerFillConsoleOutput, CONSOLE_FILLCONSOLEOUTPUT_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerGenerateConsoleCtrlEvent, CONSOLE_CTRLEVENT_MSG),
    CONSOLE_API_NO_PARAMETER(ApiDispatchers::ServerSetConsoleActiveScreenBuffer),
    CONSOLE_API_NO_PARAMETER(ApiDispatchers::ServerFlushConsoleInputBuffer),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerSetConsoleCP, CONSOLE_SETCP_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerGetConsoleCursorInfo, CONSOLE_GETCURSORINFO_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerSetConsoleCursorInfo, CONSOLE_SETCURSORINFO_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerGetConsoleScreenBufferInfo, CONSOLE_SCREENBUFFERINFO_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerSetConsoleScreenBufferInfo, CONSOLE_SCREENBUFFERINFO_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerSetConsoleScreenBufferSize, CONSOLE_SETSCREENBUFFERSIZE_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerSetConsoleCursorPosition, CONSOLE_SETCURSORPOSITION_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerGetLargestConsoleWindowSize, CONSOLE_GETLARGESTWINDOWSIZE_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerScrollConsoleScreenBuffer, CONSOLE_SCROLLSCREENBUFFER_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerSetConsoleTextAttribute, CONSOLE_SETTEXTATTRIBUTE_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerSetConsoleWindowInfo, CONSOLE_SETWINDOWINFO_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerReadConsoleOutputString, CONSOLE_READCONSOLEOUTPUTSTRING_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerWriteConsoleInput, CONSOLE_WRITECONSOLEINPUT_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerWriteConsoleOutput, CONSOLE_WRITECONSOLEOUTPUT_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerWriteConsoleOutputString, CONSOLE_WRITECONSOLEOUTPUTSTRING_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerReadConsoleOutput, CONSOLE_READCONSOLEOUTPUT_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerGetConsoleTitle, CONSOLE_GETTITLE_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerSetConsoleTitle, CONSOLE_SETTITLE_MSG),
};

const CONSOLE_API_DESCRIPTOR ConsoleApiLayer3[] = {
    CONSOLE_API_STRUCT(ApiDispatchers::ServerDeprecatedApi, CONSOLE_GETNUMBEROFFONTS_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerGetConsoleMouseInfo, CONSOLE_GETMOUSEINFO_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerDeprecatedApi, CONSOLE_GETFONTINFO_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerGetConsoleFontSize, CONSOLE_GETFONTSIZE_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerGetConsoleCurrentFont, CONSOLE_CURRENTFONT_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerDeprecatedApi, CONSOLE_SETFONT_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerDeprecatedApi, CONSOLE_SETICON_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerDeprecatedApi, CONSOLE_INVALIDATERECT_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerDeprecatedApi, CONSOLE_VDM_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerDeprecatedApi, CONSOLE_SETCURSOR_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerDeprecatedApi, CONSOLE_SHOWCURSOR_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerDeprecatedApi, CONSOLE_MENUCONTROL_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerDeprecatedApi, CONSOLE_SETPALETTE_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerSetConsoleDisplayMode, CONSOLE_SETDISPLAYMODE_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerDeprecatedApi, CONSOLE_REGISTERVDM_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerDeprecatedApi, CONSOLE_GETHARDWARESTATE_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerDeprecatedApi, CONSOLE_SETHARDWARESTATE_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerGetConsoleDisplayMode, CONSOLE_GETDISPLAYMODE_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerAddConsoleAlias, CONSOLE_ADDALIAS_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerGetConsoleAlias, CONSOLE_GETALIAS_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerGetConsoleAliasesLength, CONSOLE_GETALIASESLENGTH_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerGetConsoleAliasExesLength, CONSOLE_GETALIASEXESLENGTH_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerGetConsoleAliases, CONSOLE_GETALIASES_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerGetConsoleAliasExes, CONSOLE_GETALIASEXES_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerExpungeConsoleCommandHistory, CONSOLE_EXPUNGECOMMANDHISTORY_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerSetConsoleNumberOfCommands, CONSOLE_SETNUMBEROFCOMMANDS_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerGetConsoleCommandHistoryLength, CONSOLE_GETCOMMANDHISTORYLENGTH_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerGetConsoleCommandHistory, CONSOLE_GETCOMMANDHISTORY_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerDeprecatedApi, CONSOLE_SETKEYSHORTCUTS_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerDeprecatedApi, CONSOLE_SETMENUCLOSE_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerDeprecatedApi, CONSOLE_GETKEYBOARDLAYOUTNAME_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerGetConsoleWindow, CONSOLE_GETCONSOLEWINDOW_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerDeprecatedApi, CONSOLE_CHAR_TYPE_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerDeprecatedApi, CONSOLE_LOCAL_EUDC_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerDeprecatedApi, CONSOLE_CURSOR_MODE_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerDeprecatedApi, CONSOLE_CURSOR_MODE_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerDeprecatedApi, CONSOLE_REGISTEROS2_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerDeprecatedApi, CONSOLE_SETOS2OEMFORMAT_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerDeprecatedApi, CONSOLE_NLS_MODE_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerDeprecatedApi, CONSOLE_NLS_MODE_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerGetConsoleSelectionInfo, CONSOLE_GETSELECTIONINFO_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerGetConsoleProcessList, CONSOLE_GETCONSOLEPROCESSLIST_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerGetConsoleHistory, CONSOLE_HISTORY_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerSetConsoleHistory, CONSOLE_HISTORY_MSG),
    CONSOLE_API_STRUCT(ApiDispatchers::ServerSetConsoleCurrentFont, CONSOLE_CURRENTFONT_MSG)
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

    // TODO: MSFT 9574400: This currently causes an issue with double printing some lines. To be corrected.
    //Status = NTSTATUS_FROM_HRESULT((*Descriptor->Routine) (Message, &ReplyPending));
    Status = (*Descriptor->Routine)(Message, &ReplyPending);

    if (!ReplyPending)
    {
        goto Complete;
    }

    return nullptr;

Complete:

    Message->SetReplyStatus(Status);

    return Message;
}
