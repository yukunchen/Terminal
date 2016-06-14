#include "stdafx.h"
#include "Entrypoint.h"
#include "DeviceComm.h"
#include "DeviceProtocol.h"

using namespace std;

DWORD DoConnect(_In_ DeviceProtocol* Server, _In_ CD_IO_DESCRIPTOR* const pMsg)
{

    HANDLE InputEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    DWORD result = Server->SetInputAvailableEvent(InputEvent);
    
    // TODO: These are junk handles for now to various info that we want to get back when other calls come in.
    result = Server->SetConnectionInformation(pMsg, 0x14, 0x18, 0x1a);

    return result;
}

DWORD DoDefault(_In_ DeviceProtocol* Server, _In_ CD_IO_DESCRIPTOR* const pMsg)
{
    return Server->SendCompletion(pMsg, STATUS_UNSUCCESSFUL, nullptr, 0);
}

typedef NTSTATUS(*PCONSOLE_API_ROUTINE) (_Inout_ PCONSOLE_API_MSG m);

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

NTSTATUS ServeUnimplementedApi(_Inout_ PCONSOLE_API_MSG m)
{
    UNREFERENCED_PARAMETER(m);

    DebugBreak();

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS ServeDeprecatedApi(_Inout_ PCONSOLE_API_MSG m)
{
    return ServeUnimplementedApi(m);
}

const ApiDescriptor ConsoleApiLayer1[] = {
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_GETCP_MSG)),                      // GetConsoleCP
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_MODE_MSG)),                       // GetConsoleMode
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_MODE_MSG)),                       // SetConsoleMode
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_GETNUMBEROFINPUTEVENTS_MSG)),     // GetNumberOfInputEvents
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_GETCONSOLEINPUT_MSG)),            // GetConsoleInput
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_READCONSOLE_MSG)),                // ReadConsole
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_WRITECONSOLE_MSG)),               // WriteConsole
    ApiDescriptor(ServeDeprecatedApi, 0),                                              // <reserved>
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_LANGID_MSG)),                     // GetConsoleLangId
    ApiDescriptor(ServeDeprecatedApi, 0),                                              // <reserved>
};

const ApiDescriptor ConsoleApiLayer2[] = {
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_FILLCONSOLEOUTPUT_MSG)),                 // FillConsoleOutput
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_CTRLEVENT_MSG)),                  // GenerateConsoleCtrlEvent
    ApiDescriptor(ServeUnimplementedApi, 0),                                          // SetConsoleActiveScreenBuffer
    ApiDescriptor(ServeUnimplementedApi, 0),                                               // FlushConsoleInputBuffer
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_SETCP_MSG)),                                  // SetConsoleCP
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_GETCURSORINFO_MSG)),                  // GetConsoleCursorInfo
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_SETCURSORINFO_MSG)),                  // SetConsoleCursorInfo
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_SCREENBUFFERINFO_MSG)),         // GetConsoleScreenBufferInfo
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_SCREENBUFFERINFO_MSG)),                // SetScreenBufferInfo
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_SETSCREENBUFFERSIZE_MSG)),      // SetConsoleScreenBufferSize
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_SETCURSORPOSITION_MSG)),          // SetConsoleCursorPosition
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_GETLARGESTWINDOWSIZE_MSG)),    // GetLargestConsoleWindowSize
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_SCROLLSCREENBUFFER_MSG)),        // ScrollConsoleScreenBuffer
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_SETTEXTATTRIBUTE_MSG)),            // SetConsoleTextAttribute
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_SETWINDOWINFO_MSG)),                  // SetConsoleWindowInfo
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_READCONSOLEOUTPUTSTRING_MSG)),     // ReadConsoleOutputString
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_WRITECONSOLEINPUT_MSG)),                 // WriteConsoleInput
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_WRITECONSOLEOUTPUT_MSG)),               // WriteConsoleOutput
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_WRITECONSOLEOUTPUTSTRING_MSG)),   // WriteConsoleOutputString
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_READCONSOLEOUTPUT_MSG)),                 // ReadConsoleOutput
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_GETTITLE_MSG)),                            // GetConsoleTitle
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_SETTITLE_MSG)),                            // SetConsoleTitle
};

const ApiDescriptor ConsoleApiLayer3[] = {
    ApiDescriptor(ServeDeprecatedApi, 0), // <reserved>
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_GETMOUSEINFO_MSG)), // GetConsoleMouseInfo
    ApiDescriptor(ServeDeprecatedApi, 0), // <reserved>
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_GETFONTSIZE_MSG)), // GetConsoleFontSize
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_CURRENTFONT_MSG)), // GetConsoleCurrentFont
    ApiDescriptor(ServeDeprecatedApi, 0), // <reserved>
    ApiDescriptor(ServeDeprecatedApi, 0), // <reserved>
    ApiDescriptor(ServeDeprecatedApi, 0), // <reserved>
    ApiDescriptor(ServeDeprecatedApi, 0), // <reserved>
    ApiDescriptor(ServeDeprecatedApi, 0), // <reserved>
    ApiDescriptor(ServeDeprecatedApi, 0), // <reserved>
    ApiDescriptor(ServeDeprecatedApi, 0), // <reserved>
    ApiDescriptor(ServeDeprecatedApi, 0), // <reserved>
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_SETDISPLAYMODE_MSG)), // SetConsoleDisplayMode
    ApiDescriptor(ServeDeprecatedApi, 0), // <reserved>
    ApiDescriptor(ServeDeprecatedApi, 0), // <reserved>
    ApiDescriptor(ServeDeprecatedApi, 0), // <reserved>
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_GETDISPLAYMODE_MSG)), // GetConsoleDisplayMode
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_ADDALIAS_MSG)), // AddConsoleAlias
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_GETALIAS_MSG)), // GetConsoleAlias
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_GETALIASESLENGTH_MSG)), // GetConsoleAliasesLength
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_GETALIASEXESLENGTH_MSG)), // GetConsoleAliasExesLength
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_GETALIASES_MSG)), // GetConsoleAlises
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_GETALIASEXES_MSG)), // GetConsoleAliasExes
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_EXPUNGECOMMANDHISTORY_MSG)), // ExpungeConsoleCommandHistory
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_SETNUMBEROFCOMMANDS_MSG)), // SetConsoleNumberOfCommands
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_GETCOMMANDHISTORYLENGTH_MSG)), // GetConsoleCommandHistoryLength
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_GETCOMMANDHISTORY_MSG)), // GetConsoleCommandHistory
    ApiDescriptor(ServeDeprecatedApi, 0), // <reserved>
    ApiDescriptor(ServeDeprecatedApi, 0), // <reserved>
    ApiDescriptor(ServeDeprecatedApi, 0), // <reserved>
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_GETCONSOLEWINDOW_MSG)), // GetConsoleWindow
    ApiDescriptor(ServeDeprecatedApi, 0), // <reserved>
    ApiDescriptor(ServeDeprecatedApi, 0), // <reserved>
    ApiDescriptor(ServeDeprecatedApi, 0), // <reserved>
    ApiDescriptor(ServeDeprecatedApi, 0), // <reserved>
    ApiDescriptor(ServeDeprecatedApi, 0), // <reserved>
    ApiDescriptor(ServeDeprecatedApi, 0), // <reserved>
    ApiDescriptor(ServeDeprecatedApi, 0), // <reserved>
    ApiDescriptor(ServeDeprecatedApi, 0), // <reserved>
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_GETSELECTIONINFO_MSG)), // GetConsoleSelectionInfo
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_GETCONSOLEPROCESSLIST_MSG)), // GetConsoleProcessList
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_HISTORY_MSG)), // GetConsoleHistory
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_HISTORY_MSG)), // SetConsoleHistory
    ApiDescriptor(ServeUnimplementedApi, sizeof(CONSOLE_CURRENTFONT_MSG)), // SetConsoleCurrentFont
};

const ApiLayerDescriptor ConsoleApiLayerTable[] = {
    ApiLayerDescriptor(ConsoleApiLayer1, RTL_NUMBER_OF(ConsoleApiLayer1)),
    ApiLayerDescriptor(ConsoleApiLayer2, RTL_NUMBER_OF(ConsoleApiLayer2)),
    ApiLayerDescriptor(ConsoleApiLayer3, RTL_NUMBER_OF(ConsoleApiLayer3)),
};

DWORD DoApiCall(_In_ DeviceProtocol* Server, _In_ CONSOLE_API_MSG* const pMsg)
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

            Status = (*Descriptor->Routine) (pMsg);

            Status = Server->SendCompletion(&pMsg->Descriptor,
                                            Status,
                                            &pMsg->u,
                                            pMsg->msgHeader.ApiDescriptorSize);
        }
    }

    return Status;
}

void IoLoop(_In_ HANDLE Server)
{
    DeviceComm Comm(Server);
    DeviceProtocol Prot(&Comm);

    bool fExiting = false;

    while (!fExiting)
    {
        CONSOLE_API_MSG ReceiveMsg;
        DWORD Error = Prot.GetReadIo(&ReceiveMsg);

        // TODO: Wait for pending

        if (ERROR_PIPE_NOT_CONNECTED == Error)
        {
            fExiting = true;
            continue;
        }
        else if (S_OK != Error)
        {
            DebugBreak();
        }

        // Route function to handler
        switch (ReceiveMsg.Descriptor.Function)
        {
        case CONSOLE_IO_USER_DEFINED:
        {
            DoApiCall(&Prot, &ReceiveMsg);
            break;
        }
        case CONSOLE_IO_CONNECT:
        {
            DoConnect(&Prot, &ReceiveMsg.Descriptor);
            break;
        }
        default:
        {
            DoDefault(&Prot, &ReceiveMsg.Descriptor);
            break;
        }
        }

    }

    ExitProcess(STATUS_SUCCESS);
}

extern "C"
{
    _declspec(dllexport)
        NTSTATUS ConsoleCreateIoThread(_In_ HANDLE Server)
    {
        thread ioThread(IoLoop, Server);
        ioThread.detach();

        return STATUS_SUCCESS;
    }
}
