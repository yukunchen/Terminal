/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#include "precomp.h"
#include "WexTestClass.h"

#include "CommonState.hpp"

#include "globals.h"
#include "../buffer/out/textBuffer.hpp"
#include "../buffer/out/CharRow.hpp"

#include "input.h"

#include "../interactivity/inc/ServiceLocator.hpp"

using namespace WEX::Common;
using namespace WEX::Logging;
using namespace WEX::TestExecution;

class ScreenInfoIteratorTests
{
    CommonState* m_state;

    TEST_CLASS(ScreenInfoIteratorTests);

    TEST_CLASS_SETUP(ClassSetup)
    {
        m_state = new CommonState();

        m_state->PrepareGlobalFont();
        m_state->PrepareGlobalScreenBuffer();

        return true;
    }

    TEST_CLASS_CLEANUP(ClassCleanup)
    {
        m_state->CleanupGlobalScreenBuffer();
        m_state->CleanupGlobalFont();

        delete m_state;

        return true;
    }

    TEST_METHOD_SETUP(MethodSetup)
    {
        m_state->PrepareNewTextBufferInfo();

        return true;
    }

    TEST_METHOD_CLEANUP(MethodCleanup)
    {
        m_state->CleanupNewTextBufferInfo();

        return true;
    }

    template <typename T>
    void BoolOperatorTestHelper()
    {
        const auto it = GetIterator<T>();
        VERIFY_IS_TRUE(it);

        const auto& outputBuffer = ServiceLocator::LocateGlobals().getConsoleInformation().GetActiveOutputBuffer();
        const auto size = outputBuffer.GetScreenBufferSize();
        const T itInvalidPos(outputBuffer, { size.X, size.Y });
        VERIFY_IS_FALSE(itInvalidPos);
    }

    TEST_METHOD(BoolOperatorText);
    TEST_METHOD(BoolOperatorCell);

    template <typename T>
    void EqualsOperatorTestHelper()
    {
        const auto it = GetIterator<T>();
        const auto it2 = GetIterator<T>();

        VERIFY_ARE_EQUAL(it, it2);
    }

    TEST_METHOD(EqualsOperatorText);
    TEST_METHOD(EqualsOperatorCell);
    
    template <typename T>
    void NotEqualsOperatorTestHelper()
    {
        const auto it = GetIterator<T>();

        COORD oneOff = it._pos;
        oneOff.X++;
        const auto it2 = T(it._si, oneOff);

        VERIFY_ARE_NOT_EQUAL(it, it2);
    }

    TEST_METHOD(NotEqualsOperatorText);
    TEST_METHOD(NotEqualsOperatorCell);

    template <typename T>
    void PlusEqualsOperatorTestHelper()
    {
        auto it = GetIterator<T>();

        ptrdiff_t diffUnit = 3;
        COORD expectedPos = it._pos;
        expectedPos.X += gsl::narrow<SHORT>(diffUnit);
        const auto itExpected = T(it._si, expectedPos);

        it += diffUnit;

        VERIFY_ARE_EQUAL(itExpected, it);
    }

    TEST_METHOD(PlusEqualsOperatorText);
    TEST_METHOD(PlusEqualsOperatorCell);
    
    template <typename T>
    void MinusEqualsOperatorTestHelper()
    {
        auto itExpected = GetIterator<T>();

        ptrdiff_t diffUnit = 3;
        COORD pos = itExpected._pos;
        pos.X += gsl::narrow<SHORT>(diffUnit);
        auto itOffset = T(itExpected._si, pos);

        itOffset -= diffUnit;

        VERIFY_ARE_EQUAL(itExpected, itOffset);
    }
    
    TEST_METHOD(MinusEqualsOperatorText);
    TEST_METHOD(MinusEqualsOperatorCell);

    template <typename T>
    void PrefixPlusPlusOperatorTestHelper()
    {
        auto itActual = GetIterator<T>();

        COORD expectedPos = itActual._pos;
        expectedPos.X++;
        const auto itExpected = T(itActual._si, expectedPos);

        ++itActual;

        VERIFY_ARE_EQUAL(itExpected, itActual);
    }

    TEST_METHOD(PrefixPlusPlusOperatorText);
    TEST_METHOD(PrefixPlusPlusOperatorCell);

    template <typename T>
    void PrefixMinusMinusOperatorTestHelper()
    {
        const auto itExpected = GetIterator<T>();

        COORD pos = itExpected._pos;
        pos.X++;
        auto itActual = T(itExpected._si, pos);

        --itActual;

        VERIFY_ARE_EQUAL(itExpected, itActual);
    }

    TEST_METHOD(PrefixMinusMinusOperatorText);
    TEST_METHOD(PrefixMinusMinusOperatorCell);
    
    template <typename T>
    void PostfixPlusPlusOperatorTestHelper()
    {
        auto it = GetIterator<T>();

        COORD expectedPos = it._pos;
        expectedPos.X++;
        const auto itExpected = T(it._si, expectedPos);

        ++it;

        VERIFY_ARE_EQUAL(itExpected, it);
    }

    TEST_METHOD(PostfixPlusPlusOperatorText);
    TEST_METHOD(PostfixPlusPlusOperatorCell);
    
    template <typename T>
    void PostfixMinusMinusOperatorTestHelper()
    {
        const auto itExpected = GetIterator<T>();

        COORD pos = itExpected._pos;
        pos.X++;
        auto itActual = T(itExpected._si, pos);

        itActual--;

        VERIFY_ARE_EQUAL(itExpected, itActual);
    }

    TEST_METHOD(PostfixMinusMinusOperatorText);
    TEST_METHOD(PostfixMinusMinusOperatorCell);

    template <typename T>
    void PlusOperatorTestHelper()
    {
        auto it = GetIterator<T>();

        ptrdiff_t diffUnit = 3;
        COORD expectedPos = it._pos;
        expectedPos.X += gsl::narrow<SHORT>(diffUnit);
        const auto itExpected = T(it._si, expectedPos);

        const auto itActual = it + diffUnit;

        VERIFY_ARE_EQUAL(itExpected, itActual);
    }

    TEST_METHOD(PlusOperatorText);
    TEST_METHOD(PlusOperatorCell);

    template <typename T>
    void MinusOperatorTestHelper()
    {
        auto itExpected = GetIterator<T>();

        ptrdiff_t diffUnit = 3;
        COORD pos = itExpected._pos;
        pos.X += gsl::narrow<SHORT>(diffUnit);
        auto itOffset = T(itExpected._si, pos);

        const auto itActual = itOffset - diffUnit;

        VERIFY_ARE_EQUAL(itExpected, itActual);
    }

    TEST_METHOD(MinusOperatorText);
    TEST_METHOD(MinusOperatorCell);

    template <typename T>
    void DifferenceOperatorTestHelper()
    {
        const ptrdiff_t expected(3);
        auto it = GetIterator<T>();
        auto it2 = it + expected;

        const ptrdiff_t actual = it2 - it;
        VERIFY_ARE_EQUAL(expected, actual);
    }

    TEST_METHOD(DifferenceOperatorText);
    TEST_METHOD(DifferenceOperatorCell);

    TEST_METHOD(DereferenceOperatorText);
    TEST_METHOD(DereferenceOperatorCell);
};

template <typename T>
T GetIterator() {}

template<>
ScreenInfoCellIterator GetIterator<ScreenInfoCellIterator>()
{
    const auto& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    const auto& outputBuffer = gci.GetActiveOutputBuffer();
    return outputBuffer.GetCellDataAt({ 0 });
}

template<>
ScreenInfoTextIterator GetIterator<ScreenInfoTextIterator>()
{
    const auto& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    const auto& outputBuffer = gci.GetActiveOutputBuffer();
    return outputBuffer.GetTextDataAt({ 0 });
}

void ScreenInfoIteratorTests::BoolOperatorText()
{
    BoolOperatorTestHelper<ScreenInfoTextIterator>();
}

void ScreenInfoIteratorTests::BoolOperatorCell()
{
    BoolOperatorTestHelper<ScreenInfoCellIterator>();

    Log::Comment(L"For cells, also check incrementing past the end.");
    const auto& outputBuffer = ServiceLocator::LocateGlobals().getConsoleInformation().GetActiveOutputBuffer();
    const auto size = outputBuffer.GetScreenBufferSize();
    ScreenInfoCellIterator it(outputBuffer, { size.X-1, size.Y-1 });
    VERIFY_IS_TRUE(it);
    it++;
    VERIFY_IS_FALSE(it);
}

void ScreenInfoIteratorTests::EqualsOperatorText()
{
    EqualsOperatorTestHelper<ScreenInfoTextIterator>();
}

void ScreenInfoIteratorTests::EqualsOperatorCell()
{
    EqualsOperatorTestHelper<ScreenInfoCellIterator>();
}

void ScreenInfoIteratorTests::NotEqualsOperatorText()
{
    NotEqualsOperatorTestHelper<ScreenInfoTextIterator>();
}

void ScreenInfoIteratorTests::NotEqualsOperatorCell()
{
    NotEqualsOperatorTestHelper<ScreenInfoCellIterator>();
}

void ScreenInfoIteratorTests::PlusEqualsOperatorText()
{
    PlusEqualsOperatorTestHelper<ScreenInfoTextIterator>();
}

void ScreenInfoIteratorTests::PlusEqualsOperatorCell()
{
    PlusEqualsOperatorTestHelper<ScreenInfoCellIterator>();
}

void ScreenInfoIteratorTests::MinusEqualsOperatorText()
{
    MinusEqualsOperatorTestHelper<ScreenInfoTextIterator>();
}

void ScreenInfoIteratorTests::MinusEqualsOperatorCell()
{
    MinusEqualsOperatorTestHelper<ScreenInfoCellIterator>();
}

void ScreenInfoIteratorTests::PrefixPlusPlusOperatorText()
{
    PrefixPlusPlusOperatorTestHelper<ScreenInfoTextIterator>();
}

void ScreenInfoIteratorTests::PrefixPlusPlusOperatorCell()
{
    PrefixPlusPlusOperatorTestHelper<ScreenInfoCellIterator>();
}

void ScreenInfoIteratorTests::PrefixMinusMinusOperatorText()
{
    PrefixMinusMinusOperatorTestHelper<ScreenInfoTextIterator>();
}

void ScreenInfoIteratorTests::PrefixMinusMinusOperatorCell()
{
    PrefixMinusMinusOperatorTestHelper<ScreenInfoCellIterator>();
}

void ScreenInfoIteratorTests::PostfixPlusPlusOperatorText()
{
    PostfixPlusPlusOperatorTestHelper<ScreenInfoTextIterator>();
}

void ScreenInfoIteratorTests::PostfixPlusPlusOperatorCell()
{
    PostfixPlusPlusOperatorTestHelper<ScreenInfoCellIterator>();
}

void ScreenInfoIteratorTests::PostfixMinusMinusOperatorText()
{
    PostfixMinusMinusOperatorTestHelper<ScreenInfoTextIterator>();
}

void ScreenInfoIteratorTests::PostfixMinusMinusOperatorCell()
{
    PostfixMinusMinusOperatorTestHelper<ScreenInfoCellIterator>();
}

void ScreenInfoIteratorTests::PlusOperatorText()
{
    PlusOperatorTestHelper<ScreenInfoTextIterator>();
}

void ScreenInfoIteratorTests::PlusOperatorCell()
{
    PlusOperatorTestHelper<ScreenInfoCellIterator>();
}

void ScreenInfoIteratorTests::MinusOperatorText()
{
    MinusOperatorTestHelper<ScreenInfoTextIterator>();
}

void ScreenInfoIteratorTests::MinusOperatorCell()
{
    MinusOperatorTestHelper<ScreenInfoCellIterator>();
}

void ScreenInfoIteratorTests::DifferenceOperatorText()
{
    DifferenceOperatorTestHelper<ScreenInfoTextIterator>();
}

void ScreenInfoIteratorTests::DifferenceOperatorCell()
{
    DifferenceOperatorTestHelper<ScreenInfoCellIterator>();
}

void ScreenInfoIteratorTests::DereferenceOperatorText()
{
    m_state->FillTextBuffer();
    const auto it = GetIterator<ScreenInfoTextIterator>();

    const auto& outputBuffer = ServiceLocator::LocateGlobals().getConsoleInformation().GetActiveOutputBuffer();
    const auto cell = outputBuffer.ReadLine(it._pos.Y).at(it._pos.X);
    VERIFY_ARE_EQUAL(1u, cell.Chars().size());

    const auto wcharExpected = cell.Chars().at(0);
    const auto wcharActual = *it;

    VERIFY_ARE_EQUAL(wcharExpected, *wcharActual.begin());
}

void ScreenInfoIteratorTests::DereferenceOperatorCell()
{
    m_state->FillTextBuffer();
    const auto it = GetIterator<ScreenInfoCellIterator>();

    const auto& outputBuffer = ServiceLocator::LocateGlobals().getConsoleInformation().GetActiveOutputBuffer();
    const auto cellExpected = outputBuffer.ReadLine(it._pos.Y).at(it._pos.X);
    VERIFY_ARE_EQUAL(1u, cellExpected.Chars().size());
    const auto wcharExpected = cellExpected.Chars().at(0);
    const auto attrExpected = cellExpected.TextAttr().GetLegacyAttributes();

    const auto cellActual = *it;
    const auto wcharActual = cellActual.Char.UnicodeChar;
    const auto attrActual = cellActual.Attributes;

    VERIFY_ARE_EQUAL(wcharExpected, wcharActual);
    VERIFY_ARE_EQUAL(attrExpected, attrActual);
}
