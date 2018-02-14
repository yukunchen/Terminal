/*++
Copyright (c) Microsoft Corporation

Module Name:
- CharRowBase.hpp

Abstract:
- base class for output buffer characters rows

Author(s):
- Austin Diviness (AustDi) 13-Feb-2018

Revision History:
--*/

#pragma once

#include "ICharRow.hpp"

class CharRowBase : public ICharRow
{
public:
    CharRowBase();
    virtual ~CharRowBase() = 0;

    void swap(CharRowBase& other) noexcept;

    virtual void SetWrapForced(_In_ bool const wrap) noexcept;
    virtual bool WasWrapForced() const noexcept;
    virtual void SetDoubleBytePadded(_In_ bool const doubleBytePadded) noexcept;
    virtual bool WasDoubleBytePadded() const noexcept;

    friend constexpr bool operator==(const CharRowBase& a, const CharRowBase& b) noexcept;
protected:
    // Occurs when the user runs out of text in a given row and we're forced to wrap the cursor to the next line
    bool _wrapForced;

    // Occurs when the user runs out of text to support a double byte character and we're forced to the next line
    bool _doubleBytePadded;
};

inline CharRowBase::~CharRowBase() {}

void swap(CharRowBase& a, CharRowBase& b) noexcept;

constexpr bool operator==(const CharRowBase& a, const CharRowBase& b) noexcept
{
    return (a._wrapForced == b._wrapForced &&
            a._doubleBytePadded == b._doubleBytePadded);
}

// this sticks specialization of swap() into the std::namespace for CharRowBase, so that callers that use
// std::swap explicitly over calling the global swap can still get the performance benefit.
namespace std
{
    template<>
    inline void swap<CharRowBase>(CharRowBase& a, CharRowBase& b) noexcept
    {
        a.swap(b);
    }
}
