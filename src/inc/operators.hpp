/*++
Copyright (c) Microsoft Corporation

Module Name:
- operators.hpp

Abstract:
- This file contains helpful operator overloading for some older data structures.

Author(s):
- Austin Diviness (AustDi) Mar 2017
--*/

#pragma once

constexpr bool operator==(const COORD& a, const COORD& b) noexcept
{
    return (a.X == b.X &&
            a.Y == b.Y);
}

constexpr bool operator!=(const COORD& a, const COORD& b) noexcept
{
    return !(a == b);
}

constexpr bool operator==(const SMALL_RECT& a, const SMALL_RECT& b) noexcept
{
    return (a.Top == b.Top &&
            a.Left == b.Left &&
            a.Bottom == b.Bottom &&
            a.Right == b.Right);
}

constexpr bool operator!=(const SMALL_RECT& a, const SMALL_RECT& b) noexcept
{
    return !(a == b);
}

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
        size_t operator()(const COORD& coord) const noexcept
        {
            size_t retVal = coord.Y;
            retVal |= coord.X << (sizeof(coord.Y) * 8);
            return retVal;
        }
    };
}
