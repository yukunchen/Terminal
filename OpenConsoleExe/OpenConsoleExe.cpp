// OpenConsoleExe.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "OpenConsoleExe.h"

#include "..\ServerBase\IoThread.h"
#include "..\ServerSample\ApiResponderEmpty.h"
#include "NT\winbasep.h"
#include <NT/ntdef.h>

#define FILE_SYNCHRONOUS_IO_NONALERT            0x00000020

#include <processthreadsapi.h>

extern "C"
{
    std::vector<IoThread*> ioThreads;

    _declspec(dllexport)
        NTSTATUS ConsoleCreateIoThread(_In_ HANDLE Server)
    {
        ApiResponderEmpty* const pResponder = new ApiResponderEmpty();

        IoThread* const pNewThread = new IoThread(Server, pResponder);
        ioThreads.push_back(pNewThread);

        return STATUS_SUCCESS;
    }
}

//#include <ntdef.h>



typedef NTSTATUS(NTAPI* PfnNtOpenFile)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK, ULONG, ULONG);

NTSTATUS
NtOpenFile(
    _Out_ PHANDLE FileHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_ ULONG ShareAccess,
    _In_ ULONG OpenOptions
)
{
    NTSTATUS Status = STATUS_SUCCESS;

    // TODO: cache this, use direct link, something.
    HMODULE hModule = LoadLibraryW(L"ntdll.dll");
    if (hModule == nullptr)
    {
        Status = GetLastError();
    }

    if (NT_SUCCESS(Status))
    {
        PfnNtOpenFile pfn = (PfnNtOpenFile)GetProcAddress(hModule, "NtOpenFile");

        if (pfn == nullptr)
        {
            Status = GetLastError();
        }

        if (NT_SUCCESS(Status))
        {
            Status = pfn(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, ShareAccess, OpenOptions);
        }

        FreeLibrary(hModule);
    }

    return Status;
}

NTSTATUS
CspCreateHandle(
    __out PHANDLE Handle,
    __in PCWSTR DeviceName,
    __in ACCESS_MASK DesiredAccess,
    __in_opt HANDLE Parent,
    __in BOOLEAN Inheritable,
    __in ULONG OpenOptions
)
/*++

Routine Description:

This routine opens a handle to the console driver.

Arguments:

Handle - Receives the handle.

DeviceName - Supplies the name to be used to open the console driver.

DesiredAccess - Supplies the desired access mask.

Parent - Optionally supplies the parent object.

Inheritable - Supplies a boolean indicating if the new handle is to be made
inheritable.

OpenOptions - Supplies the open options to be passed to NtOpenFile. A common
option for clients is FILE_SYNCHRONOUS_IO_NONALERT, to make the handle
synchronous.

Return Value:

NTSTATUS indicating if the handle was successfully created.

--*/

{
    //SECURITY_ATTRIBUTES sa = { 0 };
    //sa.nLength = sizeof(sa);
    //sa.bInheritHandle = Inheritable;


    //*Handle = CreateFileW(
    //    DeviceName,
    //    DesiredAccess,
    //    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
    //    &sa, // NEED TO SPECIFY INHERITABLE
    //    OPEN_EXISTING,
    //    FILE_ATTRIBUTE_NORMAL,
    //    nullptr);

    //if (*Handle == INVALID_HANDLE_VALUE)
    //{
    //    return GetLastError();
    //}
    //

    //return STATUS_SUCCESS;

    ULONG Flags;
    IO_STATUS_BLOCK IoStatus;
    UNICODE_STRING Name;
    OBJECT_ATTRIBUTES ObjectAttributes;

    Flags = OBJ_CASE_INSENSITIVE;

    if (Inheritable != FALSE) {
        Flags |= OBJ_INHERIT;
    }

    //RtlInitUnicodeString(&Name, DeviceName);
    Name.Buffer = (wchar_t*)DeviceName;
    Name.Length = wcslen(DeviceName) * 2;
    Name.MaximumLength = Name.Length + sizeof(wchar_t);

    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               Flags,
                               Parent,
                               NULL);

    return NtOpenFile(Handle,
                      DesiredAccess,
                      &ObjectAttributes,
                      &IoStatus,
                      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                      OpenOptions);
}

NTSTATUS
CsCreateServerHandle(
    __out PHANDLE Handle,
    __in BOOLEAN Inheritable
)

/*++

Routine Description:

This routine creates a new server on the driver and returns a handle to it.

Arguments:

Handle - Receives a handle to the new server.

Inheritable - Supplies a flag indicating if the handle must be inheritable.

Return Value:

NTSTATUS indicating if the console was successfully created.

--*/

{
    return CspCreateHandle(Handle,
                           L"\\Device\\ConDrv\\Server",
                           GENERIC_ALL,
                           NULL,
                           Inheritable,
                           0);
}

NTSTATUS
CsCreateClientHandle(
    __out PHANDLE Handle,
    __in HANDLE ServerHandle,
    __in PCWSTR Name,
    __in BOOLEAN Inheritable
)

/*++

Routine Description:

This routine creates a handle to an input or output client of the given
server. No control io is sent to the server as this request must be coming
from the server itself.

Arguments:

Handle - Receives a handle to the new client.

ServerHandle - Supplies a handle to the server to which to attach the
newly created client.

Name - Supplies the name of the client object.

Inheritable - Supplies a flag indicating if the handle must be inheritable.


Return Value:

NTSTATUS indicating if the client was successfully created.

--*/

{
    return CspCreateHandle(Handle,
                           Name,
                           GENERIC_WRITE | GENERIC_READ | SYNCHRONIZE,
                           ServerHandle,
                           Inheritable,
                           FILE_SYNCHRONOUS_IO_NONALERT);
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance,
                      _In_ LPWSTR    lpCmdLine,
                      _In_ int       nCmdShow)
{
    /*struct {
        PROC_THREAD_ATTRIBUTE_LIST Head;
        PROC_THREAD_ATTRIBUTE Tail[1];
    } AttributeList;*/
    SIZE_T AttributeListSize;
    HANDLE ClientHandle[3];
    HANDLE CloseServerHandle;
    //PCS_CONSOLE Console;
    PROCESS_INFORMATION ProcessInformation;
    HANDLE ReferenceHandle;
    BOOL Result;
    HANDLE ServerHandle;
    STARTUPINFOEX StartupInformation;
    NTSTATUS Status;
    ULONG Win32Error;

    //Console = NULL;
    ClientHandle[0] = NULL;
    ClientHandle[1] = NULL;
    ClientHandle[2] = NULL;
    ReferenceHandle = NULL;
    CloseServerHandle = NULL;
    Win32Error = ERROR_SUCCESS;

    //
    // Create the server and reference handles and create the console object.
    //

    Status = CsCreateServerHandle(&ServerHandle, FALSE);
    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }

    CloseServerHandle = ServerHandle;

    Status = CsCreateClientHandle(&ReferenceHandle,
                                  ServerHandle,
                                  L"\\Reference",
                                  FALSE);

    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }

    //Status = CsCreateConsole(&Console,
    //                         ServerHandle,
    //                         Client,
    //                         80,
    //                         25,
    //                         80,
    //                         25,
    //                         &TnpConsoleCallbacks);

    Status = ConsoleCreateIoThread(ServerHandle);

    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }

    CloseServerHandle = NULL;

    //
    // Now that the console object was created, we're in a state that lets us
    // create the default io objects.
    //

    Status = CsCreateClientHandle(&ClientHandle[0],
                                  ServerHandle,
                                  L"\\Input",
                                  TRUE);

    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }

    Status = CsCreateClientHandle(&ClientHandle[1],
                                  ServerHandle,
                                  L"\\Output",
                                  TRUE);

    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }

    Status = DuplicateHandle(GetCurrentProcess(),
                             ClientHandle[1],
                             GetCurrentProcess(),
                             &ClientHandle[2],
                             0,
                             TRUE,
                             DUPLICATE_SAME_ACCESS);

    /*Status = NtDuplicateObject(NtCurrentProcess(),
                               ClientHandle[1],
                               NtCurrentProcess(),
                               &ClientHandle[2],
                               0,
                               0,
                               DUPLICATE_SAME_ACCESS |
                               DUPLICATE_SAME_ATTRIBUTES);*/

    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }


    //
    // Create the child process. We will temporarily overwrite the values in the
    // PEB to force them to be inherited.
    //

    RtlZeroMemory(&StartupInformation, sizeof(STARTUPINFOEX));

    StartupInformation.StartupInfo.cb = sizeof(STARTUPINFOEX);
    StartupInformation.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
    StartupInformation.StartupInfo.hStdInput = ClientHandle[0];
    StartupInformation.StartupInfo.hStdOutput = ClientHandle[1];
    StartupInformation.StartupInfo.hStdError = ClientHandle[2];
    StartupInformation.lpAttributeList;

    //
    // Create the extended attributes.
    //

    Result = InitializeProcThreadAttributeList(NULL,
                                               2,
                                               0,
                                               &AttributeListSize);

    StartupInformation.lpAttributeList = (PPROC_THREAD_ATTRIBUTE_LIST)new BYTE[AttributeListSize];
    

    Result = InitializeProcThreadAttributeList(StartupInformation.lpAttributeList,
                                               2,
                                               0,
                                               &AttributeListSize);

    if (Result == FALSE) {
        Win32Error = GetLastError();
        goto Exit;
    }

    Result = UpdateProcThreadAttribute(StartupInformation.lpAttributeList,
                                       0,
                                       PROC_THREAD_ATTRIBUTE_CONSOLE_REFERENCE,
                                       &ReferenceHandle,
                                       sizeof(HANDLE),
                                       NULL,
                                       NULL);

    if (Result == FALSE) {
        Win32Error = GetLastError();
        DeleteProcThreadAttributeList(StartupInformation.lpAttributeList);
        goto Exit;
    }

    Result = UpdateProcThreadAttribute(StartupInformation.lpAttributeList,
                                       0,
                                       PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
                                       &ClientHandle[0],
                                       sizeof ClientHandle,
                                       NULL,
                                       NULL);

    if (Result == FALSE) {
        Win32Error = GetLastError();
        DeleteProcThreadAttributeList(StartupInformation.lpAttributeList);
        goto Exit;
    }

    LPCWSTR CmdLine = lpCmdLine;

    if (wcslen(CmdLine) == 0)
    {
        CmdLine = L"cmd.exe";
    }
    
    size_t length = wcslen(CmdLine) + 1;
    LPWSTR CmdLineMutable = new wchar_t[length];

    //CopyMemory(CmdLineMutable, CmdLine, wcslen(CmdLine));

    wcscpy_s(CmdLineMutable, length, CmdLine);
    CmdLineMutable[length - 1] = L'\0';

    Result = CreateProcessW(NULL,
                            //lpCmdLine,
                            CmdLineMutable,
                            NULL,
                            NULL,
                            TRUE,
                            EXTENDED_STARTUPINFO_PRESENT,
                            NULL,
                            NULL,
                            &StartupInformation.StartupInfo,
                            &ProcessInformation);

    if (Result == FALSE) {
        Win32Error = GetLastError();
        DeleteProcThreadAttributeList(StartupInformation.lpAttributeList);
        goto Exit;
    }

    DeleteProcThreadAttributeList(StartupInformation.lpAttributeList);

    CloseHandle(ProcessInformation.hThread);

    //SetProcessShutdownParameters(0, 0);

    // Exit the thread so the CRT won't clean us up and kill. The IO thread owns the lifetime now.
    ExitThread(Win32Error);
    //
    // The process has been created, so update the state and start handling
    // requests.
    //

//    CsStartConsole(Console);
//    Client->Console = Console;
//    Console = NULL;
//
//    Client->Shell = ProcessInformation.hProcess;
//
Exit:
//
//    if (ClientHandle[0] != NULL) {
//        NtClose(ClientHandle[0]);
//    }
//
//    if (ClientHandle[1] != NULL) {
//        NtClose(ClientHandle[1]);
//    }
//
//    if (ClientHandle[2] != NULL) {
//        NtClose(ClientHandle[2]);
//    }
//
//    if (ReferenceHandle != NULL) {
//        NtClose(ReferenceHandle);
//    }
//
//    if (CloseServerHandle != NULL) {
//        NtClose(CloseServerHandle);
//    }
//
//    if (Console != NULL) {
//        CsCloseConsole(Console);
//    }
//
//    if (!NT_SUCCESS(Status)) {
//        return RtlNtStatusToDosError(Status);
//    }

    return Win32Error;

}