/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "IApiRoutines.h"

// This exists as a base class until we're all migrated over.
[[nodiscard]]
HRESULT IApiRoutines::FillConsoleOutputAttributeImpl(_In_ IConsoleOutputObject* const /*pOutContext*/,
                                                     _In_ WORD const /*Attribute*/,
                                                     _In_ DWORD const LengthToWrite,
                                                     _In_ COORD const /*StartingCoordinate*/,
                                                     _Out_ DWORD* const pCellsModified)
{
    assert(false);
    *pCellsModified = LengthToWrite;
    return S_OK;
}

[[nodiscard]]
HRESULT IApiRoutines::FillConsoleOutputCharacterAImpl(_In_ IConsoleOutputObject* const /*pOutContext*/,
                                                      _In_ char const /*Character*/,
                                                      _In_ DWORD const LengthToWrite,
                                                      _In_ COORD const /*StartingCoordinate*/,
                                                      _Out_ DWORD* const pCellsModified)
{
    assert(false);
    *pCellsModified = LengthToWrite;
    return S_OK;
}

[[nodiscard]]
HRESULT IApiRoutines::FillConsoleOutputCharacterWImpl(_In_ IConsoleOutputObject* const /*pOutContext*/,
                                                      _In_ wchar_t const /*Character*/,
                                                      _In_ DWORD const LengthToWrite,
                                                      _In_ COORD const /*StartingCoordinate*/,
                                                      _Out_ DWORD* const pCellsModified)
{
    assert(false);
    *pCellsModified = LengthToWrite;
    return S_OK;
}

[[nodiscard]]
HRESULT IApiRoutines::ReadConsoleOutputAttributeImpl(_In_ IConsoleOutputObject* const /*pOutContext*/,
                                                     const COORD* const /*pSourceOrigin*/,
                                                     _Out_writes_to_(_Param_(4), *_Param_(5)) WORD* const /*pAttributeBuffer*/,
                                                     _In_ ULONG const /*AttributeBufferLength*/,
                                                     _Out_ ULONG* const pAttributeBufferWritten)
{
    assert(false);
    *pAttributeBufferWritten = 0;
    return S_OK;
}

[[nodiscard]]
HRESULT IApiRoutines::ReadConsoleOutputCharacterAImpl(_In_ IConsoleOutputObject* const /*pOutContext*/,
                                                      const COORD* const /*pSourceOrigin*/,
                                                      _Out_writes_to_(_Param_(4), *_Param_(5)) char* const /*pTextBuffer*/,
                                                      _In_ ULONG const /*TextBufferLength*/,
                                                       _Out_ ULONG* const pTextBufferWritten)
{
    assert(false);
    *pTextBufferWritten = 0;
    return S_OK;
}

[[nodiscard]]
HRESULT IApiRoutines::ReadConsoleOutputCharacterWImpl(_In_ IConsoleOutputObject* const /*pOutContext*/,
                                                      const COORD* const /*pSourceOrigin*/,
                                                      _Out_writes_to_(_Param_(4), *_Param_(5)) wchar_t* const /*pTextBuffer*/,
                                                      _In_ ULONG const /*TextBufferLength*/,
                                                       _Out_ ULONG* const pTextBufferWritten)
{
    assert(false);
    *pTextBufferWritten = 0;
    return S_OK;
}

[[nodiscard]]
HRESULT IApiRoutines::WriteConsoleInputAImpl(_In_ IConsoleInputObject* const /*pInContext*/,
                                             _In_reads_(InputBufferLength) const INPUT_RECORD* const /*pInputBuffer*/,
                                             _In_ ULONG const InputBufferLength,
                                             _Out_ ULONG* const pInputBufferRead)
{
    assert(false);
    *pInputBufferRead = InputBufferLength;
    return S_OK;
}

[[nodiscard]]
HRESULT IApiRoutines::WriteConsoleInputWImpl(_In_ IConsoleInputObject* const /*pInContext*/,
                                             _In_reads_(InputBufferLength) const INPUT_RECORD* const /*pInputBuffer*/,
                                             _In_ ULONG const InputBufferLength,
                                             _Out_ ULONG* const pInputBufferRead)
{
    assert(false);
    *pInputBufferRead = InputBufferLength;
    return S_OK;
}

[[nodiscard]]
HRESULT IApiRoutines::WriteConsoleOutputAImpl(_In_ IConsoleOutputObject* const /*pOutContext*/,
                                              _In_reads_(_Param_(3)->X * _Param_(3)->Y) const CHAR_INFO* const /*pTextBuffer*/,
                                              const COORD* const /*pTextBufferSize*/,
                                              const COORD* const /*pTextBufferSourceOrigin*/,
                                              const SMALL_RECT* const /*pTargetRectangle*/,
                                              _Out_ SMALL_RECT* const pAffectedRectangle)
{
    assert(false);
    *pAffectedRectangle = { 0 };
    return S_OK;
}

[[nodiscard]]
HRESULT IApiRoutines::WriteConsoleOutputWImpl(_In_ IConsoleOutputObject* const /*pOutContext*/,
                                              _In_reads_(_Param_(3)->X * _Param_(3)->Y) const CHAR_INFO* const /*pTextBuffer*/,
                                              const COORD* const /*pTextBufferSize*/,
                                              const COORD* const /*pTextBufferSourceOrigin*/,
                                              const SMALL_RECT* const /*pTargetRectangle*/,
                                              _Out_ SMALL_RECT* const pAffectedRectangle)
{
    assert(false);
    *pAffectedRectangle = { 0 };
    return S_OK;
}

[[nodiscard]]
HRESULT IApiRoutines::WriteConsoleOutputAttributeImpl(_In_ IConsoleOutputObject* const /*pOutContext*/,
                                                      _In_reads_(AttributeBufferLength) const WORD* const /*pAttributeBuffer*/,
                                                      _In_ ULONG const AttributeBufferLength,
                                                      const COORD* const /*pTargetOrigin*/,
                                                      _Out_ ULONG* const pAttributeBufferRead)
{
    assert(false);
    *pAttributeBufferRead = AttributeBufferLength;
    return S_OK;
}

[[nodiscard]]
HRESULT IApiRoutines::WriteConsoleOutputCharacterAImpl(_In_ IConsoleOutputObject* const /*pOutContext*/,
                                                       _In_reads_(TextBufferLength) const char* const /*pTextBuffer*/,
                                                       _In_ ULONG const TextBufferLength,
                                                       const COORD* const /*pTargetOrigin*/,
                                                       _Out_ ULONG* const pTextBufferRead)
{
    assert(false);
    *pTextBufferRead = TextBufferLength;
    return S_OK;
}

[[nodiscard]]
HRESULT IApiRoutines::WriteConsoleOutputCharacterWImpl(_In_ IConsoleOutputObject* const /*pOutContext*/,
                                                       _In_reads_(TextBufferLength) const wchar_t* const /*pTextBuffer*/,
                                                       _In_ ULONG const TextBufferLength,
                                                       const COORD* const /*pTargetOrigin*/,
                                                       _Out_ ULONG* const pTextBufferRead)
{
    assert(false);
    *pTextBufferRead = TextBufferLength;
    return S_OK;
}

[[nodiscard]]
HRESULT IApiRoutines::ReadConsoleOutputA(_In_ IConsoleOutputObject* const /*pOutContext*/,
                                         _Out_writes_(_Param_(3)->X * _Param_(3)->Y) CHAR_INFO* const /*pTextBuffer*/,
                                         const COORD* const /*pTextBufferSize*/,
                                         const COORD* const /*pTextBufferTargetOrigin*/,
                                         const SMALL_RECT* const /*pSourceRectangle*/,
                                          _Out_ SMALL_RECT* const pReadRectangle)
{
    assert(false);
    *pReadRectangle = { 0, 0, -1, -1 }; // because inclusive rects, an empty has right and bottom at -1
    return S_OK;
}

[[nodiscard]]
HRESULT IApiRoutines::ReadConsoleOutputW(_In_ IConsoleOutputObject* const /*pOutContext*/,
                                         _Out_writes_(_Param_(3)->X * _Param_(3)->Y) CHAR_INFO* const /*pTextBuffer*/,
                                         const COORD* const /*pTextBufferSize*/,
                                         const COORD* const /*pTextBufferTargetOrigin*/,
                                         const SMALL_RECT* const /*pSourceRectangle*/,
                                          _Out_ SMALL_RECT* const pReadRectangle)
{
    assert(false);
    *pReadRectangle = { 0, 0, -1, -1 }; // because inclusive rects, an empty has right and bottom at -1
    return S_OK;
}
