#include "stdafx.h"
#include "DriverResponder.h"

// Handles input (from the user/keyboard) to the console
// When an application wants to read from stdin, these functions handle it.


NTSTATUS DriverResponder::PeekConsoleInputAImpl(_In_ IConsoleInputObject* const pInContext,
                                                  _Out_writes_to_(InputRecordsBufferLength, *pRecordsWritten) INPUT_RECORD* const pInputRecordsBuffer,
                                                  _In_ ULONG const InputRecordsBufferLength,
                                                  _Out_ ULONG* const pRecordsWritten)
{
    UNREFERENCED_PARAMETER(pInContext);
    UNREFERENCED_PARAMETER(pInputRecordsBuffer);
    UNREFERENCED_PARAMETER(InputRecordsBufferLength);
    *pRecordsWritten = 0;
    return 0;
}

NTSTATUS DriverResponder::PeekConsoleInputWImpl(_In_ IConsoleInputObject* const pInContext,
                                                  _Out_writes_to_(InputRecordsBufferLength, *pRecordsWritten) INPUT_RECORD* const pInputRecordsBuffer,
                                                  _In_ ULONG const InputRecordsBufferLength,
                                                  _Out_ ULONG* const pRecordsWritten)
{
    UNREFERENCED_PARAMETER(pInContext);
    UNREFERENCED_PARAMETER(pInputRecordsBuffer);
    UNREFERENCED_PARAMETER(InputRecordsBufferLength);
    *pRecordsWritten = 0;
    return 0;
}

NTSTATUS DriverResponder::ReadConsoleInputAImpl(_In_ IConsoleInputObject* const pInContext,
                                                  _Out_writes_to_(InputRecordsBufferLength, *pRecordsWritten) INPUT_RECORD* const pInputRecordsBuffer,
                                                  _In_ ULONG const InputRecordsBufferLength,
                                                  _Out_ ULONG* const pRecordsWritten)
{
    UNREFERENCED_PARAMETER(pInContext);
    UNREFERENCED_PARAMETER(pInputRecordsBuffer);
    UNREFERENCED_PARAMETER(InputRecordsBufferLength);
    *pRecordsWritten = 0;
    return 0;
}

NTSTATUS DriverResponder::ReadConsoleInputWImpl(_In_ IConsoleInputObject* const pInContext,
                                                  _Out_writes_to_(InputRecordsBufferLength, *pRecordsWritten) INPUT_RECORD* const pInputRecordsBuffer,
                                                  _In_ ULONG const InputRecordsBufferLength,
                                                  _Out_ ULONG* const pRecordsWritten)
{
    UNREFERENCED_PARAMETER(pInContext);
    UNREFERENCED_PARAMETER(pInputRecordsBuffer);
    UNREFERENCED_PARAMETER(InputRecordsBufferLength);
    *pRecordsWritten = 0;
    return 0;
}

NTSTATUS DriverResponder::ReadConsoleAImpl(_In_ IConsoleInputObject* const pInContext,
                                             _Out_writes_to_(TextBufferLength, *pTextBufferWritten) char* const pTextBuffer,
                                             _In_ ULONG const TextBufferLength,
                                             _Out_ ULONG* const pTextBufferWritten,
                                             _In_opt_ CONSOLE_READCONSOLE_CONTROL* const pReadControl)
{
    if (false)
    {
        if (TextBufferLength < 9)
        {
            return ERROR_INSUFFICIENT_BUFFER;
        }
        else
        {
            pTextBuffer[0] = 's';
            pTextBuffer[1] = 't';
            pTextBuffer[2] = 'a';
            pTextBuffer[3] = 'r';
            pTextBuffer[4] = 't';
            pTextBuffer[5] = ' ';
            pTextBuffer[6] = '.';
            pTextBuffer[7] = '\r';
            pTextBuffer[8] = '\n';
            *pTextBufferWritten = 9;
            // g_hasInput = false;
            return S_OK;
        }
    }
    else
    {
        UNREFERENCED_PARAMETER(pInContext);
        UNREFERENCED_PARAMETER(pTextBuffer);
        UNREFERENCED_PARAMETER(TextBufferLength);
        *pTextBufferWritten = 0;
        UNREFERENCED_PARAMETER(pReadControl);
        return STATUS_PENDING;
    }
}

NTSTATUS DriverResponder::ReadConsoleWImpl(_In_ IConsoleInputObject* const pInContext,
                                             _Out_writes_to_(TextBufferLength, *pTextBufferWritten) wchar_t* const pTextBuffer,
                                             _In_ ULONG const TextBufferLength,
                                             _Out_ ULONG* const pTextBufferWritten,
                                             _In_opt_ CONSOLE_READCONSOLE_CONTROL* const pReadControl)
{
    if (false)
    {
        if (TextBufferLength < 9)
        {
            return ERROR_INSUFFICIENT_BUFFER;
        }
        else
        {
            pTextBuffer[0] = L's';
            pTextBuffer[1] = L't';
            pTextBuffer[2] = L'a';
            pTextBuffer[3] = L'r';
            pTextBuffer[4] = L't';
            pTextBuffer[5] = L' ';
            pTextBuffer[6] = L'.';
            pTextBuffer[7] = L'\r';
            pTextBuffer[8] = L'\n';
            *pTextBufferWritten = 9;
            // g_hasInput = false;
            return S_OK;
        }
    }
    else
    {
        UNREFERENCED_PARAMETER(pInContext);
        UNREFERENCED_PARAMETER(pTextBuffer);
        UNREFERENCED_PARAMETER(TextBufferLength);
        *pTextBufferWritten = 0;
        UNREFERENCED_PARAMETER(pReadControl);
        return ERROR_IO_PENDING;
    }
}



NTSTATUS DriverResponder::WriteConsoleInputAImpl(_In_ IConsoleInputObject* const pInContext,
                                                   _In_reads_(InputBufferLength) const INPUT_RECORD* const pInputBuffer,
                                                   _In_ ULONG const InputBufferLength,
                                                   _Out_ ULONG* const pInputBufferRead)
{
    UNREFERENCED_PARAMETER(pInContext);
    UNREFERENCED_PARAMETER(pInputBuffer);
    *pInputBufferRead = InputBufferLength;
    return 0;
}

NTSTATUS DriverResponder::WriteConsoleInputWImpl(_In_ IConsoleInputObject* const pInContext,
                                                   _In_reads_(InputBufferLength) const INPUT_RECORD* const pInputBuffer,
                                                   _In_ ULONG const InputBufferLength,
                                                   _Out_ ULONG* const pInputBufferRead)
{
    UNREFERENCED_PARAMETER(pInContext);
    UNREFERENCED_PARAMETER(pInputBuffer);
    *pInputBufferRead = InputBufferLength;
    return 0;
}