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

class ICharRow
{
public:
    virtual ~ICharRow() = 0;

    virtual size_t size() const noexcept = 0;
    virtual void Reset() = 0;
    [[nodiscard]]
    virtual HRESULT Resize(const size_t newSize) noexcept = 0;
    virtual size_t MeasureRight() const = 0;
    virtual size_t MeasureLeft() const = 0;
    virtual void SetWrapForced(const bool fWrapWasForced) noexcept = 0;
    virtual bool WasWrapForced() const noexcept = 0;
    virtual void SetDoubleBytePadded(const bool fDoubleBytePadded) noexcept = 0;
    virtual bool WasDoubleBytePadded() const noexcept = 0;

    // cell
    virtual void ClearCell(const size_t column) = 0;

    // dbcs attribute
    virtual const DbcsAttribute& DbcsAttrAt(const size_t column) const = 0;
    virtual DbcsAttribute& DbcsAttrAt(const size_t column) = 0;

    // glyphs
    virtual void ClearGlyph(const size_t column) = 0;
    virtual bool ContainsText() const = 0;
    virtual std::wstring GetText() const = 0;
};

inline ICharRow::~ICharRow() {}
