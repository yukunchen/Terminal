/*++
Copyright (c) Microsoft Corporation

Module Name:
- cpp2017.hpp

Abstract:
- This file contains features of C++ 2017 that we don't have yet. It can be removed when we move to newer compilers.

Author(s):
- Austin Diviness (AustDi) May 2017
- Michael Niksa (MiNiksa) May 2017
--*/

// returns val if (low <= val <= high)
// returns low if (val < low)
// returns high if (val > high)
template<typename T> T clamp(T val, T low, T high)
{
    assert(low <= high);
    return max(low, min(val, high));
}

template<typename T>
constexpr auto size(const T& t) noexcept(noexcept(t.size())) -> decltype(t.size())
{
    return t.size();
}

template<typename T, std::size_t ArraySize>
constexpr std::size_t size(T (&)[ArraySize]) noexcept
{
    return ArraySize;
}
