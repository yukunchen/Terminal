/*++
Copyright (c) Microsoft Corporation

Module Name:
- AttrRow.hpp

Abstract:
- contains data structure for the attributes of one row of screen buffer

Author(s):
- Michael Niksa (miniksa) 10-Apr-2014
- Paul Campbell (paulcam) 10-Apr-2014

Revision History:
- From components of output.h/.c
  by Therese Stowell (ThereseS) 1990-1991
- Pulled into its own file from textBuffer.hpp/cpp (AustDi, 2017)
--*/

#pragma once

#include "TextAttributeRun.hpp"

class ATTR_ROW final
{
public:
    ATTR_ROW(const UINT cchRowWidth, const TextAttribute attr);
    ATTR_ROW(const ATTR_ROW& a);

    void Reset(const TextAttribute attr);

    TextAttribute GetAttrByColumn(const size_t column) const;
    TextAttribute GetAttrByColumn(const size_t column,
                                  size_t* const pApplies) const;

    size_t GetNumberOfRuns() const noexcept;

    size_t FindAttrIndex(const size_t index,
                         size_t* const pApplies) const;

    bool SetAttrToEnd(const UINT iStart, const TextAttribute attr);
    void ReplaceLegacyAttrs(const WORD wToBeReplacedAttr, const WORD wReplaceWith) noexcept;

    void Resize(const size_t newWidth);

    [[nodiscard]]
    HRESULT InsertAttrRuns(const std::vector<TextAttributeRun>& newAttrs,
                           const size_t iStart,
                           const size_t iEnd,
                           const size_t cBufferWidth);

    std::vector<TextAttribute> ATTR_ROW::UnpackAttrs() const;

    static std::vector<TextAttributeRun> PackAttrs(const std::vector<TextAttribute>& attrs);

    friend bool operator==(const ATTR_ROW& a, const ATTR_ROW& b) noexcept;

private:

    std::vector<TextAttributeRun> _list;
    size_t _cchRowWidth;


#ifdef UNIT_TESTING
    friend class AttrRowTests;
#endif

};
