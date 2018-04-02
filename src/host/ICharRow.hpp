/*++
Copyright (c) Microsoft Corporation

Module Name:
- ICharRow.hpp

Abstract:
- interface for output buffer characters rows

Author(s):
- Austin Diviness (AustDi) 07-Feb-2018

Revision History:
--*/

#pragma once

#include "DbcsAttribute.hpp"

// the characters of one row of screen buffer
// we keep the following values so that we don't write
// more pixels to the screen than we have to:
// left is initialized to screenbuffer width.  right is
// initialized to zero.
//
//      [     foo.bar    12-12-61                       ]
//       ^    ^                  ^                     ^
//       |    |                  |                     |
//     Chars Left               Right                end of Chars buffer
class ICharRow
{
public:
    enum class SupportedEncoding
    {
        Ucs2,
        Utf8
    };

    virtual ~ICharRow() = 0;

    virtual SupportedEncoding GetSupportedEncoding() const noexcept = 0;
    virtual size_t size() const noexcept = 0;
    virtual void Reset() = 0;
    [[nodiscard]]
    virtual HRESULT Resize(_In_ const size_t newSize) noexcept = 0;
    virtual size_t MeasureRight() const = 0;
    virtual size_t MeasureLeft() const = 0;
    virtual void SetWrapForced(_In_ bool const fWrapWasForced) noexcept = 0;
    virtual bool WasWrapForced() const noexcept = 0;
    virtual void SetDoubleBytePadded(_In_ bool const fDoubleBytePadded) noexcept = 0;
    virtual bool WasDoubleBytePadded() const noexcept = 0;

    // cell
    virtual void ClearCell(_In_ const size_t column) = 0;

    // dbcs attribute
    virtual const DbcsAttribute& GetAttribute(_In_ const size_t column) const = 0;
    virtual DbcsAttribute& GetAttribute(_In_ const size_t column) = 0;

    // glyphs
    virtual void ClearGlyph(const size_t column) = 0;
    virtual bool ContainsText() const = 0;
    virtual std::wstring GetText() const = 0;
};

inline ICharRow::~ICharRow() {}
