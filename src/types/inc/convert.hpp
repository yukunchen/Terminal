/*++
Copyright (c) Microsoft Corporation

Module Name:
- convert.hpp

Abstract:
- Defines functions for converting between A and W text strings.
    Largely taken from misc.h.

Author:
- Mike Griese (migrie) 30-Oct-2017

--*/

#pragma once
#include <deque>
#include <memory>

HRESULT ConvertToW(_In_ const UINT uiCodePage,
                   _In_reads_or_z_(cchSource) const char* const rgchSource,
                   _In_ size_t const cchSource,
                   _Inout_ wistd::unique_ptr<wchar_t[]>& pwsTarget,
                   _Out_ size_t& cchTarget);

HRESULT ConvertToA(_In_ const UINT uiCodePage,
                   _In_reads_or_z_(cchSource) const wchar_t* const rgwchSource,
                   _In_ size_t cchSource,
                   _Inout_ wistd::unique_ptr<char[]>& psTarget,
                   _Out_ size_t& cchTarget);

HRESULT GetALengthFromW(_In_ const UINT uiCodePage,
                        _In_reads_or_z_(cchSource) const wchar_t* const rgwchSource,
                        _In_ size_t const cchSource,
                        _Out_ size_t* const pcchTarget);

HRESULT GetUShortByteCount(_In_ size_t cchUnicode,
                           _Out_ USHORT* const pcb);

HRESULT GetDwordByteCount(_In_ size_t cchUnicode,
                          _Out_ DWORD* const pcb);

std::deque<char> ConvertToOem(_In_ const UINT codepage,
                              _In_ const std::wstring& source);
