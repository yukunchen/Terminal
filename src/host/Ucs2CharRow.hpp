/*++
Copyright (c) Microsoft Corporation

Module Name:
- Ucs2CharRow.hpp

Abstract:
- contains data structure for UCS2 encoded character data of a row

Author(s):
- Michael Niksa (miniksa) 10-Apr-2014
- Paul Campbell (paulcam) 10-Apr-2014

Revision History:
- From components of output.h/.c
  by Therese Stowell (ThereseS) 1990-1991
- Pulled into its own file from textBuffer.hpp/cpp (AustDi, 2017)
--*/

#pragma once

#include <memory>
#include <vector>

#include "DbcsAttribute.hpp"
#include "ICharRow.hpp"
#include "CharRowBase.hpp"

// Characters used for padding out the buffer with invalid/empty space
#define PADDING_CHAR UNICODE_SPACE



// the characters of one row of screen buffer
// we keep the following values so that we don't write
// more pixels to the screen than we have to:
// left is initialized to screenbuffer width.  right is
// initialized to zero.
//
//      [     foo.bar    12-12-61                       ]
//       ^    ^                  ^                     ^
//       |    |                  |                     |
//     Chars Left               Right                end of Chars buffer
class Ucs2CharRow final : public CharRowBase<wchar_t, std::wstring>
{
public:
    Ucs2CharRow(short rowWidth);
    Ucs2CharRow(const Ucs2CharRow& a) = default;
    Ucs2CharRow& operator=(const Ucs2CharRow& a);
    Ucs2CharRow(Ucs2CharRow&& a) noexcept = default;
    ~Ucs2CharRow() = default;

    void swap(Ucs2CharRow& other) noexcept;

    // ICharRow methods
    ICharRow::SupportedEncoding GetSupportedEncoding() const noexcept override;

    std::wstring GetText() const;


    friend constexpr bool operator==(const Ucs2CharRow& a, const Ucs2CharRow& b) noexcept;

private:

    using CharType = wchar_t;
    using StringType = std::wstring;

#ifdef UNIT_TESTING
    friend class Ucs2CharRowTests;
#endif

};

void swap(Ucs2CharRow& a, Ucs2CharRow& b) noexcept;

constexpr bool operator==(const Ucs2CharRow& a, const Ucs2CharRow& b) noexcept
{
    return (static_cast<const CharRowBase<Ucs2CharRow::CharType, Ucs2CharRow::StringType>&>(a) ==
            static_cast<const CharRowBase<Ucs2CharRow::CharType, Ucs2CharRow::StringType>&>(b));
}

template<typename InputIt1, typename InputIt2>
void OverwriteColumns(_In_ InputIt1 startChars, _In_ InputIt1 endChars, _In_ InputIt2 startAttrs, _Out_ Ucs2CharRow::iterator outIt)
{
    std::transform(startChars,
                   endChars,
                   startAttrs,
                   outIt,
                   [](const wchar_t wch, const DbcsAttribute attr)
    {
        return Ucs2CharRow::value_type{ wch, attr };
    });
}

// this sticks specialization of swap() into the std::namespace for Ucs2CharRow, so that callers that use
// std::swap explicitly over calling the global swap can still get the performance benefit.
namespace std
{
    template<>
    inline void swap<Ucs2CharRow>(Ucs2CharRow& a, Ucs2CharRow& b) noexcept
    {
        a.swap(b);
    }
}
