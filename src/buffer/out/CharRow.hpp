/*++
Copyright (c) Microsoft Corporation

Module Name:
- CharRow.hpp

Abstract:
- contains data structure for UCS2 encoded character data of a row

Author(s):
- Michael Niksa (miniksa) 10-Apr-2014
- Paul Campbell (paulcam) 10-Apr-2014

Revision History:
- From components of output.h/.c
  by Therese Stowell (ThereseS) 1990-1991
- Pulled into its own file from textBuffer.hpp/cpp (AustDi, 2017)
--*/

#pragma once

#include "DbcsAttribute.hpp"
#include "ICharRow.hpp"
#include "CharRowReference.hpp"
#include "CharRowCell.hpp"

class CharRow final : public ICharRow
{
public:
    using glyph_type = typename wchar_t;
    using value_type = typename CharRowCell;
    using iterator = typename std::vector<value_type>::iterator;
    using const_iterator = typename std::vector<value_type>::const_iterator;
    using reference = typename CharRowReference;

    CharRow(size_t rowWidth);
    CharRow(const CharRow& a) = default;
    CharRow& operator=(const CharRow& a);
    CharRow(CharRow&& a) = default;
    ~CharRow() = default;

    void swap(CharRow& other) noexcept;

    // ICharRow methods
    void SetWrapForced(const bool wrap) noexcept override;
    bool WasWrapForced() const noexcept override;
    void SetDoubleBytePadded(const bool doubleBytePadded) noexcept override;
    bool WasDoubleBytePadded() const noexcept override;
    size_t size() const noexcept override;
    void Reset() override;
    [[nodiscard]]
    HRESULT Resize(const size_t newSize) noexcept override;
    size_t MeasureLeft() const override;
    size_t MeasureRight() const noexcept override;
    void ClearCell(const size_t column) override;
    bool ContainsText() const noexcept override;
    const DbcsAttribute& DbcsAttrAt(const size_t column) const override;
    DbcsAttribute& DbcsAttrAt(const size_t column) override;
    void ClearGlyph(const size_t column) override;
    std::wstring GetText() const override;
    ICharRow::SupportedEncoding GetSupportedEncoding() const noexcept override;

    // other functions implemented at the template class level
    std::wstring GetTextRaw() const;

    // working with glyphs
    const reference GlyphAt(const size_t column) const;
    reference GlyphAt(const size_t column);

    // iterators
    iterator begin() noexcept;
    const_iterator cbegin() const noexcept;

    iterator end() noexcept;
    const_iterator cend() const noexcept;

    friend CharRowReference;
    friend constexpr bool operator==(const CharRow& a, const CharRow& b) noexcept;

protected:
    // Occurs when the user runs out of text in a given row and we're forced to wrap the cursor to the next line
    bool _wrapForced;

    // Occurs when the user runs out of text to support a double byte character and we're forced to the next line
    bool _doubleBytePadded;

    // storage for glyph data and dbcs attributes
    std::vector<value_type> _data;
};

void swap(CharRow& a, CharRow& b) noexcept;

constexpr bool operator==(const CharRow& a, const CharRow& b) noexcept
{
    return (a._wrapForced == b._wrapForced &&
            a._doubleBytePadded == b._doubleBytePadded &&
            a._data == b._data);
}

template<typename InputIt1, typename InputIt2>
void OverwriteColumns(InputIt1 startChars, InputIt1 endChars, InputIt2 startAttrs, CharRow::iterator outIt)
{
    std::transform(startChars,
                   endChars,
                   startAttrs,
                   outIt,
                   [](const wchar_t wch, const DbcsAttribute attr)
    {
        return CharRow::value_type{ wch, attr };
    });
}

// this sticks specialization of swap() into the std::namespace for CharRow, so that callers that use
// std::swap explicitly over calling the global swap can still get the performance benefit.
namespace std
{
    template<>
    inline void swap<CharRow>(CharRow& a, CharRow& b) noexcept
    {
        a.swap(b);
    }
}
