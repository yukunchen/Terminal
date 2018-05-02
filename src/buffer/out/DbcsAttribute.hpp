/*++
Copyright (c) Microsoft Corporation

Module Name:
- DbcsAttribute.hpp

Abstract:
- helper class for storing double byte character set information about a cell in the output buffer.

Author(s):
- Austin Diviness (AustDi) 26-Jan-2018

Revision History:
--*/

#pragma once

class DbcsAttribute final
{
    enum class Attribute : BYTE
    {
        Single = 0x00,
        Leading = 0x01,
        Trailing = 0x02
    };

public:

    DbcsAttribute() noexcept :
        _attribute{ Attribute::Single },
        _mapSpecifier{ 0 }
    {
    }

    DbcsAttribute(const Attribute attribute) noexcept :
        _attribute{ attribute },
        _mapSpecifier{ 0 }
    {
    }

    constexpr bool IsSingle() const noexcept
    {
        return _attribute == Attribute::Single;
    }

    constexpr bool IsLeading() const noexcept
    {
        return _attribute == Attribute::Leading;
    }

    constexpr bool IsTrailing() const noexcept
    {
        return _attribute == Attribute::Trailing;
    }

    constexpr bool IsDbcs() const noexcept
    {
        return IsLeading() || IsTrailing();
    }

    constexpr bool IsStored() const noexcept
    {
        return _mapSpecifier != 0;
    }

    constexpr BYTE GetMapIndex() const noexcept
    {
        return _mapSpecifier;
    }

    void SetMapIndex(const BYTE index)
    {
        // the top two bits can't be set because of the bit shifting we will do
        FAIL_FAST_IF(index & 0xC0);
        _mapSpecifier = index;
    }

    void SetSingle() noexcept
    {
        _attribute = Attribute::Single;
    }

    void SetLeading() noexcept
    {
        _attribute = Attribute::Leading;
    }

    void SetTrailing() noexcept
    {
        _attribute = Attribute::Trailing;
    }

    void EraseMapIndex()
    {
        _mapSpecifier = 0;
    }

    WORD GeneratePublicApiAttributeFormat() const noexcept
    {
        WORD publicAttribute = 0;
        if (IsLeading())
        {
            SetFlag(publicAttribute, COMMON_LVB_LEADING_BYTE);
        }
        if (IsTrailing())
        {
            SetFlag(publicAttribute, COMMON_LVB_TRAILING_BYTE);
        }
        return publicAttribute;
    }

    static DbcsAttribute FromPublicApiAttributeFormat(WORD publicAttribute)
    {
        // it's not valid to be both a leading and trailing byte
        if (AreAllFlagsSet(publicAttribute, COMMON_LVB_LEADING_BYTE | COMMON_LVB_TRAILING_BYTE))
        {
            THROW_HR(E_INVALIDARG);
        }

        DbcsAttribute attr;
        if (IsFlagSet(publicAttribute, COMMON_LVB_LEADING_BYTE))
        {
            attr.SetLeading();
        }
        else if (IsFlagSet(publicAttribute, COMMON_LVB_TRAILING_BYTE))
        {
            attr.SetTrailing();
        }
        return attr;
    }

    friend constexpr bool operator==(const DbcsAttribute& a, const DbcsAttribute& b) noexcept;

private:
    Attribute _attribute : 2;
    BYTE _mapSpecifier : 6;

#ifdef UNIT_TESTING
    friend class TextBufferTests;
#endif
};

constexpr bool operator==(const DbcsAttribute& a, const DbcsAttribute& b) noexcept
{
    return a._attribute == b._attribute;
}

static_assert(sizeof(DbcsAttribute) == sizeof(BYTE), "DbcsAttribute should be one byte big. if this changes then it needs"
    " either an implicit conversion to a BYTE or an update to all places that assume it's a byte big");
