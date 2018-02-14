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

    virtual ~ICharRow() = 0;

    virtual SupportedEncoding GetSupportedEncoding() const noexcept = 0;
    virtual size_t size() const noexcept = 0;
    virtual HRESULT Resize(_In_ const size_t newSize) = 0;
    virtual void Reset(_In_ const short newSize) = 0;
    virtual size_t MeasureRight() const = 0;
    virtual size_t MeasureLeft() const = 0;
    virtual void ClearCell(_In_ const size_t column) = 0;
    virtual bool ContainsText() const = 0;
    virtual const DbcsAttribute& GetAttribute(_In_ const size_t column) const = 0;
    virtual DbcsAttribute& GetAttribute(_In_ const size_t column) = 0;
    virtual void SetWrapForced(_In_ bool const fWrapWasForced) noexcept = 0;
    virtual bool WasWrapForced() const noexcept = 0;
    virtual void SetDoubleBytePadded(_In_ bool const fDoubleBytePadded) noexcept = 0;
    virtual bool WasDoubleBytePadded() const noexcept = 0;
};

inline ICharRow::~ICharRow() {}
