/*++
Copyright (c) Microsoft Corporation

Module Name:
- Utf8CharRow.hpp

Abstract:
- contains data structure for UTF8 encoded character data of a row

Author(s):
- Austin Diviness (AustDi) 13-Feb-2018

Revision History:
--*/

#pragma once

#include "ICharRow.hpp"
#include "CharRowBase.hpp"

#include <vector>

class Utf8CharRow final : public CharRowBase<std::vector<char>, std::string>
{
public:
    Utf8CharRow(size_t rowWidth);
    Utf8CharRow(const Utf8CharRow& a) = default;
    Utf8CharRow& operator=(const Utf8CharRow& a);
    Utf8CharRow(Utf8CharRow&& a) noexcept = default;
    ~Utf8CharRow() = default;

    void swap(Utf8CharRow& other) noexcept;

    // ICharRow methods
    ICharRow::SupportedEncoding GetSupportedEncoding() const noexcept override;

    // CharRowBase methods
    string_type GetText() const;

    friend constexpr bool operator==(const Utf8CharRow& a, const Utf8CharRow& b) noexcept;
};

void swap(Utf8CharRow& a, Utf8CharRow& b) noexcept;

constexpr bool operator==(const Utf8CharRow& a, const Utf8CharRow& b) noexcept
{
    return (static_cast<const CharRowBase<Utf8CharRow::glyph_type, Utf8CharRow::string_type>&>(a) ==
            static_cast<const CharRowBase<Utf8CharRow::glyph_type, Utf8CharRow::string_type>&>(b));
}

// this sticks specialization of swap() into the std::namespace for Utf8CharRow, so that callers that use
// std::swap explicitly over calling the global swap can still get the performance benefit.
namespace std
{
    template<>
    inline void swap<Utf8CharRow>(Utf8CharRow& a, Utf8CharRow& b) noexcept
    {
        a.swap(b);
    }
}
