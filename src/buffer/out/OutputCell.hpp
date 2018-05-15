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

#include <exception>
#include <variant>

class InvalidCharInfoConversionException : public std::exception
{
    const char* what() noexcept
    {
        return "Cannot convert to CHAR_INFO without explicit TextAttribute";
    }
};


class OutputCell final
{
public:
    enum class TextAttributeBehavior
    {
        Stored, // use contained text attribute
        Default, // use default text attribute at time of object instantiation
        Current, // use text attribute of cell being written to
    };

    static std::vector<OutputCell> FromUtf16(const std::vector<std::vector<wchar_t>>& utf16Glyphs);
    static std::vector<OutputCell> FromUtf16(const std::vector<std::vector<wchar_t>>& utf16Glyphs,
                                             const TextAttribute defaultTextAttribute);

    OutputCell(const std::vector<wchar_t>& charData,
               const DbcsAttribute dbcsAttribute,
               const TextAttributeBehavior behavior);

    OutputCell(const std::vector<wchar_t>& charData,
               const DbcsAttribute dbcsAttribute,
               const TextAttribute textAttribute);

    OutputCell(const CHAR_INFO& charInfo);

    void swap(_Inout_ OutputCell& other) noexcept;

    CHAR_INFO ToCharInfo();

    std::vector<wchar_t>& Chars() noexcept;
    DbcsAttribute& DbcsAttr() noexcept;
    TextAttribute& TextAttr();

    constexpr const std::vector<wchar_t>& Chars() const
    {
        return _charData;
    }

    constexpr const DbcsAttribute& DbcsAttr() const
    {
        return _dbcsAttribute;
    }

    const TextAttribute& TextAttr() const
    {
        THROW_HR_IF(E_INVALIDARG, _behavior == TextAttributeBehavior::Current);
        return _textAttribute;
    }

    constexpr TextAttributeBehavior TextAttrBehavior() const
    {
        return _behavior;
    }

    friend bool operator==(const OutputCell& a, const OutputCell& b) noexcept;

private:
    std::vector<wchar_t> _charData;
    DbcsAttribute _dbcsAttribute;
    TextAttribute _textAttribute;
    TextAttributeBehavior _behavior;

    void _setFromBehavior(const TextAttributeBehavior behavior);
    void _setFromCharInfo(const CHAR_INFO& charInfo);
    static std::vector<OutputCell> _fromUtf16(const std::vector<std::vector<wchar_t>>& utf16Glyphs,
                                              const std::variant<TextAttribute, TextAttributeBehavior> textAttrVal);
};

bool operator==(const OutputCell& a, const OutputCell& b) noexcept;
