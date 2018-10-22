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
#include "OutputCellView.hpp"

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
    static std::vector<OutputCell> FromUtf16(const std::vector<std::vector<wchar_t>>& utf16Glyphs);
    
    OutputCell() = default;

    OutputCell(const std::wstring_view charData,
               const DbcsAttribute dbcsAttribute,
               const TextAttributeBehavior behavior);

    OutputCell(const std::wstring_view charData,
               const DbcsAttribute dbcsAttribute,
               const TextAttribute textAttribute);

    OutputCell(const CHAR_INFO& charInfo);
    OutputCell(const OutputCellView& view);

    OutputCell(const OutputCell&) = default;
    OutputCell(OutputCell&&) = default;

    OutputCell& operator=(const OutputCell&) = default;
    OutputCell& operator=(OutputCell&&) = default;

    ~OutputCell() = default;

    CHAR_INFO ToCharInfo();

    const std::wstring_view Chars() const noexcept;
    void SetChars(const std::wstring_view chars);

    DbcsAttribute& DbcsAttr() noexcept;
    TextAttribute& TextAttr();

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
    wchar_t _singleChar;
    std::vector<wchar_t> _charData;
    DbcsAttribute _dbcsAttribute;
    TextAttribute _textAttribute;
    TextAttributeBehavior _behavior;

    bool _useSingle() const noexcept;
    void _setFromBehavior(const TextAttributeBehavior behavior);
    void _setFromCharInfo(const CHAR_INFO& charInfo);
    void _setFromStringView(const std::wstring_view view);
    void _setFromOutputCellView(const OutputCellView& cell);
    static std::vector<OutputCell> _fromUtf16(const std::vector<std::vector<wchar_t>>& utf16Glyphs,
                                              const std::variant<TextAttribute, TextAttributeBehavior> textAttrVal);
};

bool operator==(const OutputCell& a, const OutputCell& b) noexcept;
