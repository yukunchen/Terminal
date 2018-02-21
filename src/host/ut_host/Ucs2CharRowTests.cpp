/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#include "precomp.h"
#include "WexTestClass.h"

#include "CommonState.hpp"

#include "globals.h"
#include "textBuffer.hpp"

#include "input.h"

using namespace WEX::Logging;
using namespace WEX::TestExecution;

class Ucs2CharRowTests
{
    TEST_CLASS(Ucs2CharRowTests);

    const size_t _sRowWidth = 80;
    Ucs2CharRow::string_type noSpacesText = L"Loremipsumdolorsitamet,consecteturadipiscingelit.Nullametrutrummetus.Namquiseratal";


    TEST_METHOD(TestInitialize)
    {
        Ucs2CharRow charRow(_sRowWidth);
        VERIFY_IS_FALSE(charRow.WasWrapForced());
        VERIFY_IS_FALSE(charRow.WasDoubleBytePadded());
        VERIFY_ARE_EQUAL(charRow.GetSupportedEncoding(), ICharRow::SupportedEncoding::Ucs2);

        for (auto cell : charRow._data)
        {
            VERIFY_ARE_EQUAL(cell.first, UNICODE_SPACE);
            VERIFY_IS_TRUE(cell.second.IsSingle());
        }
    }

    TEST_METHOD(TestContainsText)
    {
        Ucs2CharRow charRow(_sRowWidth);
        // After init, should have no text
        VERIFY_IS_FALSE(charRow.ContainsText());

        // add some text
        charRow.GetGlyphAt(10) = L'a';

        // should have text
        VERIFY_IS_TRUE(charRow.ContainsText());
    }

    TEST_METHOD(TestMeasuring)
    {
        Ucs2CharRow charRow(_sRowWidth);

        std::vector<std::tuple<std::wstring,
                               std::vector<size_t>, // locations to fill with characters
                               size_t, // MeasureLeft value
                               size_t // MeasureRight value
                               >> testData =
        {
            {
                L"a row with all whitespace should measure the whole row",
                {},
                _sRowWidth,
                0
            },

            {
                L"a character as far left as possible",
                { 0 },
                0,
                1
            },

            {
                L"a character as far right as possible",
                { _sRowWidth - 1 },
                _sRowWidth - 1,
                _sRowWidth
            },

            {
                L"a character on the left side",
                { 10 },
                10,
                11
            },

            {
                L"a character on the right side",
                { _sRowWidth - 12 },
                _sRowWidth - 12,
                _sRowWidth - 11
            },

            {
                L"characters on both edges",
                { 0, _sRowWidth - 1},
                0,
                _sRowWidth
            },

            {
                L"characters near both edges",
                { 7, _sRowWidth - 3 },
                7,
                _sRowWidth - 2
            },
        };

        for (auto data : testData)
        {
            Log::Comment(std::get<0>(data).c_str());
            // apply the character changes
            const auto cellLocations = std::get<1>(data);
            for (auto index : cellLocations)
            {
                charRow.GetGlyphAt(index) = L'a';
            }

            // test measuring
            VERIFY_ARE_EQUAL(charRow.MeasureLeft(), std::get<2>(data));
            VERIFY_ARE_EQUAL(charRow.MeasureRight(), std::get<3>(data));

            // reset character changes
            for (auto index : cellLocations)
            {
                charRow.GetGlyphAt(index) = L' ';
            }
        }
    }

    TEST_METHOD(TestResize)
    {
        Ucs2CharRow charRow(_sRowWidth);
        VERIFY_ARE_EQUAL(charRow.size(), _sRowWidth);

        auto text = noSpacesText;
        text.resize(_sRowWidth);
        std::vector<DbcsAttribute> attrs(_sRowWidth);
        // change a bunch of random dbcs attributes
        for (auto& attr : attrs)
        {
            auto choice = rand() % 2;
            switch (choice)
            {
                case 0:
                    attr.SetSingle();
                    break;
                case 1:
                    attr.SetLeading();
                    break;
                case 2:
                    attr.SetTrailing();
                    break;
                default:
                    VERIFY_IS_TRUE(false);
            }
        }
        // fill cells with data
        OverwriteColumns(text.cbegin(), text.cend(), attrs.cbegin(), charRow.begin());
        VERIFY_ARE_EQUAL(text, charRow.GetText());

        // resize smaller
        const size_t smallSize = _sRowWidth / 2;
        charRow.Resize(smallSize);
        VERIFY_ARE_EQUAL(charRow.size(), smallSize);
        for (auto i = 0; i < smallSize; ++i)
        {
            auto cell = charRow._data[i];
            VERIFY_ARE_EQUAL(cell.first, text[i]);
            VERIFY_ARE_EQUAL(cell.second, attrs[i]);
        }

        // resize bigger
        const size_t bigSize = _sRowWidth * 2;
        charRow.Resize(bigSize);
        VERIFY_ARE_EQUAL(charRow.size(), bigSize);
        // original data should not be changed
        for (auto i = 0; i < smallSize; ++i)
        {
            auto cell = charRow._data[i];
            VERIFY_ARE_EQUAL(cell.first, text[i]);
            VERIFY_ARE_EQUAL(cell.second, attrs[i]);
        }
        // newly added cells should be set to the defaults
        for (auto i = smallSize + 1; i < bigSize; ++i)
        {
            auto cell = charRow._data[i];
            DbcsAttribute defaultAttr{};
            VERIFY_ARE_EQUAL(cell.first, L' ');
            VERIFY_ARE_EQUAL(cell.second, defaultAttr);
        }


    }

    TEST_METHOD(TextClearCell)
    {
        Ucs2CharRow charRow(_sRowWidth);
        auto text = noSpacesText;
        text.resize(_sRowWidth);
        std::vector<DbcsAttribute> attrs(_sRowWidth, DbcsAttribute::Attribute::Leading);

        // generate random cell locations to clear
        std::vector<size_t> eraseIndices;
        for (auto i = 0; i < 10; ++i)
        {
            eraseIndices.push_back(rand() % _sRowWidth);
        }

        // fill cells with data
        OverwriteColumns(text.cbegin(), text.cend(), attrs.cbegin(), charRow.begin());
        VERIFY_ARE_EQUAL(text, charRow.GetText());

        for (auto index : eraseIndices)
        {
            charRow.ClearCell(index);
            VERIFY_ARE_EQUAL(charRow._data[index].first, L' ');
            VERIFY_ARE_EQUAL(charRow._data[index].second, DbcsAttribute::Attribute::Single);
        }
    }

    TEST_METHOD(TextClearGlyph)
    {
        Ucs2CharRow charRow(_sRowWidth);
        auto text = noSpacesText;
        text.resize(_sRowWidth);
        std::vector<DbcsAttribute> attrs(_sRowWidth, DbcsAttribute::Attribute::Leading);

        // generate random cell locations to clear
        std::vector<size_t> eraseIndices;
        for (auto i = 0; i < 10; ++i)
        {
            eraseIndices.push_back(rand() % _sRowWidth);
        }

        // fill cells with data
        OverwriteColumns(text.cbegin(), text.cend(), attrs.cbegin(), charRow.begin());
        VERIFY_ARE_EQUAL(text, charRow.GetText());

        for (auto index : eraseIndices)
        {
            charRow.ClearGlyph(index);
            VERIFY_ARE_EQUAL(charRow._data[index].first, L' ');
            VERIFY_ARE_EQUAL(charRow._data[index].second, DbcsAttribute::Attribute::Leading);
        }
    }

    TEST_METHOD(TestIterators)
    {
        Ucs2CharRow charRow(_sRowWidth);
        auto text = noSpacesText;
        text.resize(_sRowWidth);
        std::vector<DbcsAttribute> attrs(_sRowWidth, DbcsAttribute::Attribute::Trailing);

        // fill cells with data
        OverwriteColumns(text.cbegin(), text.cend(), attrs.cbegin(), charRow.begin());

        // ensure iterators point to correct data
        size_t index = 0;
        for (Ucs2CharRow::const_iterator it = charRow.cbegin(); it != charRow.cend(); ++it)
        {
            VERIFY_ARE_EQUAL(text[index], it->first);
            VERIFY_ARE_EQUAL(attrs[index], it->second);
            ++index;
        }
    }

    TEST_METHOD(TestGetText)
    {
        Ucs2CharRow charRow(_sRowWidth);
        auto text = noSpacesText;
        text.resize(_sRowWidth);
        std::vector<DbcsAttribute> attrs(_sRowWidth);
        for (auto i = 0; i < attrs.size(); ++i)
        {
            if (i % 2 == 0)
            {
                attrs[i].SetLeading();
            }
            else
            {
                attrs[i].SetTrailing();
            }
        }

        // fill cells with data
        OverwriteColumns(text.cbegin(), text.cend(), attrs.cbegin(), charRow.begin());

        // we expect for trailing cells to be filtered out
        Ucs2CharRow::string_type expectedText = L"";
        for (auto i = 0; i < text.size(); ++i)
        {
            if (i % 2 == 0)
            {
                expectedText += text[i];
            }
        }

        VERIFY_ARE_EQUAL(expectedText, charRow.GetText());
    }
};
