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
    enum class SupportedEncoding
    {
        Ucs2
    };

    enum class RowFlags
    {
        NoFlags = 0x0,
        WrapForced = 0x1, // Occurs when the user runs out of text in a given row and we're forced to wrap the cursor to the next line
        DoubleBytePadded = 0x2, // Occurs when the user runs out of text to support a double byte character and we're forced to the next line
    };

    virtual ~ICharRow() = 0;

    virtual SupportedEncoding GetSupportedEncoding() const noexcept = 0;
    virtual size_t size() const noexcept = 0;
    virtual HRESULT Resize(_In_ const size_t newSize) = 0;
    virtual void Reset(_In_ const short newSize) = 0;
    virtual size_t MeasureRight() const = 0;
    virtual size_t MeasureLeft() const = 0;
    virtual void ClearCell(_In_ const size_t column) = 0;
    virtual void SetWrapStatus(_In_ bool const fWrapWasForced) = 0;
    virtual bool WasWrapForced() const = 0;
    virtual bool ContainsText() const = 0;
    virtual const DbcsAttribute& GetAttribute(_In_ const size_t column) const = 0;
    virtual DbcsAttribute& GetAttribute(_In_ const size_t column) = 0;
    virtual void SetDoubleBytePadded(_In_ bool const fDoubleBytePadded) = 0;
    virtual bool WasDoubleBytePadded() const = 0;
};

inline ICharRow::~ICharRow() {}
