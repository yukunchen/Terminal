/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "IApiRoutines.h"

// This exists as a base class until we're all migrated over.
[[nodiscard]]
HRESULT IApiRoutines::FillConsoleOutputAttributeImpl(IConsoleOutputObject& /*OutContext*/,
                                                     const WORD /*Attribute*/,
                                                     const DWORD /*LengthToWrite*/,
                                                     const COORD /*StartingCoordinate*/,
                                                     _Out_ DWORD* const /*pCellsModified*/)
{
    FAIL_FAST_HR(E_NOTIMPL);
}

[[nodiscard]]
HRESULT IApiRoutines::FillConsoleOutputCharacterAImpl(IConsoleOutputObject& /*OutContext*/,
                                                      const char /*Character*/,
                                                      const DWORD /*LengthToWrite*/,
                                                      const COORD /*StartingCoordinate*/,
                                                      _Out_ DWORD* const /*pCellsModified*/)
{
    FAIL_FAST_HR(E_NOTIMPL);
}

[[nodiscard]]
HRESULT IApiRoutines::FillConsoleOutputCharacterWImpl(IConsoleOutputObject&  /*OutContext*/,
                                                      const wchar_t /*Character*/,
                                                      const DWORD /*LengthToWrite*/,
                                                      const COORD /*StartingCoordinate*/,
                                                      _Out_ DWORD* const /*pCellsModified*/)
{
    FAIL_FAST_HR(E_NOTIMPL);
}

[[nodiscard]]
HRESULT IApiRoutines::ReadConsoleOutputAttributeImpl(const IConsoleOutputObject& /*OutContext*/,
                                                     const COORD* const /*pSourceOrigin*/,
                                                     _Out_writes_to_(_Param_(4), *_Param_(5)) WORD* const /*pAttributeBuffer*/,
                                                     const ULONG /*AttributeBufferLength*/,
                                                     _Out_ ULONG* const /*pAttributeBufferWritten*/)
{
    FAIL_FAST_HR(E_NOTIMPL);
}

[[nodiscard]]
HRESULT IApiRoutines::ReadConsoleOutputCharacterAImpl(const IConsoleOutputObject& /*OutContext*/,
                                                      const COORD* const /*pSourceOrigin*/,
                                                      _Out_writes_to_(_Param_(4), *_Param_(5)) char* const /*pTextBuffer*/,
                                                      const ULONG /*TextBufferLength*/,
                                                       _Out_ ULONG* const /*pTextBufferWritten*/)
{
    FAIL_FAST_HR(E_NOTIMPL);
}

[[nodiscard]]
HRESULT IApiRoutines::ReadConsoleOutputCharacterWImpl(const IConsoleOutputObject& /*OutContext*/,
                                                      const COORD* const /*pSourceOrigin*/,
                                                      _Out_writes_to_(_Param_(4), *_Param_(5)) wchar_t* const /*pTextBuffer*/,
                                                      const ULONG /*TextBufferLength*/,
                                                       _Out_ ULONG* const /*pTextBufferWritten*/)
{
    FAIL_FAST_HR(E_NOTIMPL);
}

[[nodiscard]]
HRESULT IApiRoutines::WriteConsoleInputAImpl(_In_ IConsoleInputObject* const /*pInContext*/,
                                             _In_reads_(_Param_(3)) const INPUT_RECORD* const /*pInputBuffer*/,
                                             const ULONG /*InputBufferLength*/,
                                             _Out_ ULONG* const /*pInputBufferRead*/)
{
    FAIL_FAST_HR(E_NOTIMPL);
}

[[nodiscard]]
HRESULT IApiRoutines::WriteConsoleInputWImpl(_In_ IConsoleInputObject* const /*pInContext*/,
                                             _In_reads_(_Param_(3)) const INPUT_RECORD* const /*pInputBuffer*/,
                                             const ULONG /*InputBufferLength*/,
                                             _Out_ ULONG* const /*pInputBufferRead*/)
{
    FAIL_FAST_HR(E_NOTIMPL);
}

[[nodiscard]]
HRESULT IApiRoutines::WriteConsoleOutputAImpl(IConsoleOutputObject& /*OutContext*/,
                                              _In_reads_(_Param_(3)->X * _Param_(3)->Y) const CHAR_INFO* const /*pTextBuffer*/,
                                              const COORD* const /*pTextBufferSize*/,
                                              const COORD* const /*pTextBufferSourceOrigin*/,
                                              const SMALL_RECT* const /*pTargetRectangle*/,
                                              _Out_ SMALL_RECT* const /*pAffectedRectangle*/)
{
    FAIL_FAST_HR(E_NOTIMPL);
}

[[nodiscard]]
HRESULT IApiRoutines::WriteConsoleOutputWImpl(IConsoleOutputObject& /*OutContext*/,
                                              _In_reads_(_Param_(3)->X * _Param_(3)->Y) const CHAR_INFO* const /*pTextBuffer*/,
                                              const COORD* const /*pTextBufferSize*/,
                                              const COORD* const /*pTextBufferSourceOrigin*/,
                                              const SMALL_RECT* const /*pTargetRectangle*/,
                                              _Out_ SMALL_RECT* const /*pAffectedRectangle*/)
{
    FAIL_FAST_HR(E_NOTIMPL);
}

[[nodiscard]]
HRESULT IApiRoutines::WriteConsoleOutputAttributeImpl(IConsoleOutputObject& /*OutContext*/,
                                                      _In_reads_(_Param_(3)) const WORD* const /*pAttributeBuffer*/,
                                                      const ULONG /*AttributeBufferLength*/,
                                                      const COORD* const /*pTargetOrigin*/,
                                                      _Out_ ULONG* const /*pAttributeBufferRead*/)
{
    FAIL_FAST_HR(E_NOTIMPL);
}

[[nodiscard]]
HRESULT IApiRoutines::WriteConsoleOutputCharacterAImpl(IConsoleOutputObject& /*OutContext*/,
                                                       _In_reads_(_Param_(3)) const char* const /*pTextBuffer*/,
                                                       const ULONG /*TextBufferLength*/,
                                                       const COORD* const /*pTargetOrigin*/,
                                                       _Out_ ULONG* const /*pTextBufferRead*/)
{
    FAIL_FAST_HR(E_NOTIMPL);
}

[[nodiscard]]
HRESULT IApiRoutines::WriteConsoleOutputCharacterWImpl(IConsoleOutputObject& /*OutContext*/,
                                                       _In_reads_(_Param_(3)) const wchar_t* const /*pTextBuffer*/,
                                                       const ULONG /*TextBufferLength*/,
                                                       const COORD* const /*pTargetOrigin*/,
                                                       _Out_ ULONG* const /*pTextBufferRead*/)
{
    FAIL_FAST_HR(E_NOTIMPL);
}

[[nodiscard]]
HRESULT IApiRoutines::ReadConsoleOutputA(const IConsoleOutputObject& /*OutContext*/,
                                         _Out_writes_(_Param_(3)->X * _Param_(3)->Y) CHAR_INFO* const /*pTextBuffer*/,
                                         const COORD* const /*pTextBufferSize*/,
                                         const COORD* const /*pTextBufferTargetOrigin*/,
                                         const SMALL_RECT* const /*pSourceRectangle*/,
                                          _Out_ SMALL_RECT* const /*pReadRectangle*/)
{
    FAIL_FAST_HR(E_NOTIMPL);
}

[[nodiscard]]
HRESULT IApiRoutines::ReadConsoleOutputW(const IConsoleOutputObject& /*OutContext*/,
                                         _Out_writes_(_Param_(3)->X * _Param_(3)->Y) CHAR_INFO* const /*pTextBuffer*/,
                                         const COORD* const /*pTextBufferSize*/,
                                         const COORD* const /*pTextBufferTargetOrigin*/,
                                         const SMALL_RECT* const /*pSourceRectangle*/,
                                          _Out_ SMALL_RECT* const /*pReadRectangle*/)
{
    FAIL_FAST_HR(E_NOTIMPL);
}
