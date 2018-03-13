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
#include "IInputEvent.hpp"

[[nodiscard]]
HRESULT ConvertToW(_In_ const UINT uiCodePage,
                   _In_reads_or_z_(cchSource) const char* const rgchSource,
                   _In_ size_t const cchSource,
                   _Inout_ wistd::unique_ptr<wchar_t[]>& pwsTarget,
                   _Out_ size_t& cchTarget);

[[nodiscard]]
HRESULT ConvertToA(_In_ const UINT uiCodePage,
                   _In_reads_or_z_(cchSource) const wchar_t* const rgwchSource,
                   _In_ size_t cchSource,
                   _Inout_ wistd::unique_ptr<char[]>& psTarget,
                   _Out_ size_t& cchTarget);

[[nodiscard]]
HRESULT GetALengthFromW(_In_ const UINT uiCodePage,
                        _In_reads_or_z_(cchSource) const wchar_t* const rgwchSource,
                        _In_ size_t const cchSource,
                        _Out_ size_t* const pcchTarget);

[[nodiscard]]
HRESULT GetUShortByteCount(_In_ size_t cchUnicode,
                           _Out_ USHORT* const pcb);

[[nodiscard]]
HRESULT GetDwordByteCount(_In_ size_t cchUnicode,
                          _Out_ DWORD* const pcb);

std::deque<char> ConvertToOem(_In_ const UINT codepage,
                              _In_ const std::wstring& source);

std::deque<std::unique_ptr<KeyEvent>> CharToKeyEvents(_In_ const wchar_t wch, _In_ const unsigned int codepage);

std::deque<std::unique_ptr<KeyEvent>> SynthesizeKeyboardEvents(_In_ const wchar_t wch,
                                                               _In_ const short keyState);

std::deque<std::unique_ptr<KeyEvent>> SynthesizeNumpadEvents(_In_ const wchar_t wch, _In_ const unsigned int codepage);

[[nodiscard]]
HRESULT IsCharFullWidth(_In_ wchar_t wch, _Out_ bool* const isFullWidth);
