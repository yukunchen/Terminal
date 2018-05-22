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

enum class CodepointWidth : BYTE
{
    Narrow,
    Wide,
    Ambiguous, // could be narrow or wide depending on the current codepage and font
    Invalid // not a valid unicode codepoint
};

[[nodiscard]]
HRESULT ConvertToW(const UINT uiCodePage,
                   _In_reads_or_z_(cchSource) const char* const rgchSource,
                   const size_t cchSource,
                   _Inout_ wistd::unique_ptr<wchar_t[]>& pwsTarget,
                   _Out_ size_t& cchTarget);

[[nodiscard]]
HRESULT ConvertToA(const UINT uiCodePage,
                   _In_reads_or_z_(cchSource) const wchar_t* const rgwchSource,
                   _In_ size_t cchSource,
                   _Inout_ wistd::unique_ptr<char[]>& psTarget,
                   _Out_ size_t& cchTarget);

[[nodiscard]]
HRESULT GetALengthFromW(const UINT uiCodePage,
                        _In_reads_or_z_(cchSource) const wchar_t* const rgwchSource,
                        const size_t cchSource,
                        _Out_ size_t* const pcchTarget);

[[nodiscard]]
HRESULT GetUShortByteCount(_In_ size_t cchUnicode,
                           _Out_ USHORT* const pcb);

[[nodiscard]]
HRESULT GetDwordByteCount(_In_ size_t cchUnicode,
                          _Out_ DWORD* const pcb);

std::deque<char> ConvertToOem(const UINT codepage,
                              const std::wstring& source);

std::deque<std::unique_ptr<KeyEvent>> CharToKeyEvents(const wchar_t wch, const unsigned int codepage);

std::deque<std::unique_ptr<KeyEvent>> SynthesizeKeyboardEvents(const wchar_t wch,
                                                               const short keyState);

std::deque<std::unique_ptr<KeyEvent>> SynthesizeNumpadEvents(const wchar_t wch, const unsigned int codepage);

CodepointWidth GetCharWidth(const wchar_t wch) noexcept;

wchar_t Utf16ToUcs2(const std::vector<wchar_t>& charData);
