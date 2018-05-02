/*++
Copyright (c) Microsoft Corporation

Module Name:
- UnicodeStorage.hpp

Abstract:
- dynamic storage location for glyphs that can't normally fit in the output buffer

Author(s):
- Austin Diviness (AustDi) 02-May-2018
--*/

#pragma once

#include <vector>
#include <unordered_map>


class UnicodeStorage final
{
public:
    using key_type = typename unsigned short;
    using mapped_type = typename std::vector<wchar_t>;

    static UnicodeStorage& GetInstance()
    {
        static UnicodeStorage storage;
        return storage;
    }

    const mapped_type& GetText(const key_type key)
    {
        return _map.at(key);
    }

    const key_type StoreText(const std::vector<wchar_t>& text)
    {
        _map.emplace(_currentKey, text);
        auto retVal = _currentKey;
        ++_currentKey;
        return retVal;
    }

    void Erase(const key_type key)
    {
        _map.erase(key);
    }

private:
    UnicodeStorage() :
        _map{},
        _currentKey{ 1u }
    {
    }

    std::unordered_map<key_type, mapped_type> _map;
    key_type _currentKey;
};
