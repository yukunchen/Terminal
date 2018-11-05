/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "IApiRoutines.h"

// This exists as a base class until we're all migrated over.

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
