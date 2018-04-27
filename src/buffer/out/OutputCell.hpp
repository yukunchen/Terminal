/*++
Copyright (c) Microsoft Corporation

Module Name:
- OutputCell.hpp

Abstract:
- Representation of all data stored in a cell of the output buffer.
- RGB color supported.

Author:
- Austin Diviness (AustDi) 20-Mar-2018

--*/

#pragma once

#include "DbcsAttribute.hpp"
#include "TextAttribute.hpp"

class OutputCell final
{
public:
    OutputCell(const wchar_t charData,
               const DbcsAttribute dbcsAttribute,
               const TextAttribute textAttribute) noexcept;

    OutputCell(const std::vector<wchar_t>& charData,
               const DbcsAttribute dbcsAttribute,
               const TextAttribute textAttribute);

    void swap(_Inout_ OutputCell& other) noexcept;

    CHAR_INFO ToCharInfo();

    std::vector<wchar_t>& Chars() noexcept;
    DbcsAttribute& DbcsAttr() noexcept;
    TextAttribute& TextAttr() noexcept;

    constexpr const std::vector<wchar_t>& Chars() const
    {
        return _charData;
    }

    constexpr const DbcsAttribute& DbcsAttr() const
    {
        return _dbcsAttribute;
    }

    constexpr const TextAttribute& TextAttr() const
    {
        return _textAttribute;
    }

private:
    std::vector<wchar_t> _charData;
    DbcsAttribute _dbcsAttribute;
    TextAttribute _textAttribute;
};
