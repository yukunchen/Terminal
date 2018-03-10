/*++
Copyright (c) Microsoft Corporation

Module Name:
- ProcessPolicy.h

Abstract:
- This file defines a policy framework that applies to attached client applications
  to restrict or enforce certain behavior depending on the client app type.

Author:
- Michael Niksa (miniksa) 06-Oct-2017

--*/

#pragma once

class ConsoleProcessPolicy
{
public:
    ~ConsoleProcessPolicy();

    static ConsoleProcessPolicy s_CreateInstance(_In_ const HANDLE hProcess);

    bool CanReadOutputBuffer() const;
    bool CanWriteInputBuffer() const;

private:
    ConsoleProcessPolicy(_In_ const bool fCanReadOutputBuffer,
                         _In_ const bool fCanWriteInputBuffer);

    [[nodiscard]]
    static HRESULT s_CheckAppModelPolicy(_In_ const HANDLE hToken,
                                         _Inout_ bool& fIsWrongWayBlocked);

    [[nodiscard]]
    static HRESULT s_CheckIntegrityLevelPolicy(_In_ const HANDLE hOtherToken,
                                               _Inout_ bool& fIsWrongWayBlocked);

    [[nodiscard]]
    static HRESULT s_GetIntegrityLevel(_In_ const HANDLE hToken, _Out_ DWORD& dwIntegrityLevel);

    const bool _fCanReadOutputBuffer;
    const bool _fCanWriteInputBuffer;
};
