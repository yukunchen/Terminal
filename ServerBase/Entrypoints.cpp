#include "stdafx.h"
#include "..\ServerBaseApi\Entrypoints.h"

#include "DeviceHandle.h"
#include "IoThread.h"

#include <NT\winbasep.h>

std::vector<IoThread*> ioThreads;

NTSTATUS Entrypoints::StartConsoleForServerHandle(_In_ HANDLE const ServerHandle,
                                                  _In_ IApiResponders* const pResponder)
{
    IoThread* const pNewThread = new IoThread(ServerHandle, pResponder);
    ioThreads.push_back(pNewThread);

    return STATUS_SUCCESS;
}

NTSTATUS Entrypoints::StartConsoleForCmdLine(_In_ PCWSTR CmdLine,
                                             _In_ IApiResponders* const pResponder)
{
    // Create a scope because we're going to exit thread if everything goes well.
    // This scope will ensure all C++ objects and smart pointers get a chance to destruct before ExitThread is called.
    {
        // Create the server and reference handles and create the console object.
        //wil::unique_handle ServerHandle;
        HANDLE ServerHandle;
        RETURN_IF_NTSTATUS_FAILED(DeviceHandle::CreateServerHandle(&ServerHandle, FALSE));

        //wil::unique_handle ReferenceHandle;
        HANDLE ReferenceHandle;
        RETURN_IF_NTSTATUS_FAILED(DeviceHandle::CreateClientHandle(&ReferenceHandle,
                                                                   ServerHandle,
                                                                   L"\\Reference",
                                                                   FALSE));

        RETURN_IF_NTSTATUS_FAILED(Entrypoints::StartConsoleForServerHandle(ServerHandle, pResponder));

        // If the console server took the handle, it owns it now, not us. Release.
        //ServerHandle.release();

        // Now that the console object was created, we're in a state that lets us
        // create the default io objects.
        //wil::unique_handle ClientHandle[3];
        HANDLE ClientHandle[3];


        // Input
        RETURN_IF_NTSTATUS_FAILED(DeviceHandle::CreateClientHandle(
            //ClientHandle[0].addressof(),
            &ClientHandle[0],
            ServerHandle,
            L"\\Input",
            TRUE));

        // Output
        RETURN_IF_NTSTATUS_FAILED(DeviceHandle::CreateClientHandle(
            //ClientHandle[1].addressof(),
            &ClientHandle[1],
            ServerHandle,
            L"\\Output",
            TRUE));

        // Error is a copy of Output
        RETURN_IF_WIN32_BOOL_FALSE(DuplicateHandle(GetCurrentProcess(),
                                                   //ClientHandle[1].get(),
                                                   ClientHandle[1],
                                                   GetCurrentProcess(),
                                                   //ClientHandle[2].addressof(),
                                                   &ClientHandle[2],
                                                   0,
                                                   TRUE,
                                                   DUPLICATE_SAME_ACCESS));


        // Create the child process. We will temporarily overwrite the values in the
        // PEB to force them to be inherited.

        STARTUPINFOEX StartupInformation = { 0 };
        StartupInformation.StartupInfo.cb = sizeof(STARTUPINFOEX);
        StartupInformation.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
        //StartupInformation.StartupInfo.hStdInput = ClientHandle[0].get();
        //StartupInformation.StartupInfo.hStdOutput = ClientHandle[1].get();
        //StartupInformation.StartupInfo.hStdError = ClientHandle[2].get();

        StartupInformation.StartupInfo.hStdInput = ClientHandle[0];
        StartupInformation.StartupInfo.hStdOutput = ClientHandle[1];
        StartupInformation.StartupInfo.hStdError = ClientHandle[2];

        // Create the extended attributes list that will pass the console server information into the child process.

        // Call first time to find size
        SIZE_T AttributeListSize;
        InitializeProcThreadAttributeList(NULL,
                                          2,
                                          0,
                                          &AttributeListSize);

        // Alloc space
        //std::unique_ptr<BYTE[]> AttributeList = std::make_unique<BYTE[]>(AttributeListSize);
        //StartupInformation.lpAttributeList = reinterpret_cast<PPROC_THREAD_ATTRIBUTE_LIST>(AttributeList.get());
        StartupInformation.lpAttributeList = (PPROC_THREAD_ATTRIBUTE_LIST)new BYTE[AttributeListSize];

        // Call second time to actually initialize space.
        RETURN_IF_WIN32_BOOL_FALSE(InitializeProcThreadAttributeList(StartupInformation.lpAttributeList,
                                                                     2,
                                                                     0,
                                                                     &AttributeListSize));
        // Set cleanup data for ProcThreadAttributeList when successful.
        auto CleanupProcThreadAttribute = wil::ScopeExit([StartupInformation] {
            DeleteProcThreadAttributeList(StartupInformation.lpAttributeList);
        });


        RETURN_IF_WIN32_BOOL_FALSE(UpdateProcThreadAttribute(StartupInformation.lpAttributeList,
                                                             0,
                                                             PROC_THREAD_ATTRIBUTE_CONSOLE_REFERENCE,
                                                             &ReferenceHandle,
                                                             sizeof(HANDLE),
                                                             NULL,
                                                             NULL));

        RETURN_IF_WIN32_BOOL_FALSE(UpdateProcThreadAttribute(StartupInformation.lpAttributeList,
                                                             0,
                                                             PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
                                                             &ClientHandle[0],
                                                             sizeof ClientHandle,
                                                             NULL,
                                                             NULL));

        // We have to copy the command line string we're given because CreateProcessW has to be called with mutable data.
        if (wcslen(CmdLine) == 0)
        {
            // If they didn't give us one, just launch cmd.exe.
            CmdLine = L"cmd.exe";
        }

        size_t length = wcslen(CmdLine) + 1;
        std::unique_ptr<wchar_t[]> CmdLineMutable = std::make_unique<wchar_t[]>(length);

        wcscpy_s(CmdLineMutable.get(), length, CmdLine);
        CmdLineMutable[length - 1] = L'\0';




        //wil::unique_process_information ProcessInformation;
        PROCESS_INFORMATION ProcessInformation;
        RETURN_IF_WIN32_BOOL_FALSE(CreateProcessW(NULL,
                                                  CmdLineMutable.get(),
                                                  NULL,
                                                  NULL,
                                                  TRUE,
                                                  EXTENDED_STARTUPINFO_PRESENT,
                                                  NULL,
                                                  NULL,
                                                  &StartupInformation.StartupInfo,
                                                  &ProcessInformation));
        //ProcessInformation.addressof()));
    }

    // Exit the thread so the CRT won't clean us up and kill. The IO thread owns the lifetime now.
    ExitThread(STATUS_SUCCESS);

    // We won't hit this. The ExitThread above will kill the caller at this point.
    return STATUS_SUCCESS;
}
