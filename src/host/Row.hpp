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

#include "ICharRow.hpp"
#include "AttrRow.hpp"
#include "OutputCell.hpp"

#include <memory>
#include <vector>

class ROW final
{
public:
    ROW(_In_ const SHORT rowId, _In_ const short rowWidth, _In_ const TextAttribute fillAttribute);
    ROW(const ROW& a);
    ROW& operator=(const ROW& a);
    ROW(ROW&& a) noexcept;
    ~ROW() = default;

    void swap(ROW& other) noexcept;
    size_t size() const;
    const OutputCell at(const size_t column) const;

    const ICharRow& GetCharRow() const;
    ICharRow& GetCharRow();

    const ATTR_ROW& GetAttrRow() const;
    ATTR_ROW& GetAttrRow();

    SHORT GetId() const noexcept;
    void SetId(_In_ const SHORT id);

    bool Reset(_In_ const TextAttribute Attr);
    [[nodiscard]]
    HRESULT Resize(_In_ size_t const width);

    void ClearColumn(_In_ const size_t column);
    std::vector<OutputCell> AsCells() const;
    std::vector<OutputCell> AsCells(_In_ const size_t startIndex) const;
    std::vector<OutputCell> AsCells(_In_ const size_t startIndex, _In_ const size_t count) const;

    friend bool operator==(const ROW& a, const ROW& b) noexcept;

#ifdef UNIT_TESTING
    friend class RowTests;
#endif

private:
    std::unique_ptr<ICharRow> _charRow;
    ATTR_ROW _attrRow;
    SHORT _id;
    size_t _rowWidth;

};

void swap(ROW& a, ROW& b) noexcept;
inline bool operator==(const ROW& a, const ROW& b) noexcept
{
    return (a._charRow == b._charRow &&
            a._attrRow == b._attrRow &&
            a._id == b._id);
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
