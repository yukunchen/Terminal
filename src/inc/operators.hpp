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

inline bool operator==(const COORD& a, const COORD& b)
{
    return (a.X == b.X &&
            a.X == b.Y);
}

inline bool operator!=(const COORD& a, const COORD& b)
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
