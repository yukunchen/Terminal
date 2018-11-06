/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "IApiRoutines.h"

// This exists as a base class until we're all migrated over.

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
