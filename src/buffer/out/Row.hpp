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

#include "AttrRow.hpp"
#include "OutputCell.hpp"
#include "CharRow.hpp"
#include "UnicodeStorage.hpp"

class TextBuffer;

class ROW final
{
public:
    ROW(const SHORT rowId, const short rowWidth, const TextAttribute fillAttribute, TextBuffer* const pParent);
    ROW(const ROW& a);
    ROW& operator=(const ROW& a);
    ROW(ROW&& a) noexcept;
    ~ROW() = default;

    void swap(ROW& other) noexcept;
    size_t size() const noexcept;
    const OutputCell at(const size_t column) const;

    const CharRow& GetCharRow() const;
    CharRow& GetCharRow();

    const ATTR_ROW& GetAttrRow() const noexcept;
    ATTR_ROW& GetAttrRow() noexcept;

    SHORT GetId() const noexcept;
    void SetId(const SHORT id) noexcept;

    bool Reset(const TextAttribute Attr);
    [[nodiscard]]
    HRESULT Resize(const size_t width);

    void ClearColumn(const size_t column);
    std::wstring GetText() const;
    std::vector<OutputCell> AsCells() const;
    std::vector<OutputCell> AsCells(const size_t startIndex) const;
    std::vector<OutputCell> AsCells(const size_t startIndex, const size_t count) const;

    UnicodeStorage& GetUnicodeStorage();
    const UnicodeStorage& GetUnicodeStorage() const;

    // Routine Description:
    // - writes cell data to the row
    // Arguments:
    // - start - starting iterator to cells to write
    // - end - ending iterator to cells to write
    // - index - column in row to start writing at
    // Return Value:
    // - iterator to first cell that was not written to this row. will be equal to end if all cells were written
    // to row
    template <typename InputIt>
    InputIt WriteCells(InputIt start, InputIt end, const size_t index)
    {
        THROW_HR_IF(E_INVALIDARG, index >= _charRow.size());
        auto it = start;
        size_t currentIndex = index;
        while (it != end && currentIndex < _charRow.size())
        {
            _charRow.DbcsAttrAt(currentIndex) = it->DbcsAttr();
            _charRow.GlyphAt(currentIndex) = it->Chars();
            if (it->TextAttrBehavior() != OutputCell::TextAttributeBehavior::Current)
            {
                const TextAttributeRun attrRun{ 1, it->TextAttr() };
                const std::vector<TextAttributeRun> runs{ attrRun };
                LOG_IF_FAILED(_attrRow.InsertAttrRuns(runs,
                                                    currentIndex,
                                                    currentIndex,
                                                    _charRow.size()));
            }

            ++it;
            ++currentIndex;
        }
        return it;
    }

    friend bool operator==(const ROW& a, const ROW& b) noexcept;

#ifdef UNIT_TESTING
    friend class RowTests;
#endif

private:
    CharRow _charRow;
    ATTR_ROW _attrRow;
    SHORT _id;
    size_t _rowWidth;
    TextBuffer* _pParent; // non ownership pointer
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
