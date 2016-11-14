/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "IApiRoutines.h"

// This exists as a base class until we're all migrated over.

HRESULT IApiRoutines::PeekConsoleInputAImpl(_In_ IConsoleInputObject* const pInContext,
                                             _Out_writes_to_(InputRecordsBufferLength, *pRecordsWritten) INPUT_RECORD* const pInputRecordsBuffer,
                                             _In_ ULONG const InputRecordsBufferLength,
                                             _Out_ ULONG* const pRecordsWritten)
{
    assert(false);
    UNREFERENCED_PARAMETER(pInContext);
    UNREFERENCED_PARAMETER(pInputRecordsBuffer);
    UNREFERENCED_PARAMETER(InputRecordsBufferLength);
    *pRecordsWritten = 0;
    return S_OK;
}

HRESULT IApiRoutines::PeekConsoleInputWImpl(_In_ IConsoleInputObject* const pInContext,
                                             _Out_writes_to_(InputRecordsBufferLength, *pRecordsWritten) INPUT_RECORD* const pInputRecordsBuffer,
                                             _In_ ULONG const InputRecordsBufferLength,
                                             _Out_ ULONG* const pRecordsWritten)
{
    assert(false);
    UNREFERENCED_PARAMETER(pInContext);
    UNREFERENCED_PARAMETER(pInputRecordsBuffer);
    UNREFERENCED_PARAMETER(InputRecordsBufferLength);
    *pRecordsWritten = 0;
    return S_OK;
}

HRESULT IApiRoutines::ReadConsoleInputAImpl(_In_ IConsoleInputObject* const pInContext,
                                             _Out_writes_to_(InputRecordsBufferLength, *pRecordsWritten) INPUT_RECORD* const pInputRecordsBuffer,
                                             _In_ ULONG const InputRecordsBufferLength,
                                             _Out_ ULONG* const pRecordsWritten)
{
    assert(false);
    UNREFERENCED_PARAMETER(pInContext);
    UNREFERENCED_PARAMETER(pInputRecordsBuffer);
    UNREFERENCED_PARAMETER(InputRecordsBufferLength);
    *pRecordsWritten = 0;
    return S_OK;
}

HRESULT IApiRoutines::ReadConsoleInputWImpl(_In_ IConsoleInputObject* const pInContext,
                                             _Out_writes_to_(InputRecordsBufferLength, *pRecordsWritten) INPUT_RECORD* const pInputRecordsBuffer,
                                             _In_ ULONG const InputRecordsBufferLength,
                                             _Out_ ULONG* const pRecordsWritten)
{
    assert(false);
    UNREFERENCED_PARAMETER(pInContext);
    UNREFERENCED_PARAMETER(pInputRecordsBuffer);
    UNREFERENCED_PARAMETER(InputRecordsBufferLength);
    *pRecordsWritten = 0;
    return S_OK;
}

HRESULT IApiRoutines::ReadConsoleAImpl(_In_ IConsoleInputObject* const pInContext,
                                        _Out_writes_to_(TextBufferLength, *pTextBufferWritten) char* const pTextBuffer,
                                        _In_ ULONG const TextBufferLength,
                                        _Out_ ULONG* const pTextBufferWritten,
                                        _In_opt_ CONSOLE_READCONSOLE_CONTROL* const pReadControl)
{
    assert(false);
    UNREFERENCED_PARAMETER(pInContext);
    UNREFERENCED_PARAMETER(pTextBuffer);
    UNREFERENCED_PARAMETER(TextBufferLength);
    *pTextBufferWritten = 0;
    UNREFERENCED_PARAMETER(pReadControl);
    return S_OK;
}

HRESULT IApiRoutines::ReadConsoleWImpl(_In_ IConsoleInputObject* const pInContext,
                                        _Out_writes_to_(TextBufferLength, *pTextBufferWritten) wchar_t* const pTextBuffer,
                                        _In_ ULONG const TextBufferLength,
                                        _Out_ ULONG* const pTextBufferWritten,
                                        _In_opt_ CONSOLE_READCONSOLE_CONTROL* const pReadControl)
{
    assert(false);
    UNREFERENCED_PARAMETER(pInContext);
    UNREFERENCED_PARAMETER(pTextBuffer);
    UNREFERENCED_PARAMETER(TextBufferLength);
    *pTextBufferWritten = 0;
    UNREFERENCED_PARAMETER(pReadControl);
    return S_OK;
}

HRESULT IApiRoutines::WriteConsoleAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                         _In_reads_(TextBufferLength) char* const pTextBuffer,
                                         _In_ ULONG const TextBufferLength,
                                         _Out_ ULONG* const pTextBufferRead)
{
    assert(false);
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pTextBuffer);
    *pTextBufferRead = TextBufferLength;
    return S_OK;
}

HRESULT IApiRoutines::WriteConsoleWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                         _In_reads_(TextBufferLength) wchar_t* const pTextBuffer,
                                         _In_ ULONG const TextBufferLength,
                                         _Out_ ULONG* const pTextBufferRead)
{
    assert(false);
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pTextBuffer);
    *pTextBufferRead = TextBufferLength;
    return S_OK;
}

HRESULT IApiRoutines::FillConsoleOutputAttributeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                      _In_ WORD const Attribute,
                                                      _In_ DWORD const LengthToWrite,
                                                      _In_ COORD const StartingCoordinate,
                                                      _Out_ DWORD* const pCellsModified)
{
    assert(false);
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(Attribute);
    UNREFERENCED_PARAMETER(StartingCoordinate);
    *pCellsModified = LengthToWrite;
    return S_OK;
}

HRESULT IApiRoutines::FillConsoleOutputCharacterAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                       _In_ char const Character,
                                                       _In_ DWORD const LengthToWrite,
                                                       _In_ COORD const StartingCoordinate,
                                                       _Out_ DWORD* const pCellsModified)
{
    assert(false);
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(Character);
    UNREFERENCED_PARAMETER(StartingCoordinate);
    *pCellsModified = LengthToWrite;
    return S_OK;
}

HRESULT IApiRoutines::FillConsoleOutputCharacterWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                       _In_ wchar_t const Character,
                                                       _In_ DWORD const LengthToWrite,
                                                       _In_ COORD const StartingCoordinate,
                                                       _Out_ DWORD* const pCellsModified)
{
    assert(false);
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(Character);
    UNREFERENCED_PARAMETER(StartingCoordinate);
    *pCellsModified = LengthToWrite;
    return S_OK;
}

HRESULT IApiRoutines::ReadConsoleOutputAttributeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                      _In_ const COORD* const pSourceOrigin,
                                                      _Out_writes_to_(AttributeBufferLength, *pAttributeBufferWritten) WORD* const pAttributeBuffer,
                                                      _In_ ULONG const AttributeBufferLength,
                                                      _Out_ ULONG* const pAttributeBufferWritten)
{
    assert(false);
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pSourceOrigin);
    UNREFERENCED_PARAMETER(pAttributeBuffer);
    UNREFERENCED_PARAMETER(AttributeBufferLength);
    *pAttributeBufferWritten = 0;
    return S_OK;
}

HRESULT IApiRoutines::ReadConsoleOutputCharacterAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                       _In_ const COORD* const pSourceOrigin,
                                                       _Out_writes_to_(TextBufferLength, *pTextBufferWritten) char* const pTextBuffer,
                                                       _In_ ULONG const TextBufferLength,
                                                       _Out_ ULONG* const pTextBufferWritten)
{
    assert(false);
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pSourceOrigin);
    UNREFERENCED_PARAMETER(pTextBuffer);
    UNREFERENCED_PARAMETER(TextBufferLength);
    *pTextBufferWritten = 0;
    return S_OK;
}

HRESULT IApiRoutines::ReadConsoleOutputCharacterWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                       _In_ const COORD* const pSourceOrigin,
                                                       _Out_writes_to_(TextBufferLength, *pTextBufferWritten) wchar_t* const pTextBuffer,
                                                       _In_ ULONG const TextBufferLength,
                                                       _Out_ ULONG* const pTextBufferWritten)
{
    assert(false);
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pSourceOrigin);
    UNREFERENCED_PARAMETER(pTextBuffer);
    UNREFERENCED_PARAMETER(TextBufferLength);
    *pTextBufferWritten = 0;
    return S_OK;
}

HRESULT IApiRoutines::WriteConsoleInputAImpl(_In_ IConsoleInputObject* const pInContext,
                                              _In_reads_(InputBufferLength) const INPUT_RECORD* const pInputBuffer,
                                              _In_ ULONG const InputBufferLength,
                                              _Out_ ULONG* const pInputBufferRead)
{
    assert(false);
    UNREFERENCED_PARAMETER(pInContext);
    UNREFERENCED_PARAMETER(pInputBuffer);
    *pInputBufferRead = InputBufferLength;
    return S_OK;
}

HRESULT IApiRoutines::WriteConsoleInputWImpl(_In_ IConsoleInputObject* const pInContext,
                                              _In_reads_(InputBufferLength) const INPUT_RECORD* const pInputBuffer,
                                              _In_ ULONG const InputBufferLength,
                                              _Out_ ULONG* const pInputBufferRead)
{
    assert(false);
    UNREFERENCED_PARAMETER(pInContext);
    UNREFERENCED_PARAMETER(pInputBuffer);
    *pInputBufferRead = InputBufferLength;
    return S_OK;
}

HRESULT IApiRoutines::WriteConsoleOutputAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                               _In_reads_(pTextBufferSize->X * pTextBufferSize->Y) const CHAR_INFO* const pTextBuffer,
                                               _In_ const COORD* const pTextBufferSize,
                                               _In_ const COORD* const pTextBufferSourceOrigin,
                                               _In_ const SMALL_RECT* const pTargetRectangle,
                                               _Out_ SMALL_RECT* const pAffectedRectangle)
{
    assert(false);
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pTextBuffer);
    UNREFERENCED_PARAMETER(pTextBufferSize);
    UNREFERENCED_PARAMETER(pTextBufferSourceOrigin);
    UNREFERENCED_PARAMETER(pTargetRectangle);
    *pAffectedRectangle = { 0 };
    return S_OK;
}

HRESULT IApiRoutines::WriteConsoleOutputWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                               _In_reads_(pTextBufferSize->X * pTextBufferSize->Y) const CHAR_INFO* const pTextBuffer,
                                               _In_ const COORD* const pTextBufferSize,
                                               _In_ const COORD* const pTextBufferSourceOrigin,
                                               _In_ const SMALL_RECT* const pTargetRectangle,
                                               _Out_ SMALL_RECT* const pAffectedRectangle)
{
    assert(false);
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pTextBuffer);
    UNREFERENCED_PARAMETER(pTextBufferSize);
    UNREFERENCED_PARAMETER(pTextBufferSourceOrigin);
    UNREFERENCED_PARAMETER(pTargetRectangle);
    *pAffectedRectangle = { 0 };
    return S_OK;
}

HRESULT IApiRoutines::WriteConsoleOutputAttributeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                       _In_reads_(AttributeBufferLength) const WORD* const pAttributeBuffer,
                                                       _In_ ULONG const AttributeBufferLength,
                                                       _In_ const COORD* const pTargetOrigin,
                                                       _Out_ ULONG* const pAttributeBufferRead)
{
    assert(false);
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pAttributeBuffer);
    UNREFERENCED_PARAMETER(pTargetOrigin);
    *pAttributeBufferRead = AttributeBufferLength;
    return S_OK;
}

HRESULT IApiRoutines::WriteConsoleOutputCharacterAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                        _In_reads_(TextBufferLength) const char* const pTextBuffer,
                                                        _In_ ULONG const TextBufferLength,
                                                        _In_ const COORD* const pTargetOrigin,
                                                        _Out_ ULONG* const pTextBufferRead)
{
    assert(false);
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pTextBuffer);
    UNREFERENCED_PARAMETER(pTargetOrigin);
    *pTextBufferRead = TextBufferLength;
    return S_OK;
}

HRESULT IApiRoutines::WriteConsoleOutputCharacterWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                        _In_reads_(TextBufferLength) const wchar_t* const pTextBuffer,
                                                        _In_ ULONG const TextBufferLength,
                                                        _In_ const COORD* const pTargetOrigin,
                                                        _Out_ ULONG* const pTextBufferRead)
{
    assert(false);
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pTextBuffer);
    UNREFERENCED_PARAMETER(pTargetOrigin);
    *pTextBufferRead = TextBufferLength;
    return S_OK;
}

HRESULT IApiRoutines::ReadConsoleOutputA(_In_ IConsoleOutputObject* const pOutContext,
                                          _Out_writes_(pTextBufferSize->X * pTextBufferSize->Y) CHAR_INFO* const pTextBuffer,
                                          _In_ const COORD* const pTextBufferSize,
                                          _In_ const COORD* const pTextBufferTargetOrigin,
                                          _In_ const SMALL_RECT* const pSourceRectangle,
                                          _Out_ SMALL_RECT* const pReadRectangle)
{
    assert(false);
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pTextBuffer);
    UNREFERENCED_PARAMETER(pTextBufferSize);
    UNREFERENCED_PARAMETER(pTextBufferTargetOrigin);
    UNREFERENCED_PARAMETER(pSourceRectangle);
    *pReadRectangle = { 0, 0, -1, -1 }; // because inclusive rects, an empty has right and bottom at -1
    return S_OK;
}

HRESULT IApiRoutines::ReadConsoleOutputW(_In_ IConsoleOutputObject* const pOutContext,
                                          _Out_writes_(pTextBufferSize->X * pTextBufferSize->Y) CHAR_INFO* const pTextBuffer,
                                          _In_ const COORD* const pTextBufferSize,
                                          _In_ const COORD* const pTextBufferTargetOrigin,
                                          _In_ const SMALL_RECT* const pSourceRectangle,
                                          _Out_ SMALL_RECT* const pReadRectangle)
{
    assert(false);
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pTextBuffer);
    UNREFERENCED_PARAMETER(pTextBufferSize);
    UNREFERENCED_PARAMETER(pTextBufferTargetOrigin);
    UNREFERENCED_PARAMETER(pSourceRectangle);
    *pReadRectangle = { 0, 0, -1, -1 }; // because inclusive rects, an empty has right and bottom at -1
    return S_OK;
}
