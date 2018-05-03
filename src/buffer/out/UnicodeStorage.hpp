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
    using key_type = typename COORD;
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

    const void StoreText(const key_type key, const std::vector<wchar_t>& text)
    {
        _map.emplace(key, text);
    }

    void Erase(const key_type key)
    {
        _map.erase(key);
    }

private:
    UnicodeStorage() :
        _map{}
    {
    }

    std::unordered_map<key_type, mapped_type> _map;
};
