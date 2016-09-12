#include "precomp.h"
#include "WinNTControl.h"

WinNTControl::WinNTControl() :
    _NtDllDll(THROW_LAST_ERROR_IF_NULL(LoadLibraryW(L"ntdll.dll"))),
    _NtOpenFile(reinterpret_cast<PfnNtOpenFile>(THROW_LAST_ERROR_IF_NULL(GetProcAddress(_NtDllDll.get(), "NtOpenFile"))))
{
}

WinNTControl::~WinNTControl()
{

}

WinNTControl& WinNTControl::GetInstance()
{
    static WinNTControl Instance;
    return Instance;
}

NTSTATUS WinNTControl::NtOpenFile(_Out_ PHANDLE FileHandle,
                                  _In_ ACCESS_MASK DesiredAccess,
                                  _In_ POBJECT_ATTRIBUTES ObjectAttributes,
                                  _Out_ PIO_STATUS_BLOCK IoStatusBlock,
                                  _In_ ULONG ShareAccess,
                                  _In_ ULONG OpenOptions)
{
    return GetInstance()._NtOpenFile(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, ShareAccess, OpenOptions);
}
