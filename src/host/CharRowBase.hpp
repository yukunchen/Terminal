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

    CharRowBase(_In_ const size_t rowWidth, _In_ const GlyphType defaultValue);
    virtual ~CharRowBase() = default;

    void swap(CharRowBase& other) noexcept;

    // ICharRow methods
    void SetWrapForced(_In_ bool const wrap) noexcept;
    bool WasWrapForced() const noexcept;
    void SetDoubleBytePadded(_In_ bool const doubleBytePadded) noexcept;
    bool WasDoubleBytePadded() const noexcept;
    size_t size() const noexcept override;
    // TODO does Reset also resize the row?
    void Reset(_In_ short const sRowWidth);
    HRESULT Resize(_In_ const size_t newSize) noexcept;
    size_t MeasureLeft() const override;
    size_t MeasureRight() const override;
    void ClearCell(_In_ const size_t column) override;
    bool ContainsText() const override;
    const DbcsAttribute& GetAttribute(_In_ const size_t column) const override;
    DbcsAttribute& GetAttribute(_In_ const size_t column) override;
    void ClearGlyph(const size_t column);


    virtual StringType GetText() const = 0;

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

    const GlyphType _defaultValue;

    std::vector<value_type> _data;
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

// this sticks specialization of swap() into the std::namespace for CharRowBase, so that callers that use
// std::swap explicitly over calling the global swap can still get the performance benefit.
namespace std
{
    /*
      // TODO
    template<>
    inline void swap<CharRowBase<GlyphType, StringType>>(CharRowBase<GlyphType, StringType>& a, CharRowBase<GlyphType, StringType>& b) noexcept
    {
        a.swap(b);
    }
    */
}

#include "CharRowBaseImpl.hpp"
