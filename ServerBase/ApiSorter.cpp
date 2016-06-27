#include "stdafx.h"
#include "ApiSorter.h"
#include "ApiDispatchers.h"

using namespace ApiDispatchers;

typedef NTSTATUS(*PCONSOLE_API_ROUTINE) (_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* m);

NTSTATUS ServeUnimplementedApi(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* m)
{
    UNREFERENCED_PARAMETER(pResponders);
    UNREFERENCED_PARAMETER(m);

    DebugBreak();

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS ServeDeprecatedApi(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* m)
{
    return ServeUnimplementedApi(pResponders, m);
}

struct ApiDescriptor
{
    ApiDescriptor(_In_ PCONSOLE_API_ROUTINE const Routine, _In_ ULONG const RequiredSize) :
        Routine(Routine),
        RequiredSize(RequiredSize) {}

    PCONSOLE_API_ROUTINE const Routine;
    ULONG const RequiredSize;

};

struct ApiLayerDescriptor
{
    ApiLayerDescriptor(_In_ const ApiDescriptor* const Descriptor, _In_ ULONG const Count) :
        Descriptor(Descriptor),
        Count(Count) {}

    const ApiDescriptor* const Descriptor;
    ULONG const Count;
};

const ApiDescriptor RawReadDescriptor(ServeReadConsole, sizeof(CONSOLE_READCONSOLE_MSG));

const ApiDescriptor ConsoleApiLayer1[] = {
    ApiDescriptor(ServeGetConsoleCP, sizeof(CONSOLE_GETCP_MSG)),                      // GetConsoleCP
    ApiDescriptor(ServeGetConsoleMode, sizeof(CONSOLE_MODE_MSG)),                       // GetConsoleMode
    ApiDescriptor(ServeSetConsoleMode, sizeof(CONSOLE_MODE_MSG)),                       // SetConsoleMode
    ApiDescriptor(ServeGetNumberOfInputEvents, sizeof(CONSOLE_GETNUMBEROFINPUTEVENTS_MSG)),     // GetNumberOfInputEvents
    ApiDescriptor(ServeGetConsoleInput, sizeof(CONSOLE_GETCONSOLEINPUT_MSG)),            // GetConsoleInput
    //ApiDescriptor(ServeReadConsole, sizeof(CONSOLE_READCONSOLE_MSG)),                // ReadConsole
	RawReadDescriptor,
    ApiDescriptor(ServeWriteConsole, sizeof(CONSOLE_WRITECONSOLE_MSG)),               // WriteConsole
    ApiDescriptor(ServeDeprecatedApi, 0),                                              // <reserved>
    ApiDescriptor(ServeGetConsoleLangId, sizeof(CONSOLE_LANGID_MSG)),                     // GetConsoleLangId
    ApiDescriptor(ServeDeprecatedApi, 0),                                              // <reserved>
};

const ApiDescriptor ConsoleApiLayer2[] = {
    ApiDescriptor(ServeFillConsoleOutput, sizeof(CONSOLE_FILLCONSOLEOUTPUT_MSG)),                 // FillConsoleOutput
    ApiDescriptor(ServeGenerateConsoleCtrlEvent, sizeof(CONSOLE_CTRLEVENT_MSG)),                  // GenerateConsoleCtrlEvent
    ApiDescriptor(ServeSetConsoleActiveScreenBuffer, 0),                                          // SetConsoleActiveScreenBuffer
    ApiDescriptor(ServeFlushConsoleInputBuffer, 0),                                               // FlushConsoleInputBuffer
    ApiDescriptor(ServeSetConsoleCP, sizeof(CONSOLE_SETCP_MSG)),                                  // SetConsoleCP
    ApiDescriptor(ServeGetConsoleCursorInfo, sizeof(CONSOLE_GETCURSORINFO_MSG)),                  // GetConsoleCursorInfo
    ApiDescriptor(ServeSetConsoleCursorInfo, sizeof(CONSOLE_SETCURSORINFO_MSG)),                  // SetConsoleCursorInfo
    ApiDescriptor(ServeGetConsoleScreenBufferInfo, sizeof(CONSOLE_SCREENBUFFERINFO_MSG)),         // GetConsoleScreenBufferInfo
    ApiDescriptor(ServeSetConsoleScreenBufferInfo, sizeof(CONSOLE_SCREENBUFFERINFO_MSG)),                // SetScreenBufferInfo
    ApiDescriptor(ServeSetConsoleScreenBufferSize, sizeof(CONSOLE_SETSCREENBUFFERSIZE_MSG)),      // SetConsoleScreenBufferSize
    ApiDescriptor(ServeSetConsoleCursorPosition, sizeof(CONSOLE_SETCURSORPOSITION_MSG)),          // SetConsoleCursorPosition
    ApiDescriptor(ServeGetLargestConsoleWindowSize, sizeof(CONSOLE_GETLARGESTWINDOWSIZE_MSG)),    // GetLargestConsoleWindowSize
    ApiDescriptor(ServeScrollConsoleScreenBuffer, sizeof(CONSOLE_SCROLLSCREENBUFFER_MSG)),        // ScrollConsoleScreenBuffer
    ApiDescriptor(ServeSetConsoleTextAttribute, sizeof(CONSOLE_SETTEXTATTRIBUTE_MSG)),            // SetConsoleTextAttribute
    ApiDescriptor(ServeSetConsoleWindowInfo, sizeof(CONSOLE_SETWINDOWINFO_MSG)),                  // SetConsoleWindowInfo
    ApiDescriptor(ServeReadConsoleOutputString, sizeof(CONSOLE_READCONSOLEOUTPUTSTRING_MSG)),     // ReadConsoleOutputString
    ApiDescriptor(ServeWriteConsoleInput, sizeof(CONSOLE_WRITECONSOLEINPUT_MSG)),                 // WriteConsoleInput
    ApiDescriptor(ServeWriteConsoleOutput, sizeof(CONSOLE_WRITECONSOLEOUTPUT_MSG)),               // WriteConsoleOutput
    ApiDescriptor(ServeWriteConsoleOutputString, sizeof(CONSOLE_WRITECONSOLEOUTPUTSTRING_MSG)),   // WriteConsoleOutputString
    ApiDescriptor(ServeReadConsoleOutput, sizeof(CONSOLE_READCONSOLEOUTPUT_MSG)),                 // ReadConsoleOutput
    ApiDescriptor(ServeGetConsoleTitle, sizeof(CONSOLE_GETTITLE_MSG)),                            // GetConsoleTitle
    ApiDescriptor(ServeSetConsoleTitle, sizeof(CONSOLE_SETTITLE_MSG)),                            // SetConsoleTitle
};

const ApiDescriptor ConsoleApiLayer3[] = {
    ApiDescriptor(ServeDeprecatedApi, 0), // <reserved>
    ApiDescriptor(ServeGetConsoleMouseInfo, sizeof(CONSOLE_GETMOUSEINFO_MSG)), // GetConsoleMouseInfo
    ApiDescriptor(ServeDeprecatedApi, 0), // <reserved>
    ApiDescriptor(ServeGetConsoleFontSize, sizeof(CONSOLE_GETFONTSIZE_MSG)), // GetConsoleFontSize
    ApiDescriptor(ServeGetConsoleCurrentFont, sizeof(CONSOLE_CURRENTFONT_MSG)), // GetConsoleCurrentFont
    ApiDescriptor(ServeDeprecatedApi, 0), // <reserved>
    ApiDescriptor(ServeDeprecatedApi, 0), // <reserved>
    ApiDescriptor(ServeDeprecatedApi, 0), // <reserved>
    ApiDescriptor(ServeDeprecatedApi, 0), // <reserved>
    ApiDescriptor(ServeDeprecatedApi, 0), // <reserved>
    ApiDescriptor(ServeDeprecatedApi, 0), // <reserved>
    ApiDescriptor(ServeDeprecatedApi, 0), // <reserved>
    ApiDescriptor(ServeDeprecatedApi, 0), // <reserved>
    ApiDescriptor(ServeSetConsoleDisplayMode, sizeof(CONSOLE_SETDISPLAYMODE_MSG)), // SetConsoleDisplayMode
    ApiDescriptor(ServeDeprecatedApi, 0), // <reserved>
    ApiDescriptor(ServeDeprecatedApi, 0), // <reserved>
    ApiDescriptor(ServeDeprecatedApi, 0), // <reserved>
    ApiDescriptor(ServeGetConsoleDisplayMode, sizeof(CONSOLE_GETDISPLAYMODE_MSG)), // GetConsoleDisplayMode
    ApiDescriptor(ServeAddConsoleAlias, sizeof(CONSOLE_ADDALIAS_MSG)), // AddConsoleAlias
    ApiDescriptor(ServeGetConsoleAlias, sizeof(CONSOLE_GETALIAS_MSG)), // GetConsoleAlias
    ApiDescriptor(ServeGetConsoleAliasesLength, sizeof(CONSOLE_GETALIASESLENGTH_MSG)), // GetConsoleAliasesLength
    ApiDescriptor(ServeGetConsoleAliasExesLength, sizeof(CONSOLE_GETALIASEXESLENGTH_MSG)), // GetConsoleAliasExesLength
    ApiDescriptor(ServeGetConsoleAliases, sizeof(CONSOLE_GETALIASES_MSG)), // GetConsoleAlises
    ApiDescriptor(ServeGetConsoleAliasExes, sizeof(CONSOLE_GETALIASEXES_MSG)), // GetConsoleAliasExes
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_EXPUNGECOMMANDHISTORY_MSG)), // ExpungeConsoleCommandHistory <unpublished, private doskey utility only>
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_SETNUMBEROFCOMMANDS_MSG)), // SetConsoleNumberOfCommands <unpublished, private doskey utility only>
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_GETCOMMANDHISTORYLENGTH_MSG)), // GetConsoleCommandHistoryLength <unpublished, private doskey utility only>
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_GETCOMMANDHISTORY_MSG)), // GetConsoleCommandHistory <unpublished, private doskey utility only>
    ApiDescriptor(ServeDeprecatedApi, 0), // <reserved>
    ApiDescriptor(ServeDeprecatedApi, 0), // <reserved>
    ApiDescriptor(ServeDeprecatedApi, 0), // <reserved>
    ApiDescriptor(ServeGetConsoleWindow, sizeof(CONSOLE_GETCONSOLEWINDOW_MSG)), // GetConsoleWindow
    ApiDescriptor(ServeDeprecatedApi, 0), // <reserved>
    ApiDescriptor(ServeDeprecatedApi, 0), // <reserved>
    ApiDescriptor(ServeDeprecatedApi, 0), // <reserved>
    ApiDescriptor(ServeDeprecatedApi, 0), // <reserved>
    ApiDescriptor(ServeDeprecatedApi, 0), // <reserved>
    ApiDescriptor(ServeDeprecatedApi, 0), // <reserved>
    ApiDescriptor(ServeDeprecatedApi, 0), // <reserved>
    ApiDescriptor(ServeDeprecatedApi, 0), // <reserved>
    ApiDescriptor(ServeGetConsoleSelectionInfo, sizeof(CONSOLE_GETSELECTIONINFO_MSG)), // GetConsoleSelectionInfo
    ApiDescriptor(ServeGetConsoleProcessList, sizeof(CONSOLE_GETCONSOLEPROCESSLIST_MSG)), // GetConsoleProcessList
    ApiDescriptor(ServeGetConsoleHistory, sizeof(CONSOLE_HISTORY_MSG)), // GetConsoleHistory
    ApiDescriptor(ServeSetConsoleHistory, sizeof(CONSOLE_HISTORY_MSG)), // SetConsoleHistory
    ApiDescriptor(ServeSetConsoleCurrentFont, sizeof(CONSOLE_CURRENTFONT_MSG)), // SetConsoleCurrentFont
};

const ApiLayerDescriptor ConsoleApiLayerTable[] = {
    ApiLayerDescriptor(ConsoleApiLayer1, RTL_NUMBER_OF(ConsoleApiLayer1)),
    ApiLayerDescriptor(ConsoleApiLayer2, RTL_NUMBER_OF(ConsoleApiLayer2)),
    ApiLayerDescriptor(ConsoleApiLayer3, RTL_NUMBER_OF(ConsoleApiLayer3)),
};

DWORD DoApiCall(_In_ ApiDescriptor const* Descriptor, _In_ IApiResponders* pResponders, _In_ CONSOLE_API_MSG* const pMsg);

DWORD LookupAndDoApiCall(_In_ IApiResponders* pResponders, _In_ CONSOLE_API_MSG* const pMsg)
{
	ULONG const LayerNumber = (pMsg->msgHeader.ApiNumber >> 24) - 1;
	ULONG const ApiNumber = (pMsg->msgHeader.ApiNumber & 0xffffff);

	NTSTATUS Status = STATUS_SUCCESS;
	if ((LayerNumber >= RTL_NUMBER_OF(ConsoleApiLayerTable)) || (ApiNumber >= ConsoleApiLayerTable[LayerNumber].Count))
	{
		Status = STATUS_ILLEGAL_FUNCTION;
	}

	if (Status == STATUS_SUCCESS)
	{
		ApiDescriptor const *Descriptor = &ConsoleApiLayerTable[LayerNumber].Descriptor[ApiNumber];

		// Validate the argument size and call the API.
		if ((pMsg->Descriptor.InputSize < sizeof(CONSOLE_MSG_HEADER)) ||
			(pMsg->msgHeader.ApiDescriptorSize > sizeof(pMsg->u)) ||
			(pMsg->msgHeader.ApiDescriptorSize > pMsg->Descriptor.InputSize - sizeof(CONSOLE_MSG_HEADER)) ||
			(pMsg->msgHeader.ApiDescriptorSize < Descriptor->RequiredSize))
		{
			Status = STATUS_ILLEGAL_FUNCTION;
		}

		if (Status == STATUS_SUCCESS)
		{
			pMsg->State.WriteOffset = pMsg->msgHeader.ApiDescriptorSize;
			pMsg->State.ReadOffset = pMsg->msgHeader.ApiDescriptorSize + sizeof(CONSOLE_MSG_HEADER);

			Status = DoApiCall(Descriptor, pResponders, pMsg);
		}
	}

	return Status;
}

DWORD DoRawReadCall(_In_ IApiResponders* pResponders, _In_ CONSOLE_API_MSG* const pMsg)
{
	pMsg->u.consoleMsgL1.ReadConsoleW.ProcessControlZ = TRUE;

	return DoApiCall(&RawReadDescriptor, pResponders, pMsg);
}

DWORD DoApiCall(_In_ ApiDescriptor const* Descriptor, _In_ IApiResponders* pResponders, _In_ CONSOLE_API_MSG* const pMsg)
{
	return (*Descriptor->Routine) (pResponders, pMsg);
}
