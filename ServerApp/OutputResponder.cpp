#include "stdafx.h"
#include "DriverResponder.h"

// Handles outputing to the console
// Eg, when applications write to output, the console handles them here.

NTSTATUS DriverResponder::WriteConsoleAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                              _In_reads_(TextBufferLength) char* const pTextBuffer,
                                              _In_ ULONG const TextBufferLength,
                                              _Out_ ULONG* const pTextBufferRead)
{
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pTextBuffer);
    *pTextBufferRead = TextBufferLength;
    return 0;
}

NTSTATUS DriverResponder::WriteConsoleWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                              _In_reads_(TextBufferLength) wchar_t* const pTextBuffer,
                                              _In_ ULONG const TextBufferLength,
                                              _Out_ ULONG* const pTextBufferRead)
{
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pTextBuffer);
    *pTextBufferRead = TextBufferLength;
    return 0;
}