/*++
Copyright (c) Microsoft Corporation

Module Name:
- Row.hpp

Abstract:
- data structure for information associated with one row of screen buffer

Author(s):
- Michael Niksa (miniksa) 10-Apr-2014
- Paul Campbell (paulcam) 10-Apr-2014

Revision History:
- From components of output.h/.c
  by Therese Stowell (ThereseS) 1990-1991
- Pulled into its own file from textBuffer.hpp/cpp (AustDi, 2017)
--*/

#pragma once

#include "CharRow.hpp"
#include "AttrRow.hpp"

class ROW final
{
public:
    ROW(_In_ const SHORT rowId, _In_ const short rowWidth, _In_ const TextAttribute fillAttribute);
    ROW(const ROW& a);
    ROW& operator=(const ROW& a);
    ROW(ROW&& a) noexcept;

    void swap(ROW& other) noexcept;

    CHAR_ROW CharRow;
    ATTR_ROW AttrRow;
    SHORT sRowId;

    bool Reset(_In_ short const sRowWidth, _In_ const TextAttribute Attr);
    HRESULT Resize(_In_ size_t const width);

    void ClearColumn(_In_ const size_t column);

    friend constexpr bool operator==(const ROW& a, const ROW& b) noexcept;

#ifdef UNIT_TESTING
    friend class RowTests;
#endif
};

void swap(ROW& a, ROW& b) noexcept;
constexpr bool operator==(const ROW& a, const ROW& b) noexcept
{
    return (a.CharRow == b.CharRow &&
            a.AttrRow == b.AttrRow &&
            a.sRowId == b.sRowId);
}

// this sticks specialization of swap() into the std::namespace for Row, so that callers that use
// std::swap explicitly over calling the global swap can still get the performance benefit.
namespace std
{
    template<>
    inline void swap<ROW>(ROW& a, ROW& b) noexcept
    {
        a.swap(b);
    }
}
