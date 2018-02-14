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

class Utf8CharRow final : public CharRowBase
{
public:
    using value_type = std::pair<std::vector<char>, DbcsAttribute>;
    using iterator = std::vector<value_type>::iterator;
    using const_iterator = std::vector<value_type>::const_iterator;

    Utf8CharRow(short rowWidth);
    Utf8CharRow(const Utf8CharRow& a);
    Utf8CharRow& operator=(const Utf8CharRow& a);
    Utf8CharRow(Utf8CharRow&& a) noexcept;
    ~Utf8CharRow();

    void swap(Utf8CharRow& other) noexcept;

    // ICharRow methods
    ICharRow::SupportedEncoding GetSupportedEncoding() const noexcept override;
    size_t size() const noexcept override;
    HRESULT Resize(_In_ size_t const newSize) override;
    void Reset(_In_ short const sRowWidth) override;
    size_t MeasureLeft() const override;
    size_t MeasureRight() const override;
    void ClearCell(_In_ const size_t column) override;
    bool ContainsText() const override;
    const DbcsAttribute& GetAttribute(_In_ const size_t column) const override;
    DbcsAttribute& GetAttribute(_In_ const size_t column) override;

    // iterators
    iterator begin() noexcept;
    const_iterator cbegin() const noexcept;

    iterator end() noexcept;
    const_iterator cend() const noexcept;

    friend constexpr bool operator==(const Utf8CharRow& a, const Utf8CharRow& b) noexcept;

private:
    std::vector<value_type> _data;
};

void swap(Utf8CharRow& a, Utf8CharRow& b) noexcept;

constexpr bool operator==(const Utf8CharRow& a, const Utf8CharRow& b) noexcept
{
    return (static_cast<const CharRowBase&>(a) == static_cast<const CharRowBase&>(b) &&
            a._data == b._data);
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
