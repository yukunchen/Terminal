/*++
Copyright (c) Microsoft Corporation

Module Name:
- Utf16Parser.hpp

Abstract:
- Parser for grouping together utf16 surrogate pairs from a string of utf16 encoded text

Author(s):
- Austin Diviness (AustDi) 25-Apr-2018

--*/

#pragma once

#include <vector>
#include <optional>

class Utf16Parser final
{
public:
    static std::vector<std::vector<wchar_t>> Parse(const std::wstring& wstr);

private:
    wchar_t _partialSequence;

    static bool _isLeadingSurrogate(const wchar_t wch) noexcept;
    static bool _isTrailingSurrogate(const wchar_t wch) noexcept;
};
