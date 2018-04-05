/*++
Copyright (c) Microsoft Corporation

Module Name:
- CharRowBase.hpp

Abstract:
- base class for output buffer characters rows

Author(s):
- Austin Diviness (AustDi) 13-Feb-2018

Revision History:
--*/

#pragma once

#include "ICharRow.hpp"

template <typename GlyphType, typename StringType>
class CharRowBase : public ICharRow
{
public:
    using value_type = typename std::pair<GlyphType, DbcsAttribute>;
    using iterator = typename std::vector<value_type>::iterator;
    using const_iterator = typename std::vector<value_type>::const_iterator;

    // template type params are saved here for use by child classes
    using glyph_type = typename GlyphType;
    using string_type = typename StringType;

    CharRowBase(const size_t rowWidth, const GlyphType defaultValue);
    virtual ~CharRowBase() = default;

    void swap(CharRowBase& other) noexcept;

    // ICharRow methods
    void SetWrapForced(const bool wrap) noexcept;
    bool WasWrapForced() const noexcept;
    void SetDoubleBytePadded(const bool doubleBytePadded) noexcept;
    bool WasDoubleBytePadded() const noexcept;
    size_t size() const noexcept override;
    void Reset();
    [[nodiscard]]
    HRESULT Resize(const size_t newSize) noexcept;
    size_t MeasureLeft() const override;
    size_t MeasureRight() const override;
    void ClearCell(const size_t column) override;
    bool ContainsText() const override;
    const DbcsAttribute& GetAttribute(const size_t column) const override;
    DbcsAttribute& GetAttribute(const size_t column) override;
    void ClearGlyph(const size_t column);
    std::wstring GetText() const override;

    // other functions implemented at the template class level
    StringType GetTextRaw() const;

    // working with glyphs
    const GlyphType& GetGlyphAt(const size_t column) const;
    GlyphType& GetGlyphAt(const size_t column);

    // iterators
    iterator begin() noexcept;
    const_iterator cbegin() const noexcept;

    iterator end() noexcept;
    const_iterator cend() const noexcept;

    friend constexpr bool operator==(const CharRowBase& a, const CharRowBase& b) noexcept;

protected:
    // Occurs when the user runs out of text in a given row and we're forced to wrap the cursor to the next line
    bool _wrapForced;

    // Occurs when the user runs out of text to support a double byte character and we're forced to the next line
    bool _doubleBytePadded;

    // default glyph value, used for reseting the character data portion of a cell
    const GlyphType _defaultValue;

    // storage for glyph data and dbcs attributes
    std::vector<value_type> _data;

#ifdef UNIT_TESTING
    friend class CharRowBaseTests;
#endif
};

template <typename GlyphType, typename StringType>
void swap(CharRowBase<GlyphType, StringType>& a, CharRowBase<GlyphType, StringType>& b) noexcept;

template <typename GlyphType, typename StringType>
constexpr bool operator==(const CharRowBase<GlyphType, StringType>& a, const CharRowBase<GlyphType, StringType>& b) noexcept
{
    return (a._wrapForced == b._wrapForced &&
            a._doubleBytePadded == b._doubleBytePadded &&
            a._defaultValue == b._defaultValue &&
            a._data == b._data);
}

#include "CharRowBaseImpl.hpp"
