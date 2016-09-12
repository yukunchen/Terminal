#pragma once
class DeviceHandle
{
public:
    static NTSTATUS
        CreateServerHandle(
            _Out_ PHANDLE Handle,
            _In_ BOOLEAN Inheritable
        );

    static NTSTATUS
        CreateClientHandle(
            _Out_ PHANDLE Handle,
            _In_ HANDLE ServerHandle,
            _In_ PCWSTR Name,
            _In_ BOOLEAN Inheritable
        );

private:
    static NTSTATUS
        _CreateHandle(
            _Out_ PHANDLE Handle,
            _In_ PCWSTR DeviceName,
            _In_ ACCESS_MASK DesiredAccess,
            _In_opt_ HANDLE Parent,
            _In_ BOOLEAN Inheritable,
            _In_ ULONG OpenOptions
        );

};

