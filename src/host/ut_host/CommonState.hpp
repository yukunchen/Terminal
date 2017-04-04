/*++

Copyright (c) Microsoft Corporation

Module Name:
- CommonState.hpp

Abstract:
- This represents common boilerplate state setup required for unit tests to run

Author(s):
- Michael Niksa (miniksa) 18-Jun-2014
- Paul Campbell (paulcam) 18-Jun-2014

--*/

#pragma once

class CommonState
{
public:
    CommonState();
    ~CommonState();
    void PrepareGlobalFont();
    void PrepareGlobalScreenBuffer();
    void PrepareCookedReadData();
    void PrepareNewTextBufferInfo();
    void PrepareGlobalInputBuffer();

    void CleanupGlobalFont();
    void CleanupGlobalScreenBuffer();
    void CleanupCookedReadData();
    void CleanupNewTextBufferInfo();
    void CleanupGlobalInputBuffer();

    void FillTextBuffer();
    void FillTextBufferBisect();

    NTSTATUS GetTextBufferInfoInitResult()
    {
        return m_ntstatusTextBufferInfo;
    }

private:
    HANDLE m_heap;
    NTSTATUS m_ntstatusTextBufferInfo;
};
