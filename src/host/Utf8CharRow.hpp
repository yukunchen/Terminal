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
    using value_type = std::pair<std::vector<char>, DbcsAttribute>;
    using iterator = std::vector<value_type>::iterator;
    using const_iterator = std::vector<value_type>::const_iterator;

    Utf8CharRow(short rowWidth);
    Utf8CharRow(const Utf8CharRow& a) = default;
    Utf8CharRow& operator=(const Utf8CharRow& a);
    Utf8CharRow(Utf8CharRow&& a) noexcept = default;
    ~Utf8CharRow() = default;

    void swap(Utf8CharRow& other) noexcept;

    std::string GetText() const;

    // ICharRow methods
    ICharRow::SupportedEncoding GetSupportedEncoding() const noexcept override;


    friend constexpr bool operator==(const Utf8CharRow& a, const Utf8CharRow& b) noexcept;

private:
    using CharType = std::vector<char>;
    using StringType = std::string;

};

void swap(Utf8CharRow& a, Utf8CharRow& b) noexcept;

constexpr bool operator==(const Utf8CharRow& a, const Utf8CharRow& b) noexcept
{
    return (static_cast<const CharRowBase<Utf8CharRow::CharType, Utf8CharRow::StringType>&>(a) ==
            static_cast<const CharRowBase<Utf8CharRow::CharType, Utf8CharRow::StringType>&>(b));
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
