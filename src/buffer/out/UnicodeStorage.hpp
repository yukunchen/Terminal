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

// std::unordered_map needs help to know how to hash a COORD
namespace std
{
    template <>
    struct hash<COORD>
    {

        // Routine Description:
        // - hashes a coord. coord will be hashed by storing the x and y values consecutively in the lower
        // bits of a size_t.
        // Arguments:
        // - coord - the coord to hash
        // Return Value:
        // - the hashed coord
        constexpr size_t operator()(const COORD& coord) const noexcept
        {
            size_t retVal = coord.Y;
            retVal |= coord.X << (sizeof(coord.Y) * 8);
            return retVal;
        }
    };
}

class UnicodeStorage final
{
public:
    using key_type = typename COORD;
    using mapped_type = typename std::vector<wchar_t>;

    UnicodeStorage() :
        _map{}
    {
    }

    // Routine Description:
    // - fetches the text associated with key
    // Arguments:
    // - key - the key into the storage
    // Return Value:
    // - the glyph data associated with key
    // Note: will throw exception if key is not stored yet
    const mapped_type& GetText(const key_type key) const
    {
        return _map.at(key);
    }

    // Routine Description:
    // - stores glyph data associated with key.
    // Arguments:
    // - key - the key into the storage
    // - glyph - the glyph data to store
    void StoreGlyph(const key_type key, const mapped_type& glyph)
    {
        _map.emplace(key, glyph);
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
    std::unordered_map<key_type, mapped_type> _map;
};
