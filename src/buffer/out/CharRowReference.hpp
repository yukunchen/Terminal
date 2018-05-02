/*++
Copyright (c) Microsoft Corporation

Module Name:
- CharRowReference.hpp

Abstract:
- reference class for the glyph data of a char row cell

Author(s):
- Austin Diviness (AustDi) 02-May-2018
--*/

#pragma once

#include "DbcsAttribute.hpp"
#include "CharRowCell.hpp"
#include <utility>

class CharRow;

class CharRowReference final
{
public:

    using const_iterator = const wchar_t*;

    CharRowReference(CharRow& parent, const size_t index) :
        _parent{ parent },
        _index{ index }
    {
    }
    CharRowReference(const CharRowReference&) = default;
    CharRowReference(CharRowReference&&) = default;

    void operator=(const std::vector<wchar_t>& chars);
    operator std::vector<wchar_t>() const;

    const_iterator begin() const;
    const_iterator end() const;


    friend bool operator==(const CharRowReference& ref, const std::vector<wchar_t>& glyph);
    friend bool operator==(const std::vector<wchar_t>& glyph, const CharRowReference& ref);

private:
    CharRow& _parent;
    const size_t _index;
    wchar_t _lastData;

    CharRowCell& _cellData();
    const CharRowCell& _cellData() const;

    std::vector<wchar_t> _glyphData() const;
};

bool operator==(const CharRowReference& ref, const std::vector<wchar_t>& glyph);
bool operator==(const std::vector<wchar_t>& glyph, const CharRowReference& ref);
