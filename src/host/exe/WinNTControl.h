#pragma once

#include <NT\ntdef.h>

class WinNTControl
{
public:
    static NTSTATUS NtOpenFile(_Out_ PHANDLE FileHandle,
                               _In_ ACCESS_MASK DesiredAccess,
                               _In_ POBJECT_ATTRIBUTES ObjectAttributes,
                               _Out_ PIO_STATUS_BLOCK IoStatusBlock,
                               _In_ ULONG ShareAccess,
                               _In_ ULONG OpenOptions);

    ~WinNTControl();

private:
    WinNTControl();

    WinNTControl(WinNTControl const&) = delete;
    void operator=(WinNTControl const&) = delete;

    static WinNTControl& GetInstance();

    wil::unique_hmodule const _NtDllDll;

    typedef NTSTATUS(NTAPI* PfnNtOpenFile)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK, ULONG, ULONG);
    PfnNtOpenFile const _NtOpenFile;

};
