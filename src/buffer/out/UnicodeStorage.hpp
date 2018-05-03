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

    // Routine Description:
    // - gets the singleton instance of UnicodeStorage
    // Return Value:
    // - the single instance of unicode storage
    static UnicodeStorage& GetInstance()
    {
        static UnicodeStorage storage;
        return storage;
    }

    // Routine Description:
    // - fetches the text associated with key
    // Arguments:
    // - key - the key into the storage
    // Return Value:
    // - the glyph data associated with key
    // Note: will throw exception if key is not stored yet
    const mapped_type& GetText(const key_type key)
    {
        return _map.at(key);
    }

    // Routine Description:
    // - stores glyph data associated with key.
    // Arguments:
    // - key - the key into the storage
    // - text - the glyph data to store
    const void StoreText(const key_type key, const std::vector<wchar_t>& text)
    {
        _map.emplace(key, text);
    }

    // Routine Description:
    // - erases key and it's associated data from the storage
    // Arguments:
    // - key - the key to remove
    void Erase(const key_type key) noexcept
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
