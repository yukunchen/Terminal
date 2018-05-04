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

class ScreenInfoTextIteratorTests
{
    CommonState* m_state;

    TEST_CLASS(ScreenInfoTextIteratorTests);

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

    TEST_METHOD(BoolOperator);
    TEST_METHOD(EqualsOperator);
    TEST_METHOD(NotEqualsOperator);
    TEST_METHOD(PlusEqualsOperator);
    TEST_METHOD(MinusEqualsOperator);
    TEST_METHOD(PrefixPlusPlusOperator);
    TEST_METHOD(PrefixMinusMinusOperator);
    TEST_METHOD(PostfixPlusPlusOperator);
    TEST_METHOD(PostfixMinusMinusOperator);
    TEST_METHOD(PlusOperator);
    TEST_METHOD(MinusOperator);
    TEST_METHOD(DifferenceOperator);
    TEST_METHOD(DereferenceOperator);
    TEST_METHOD(GetPtr);
};

ScreenInfoTextIterator GetIterator()
{
    const auto& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    const auto& outputBuffer = gci.GetActiveOutputBuffer();
    return outputBuffer.GetTextDataAt({ 0 });
}

void ScreenInfoTextIteratorTests::BoolOperator()
{
    const auto it = GetIterator();
    VERIFY_IS_TRUE(it);
    
    VERIFY_THROWS_SPECIFIC(ScreenInfoTextIterator(nullptr, { 0 }); , wil::ResultException, [](wil::ResultException& e) { return e.GetErrorCode() == E_INVALIDARG; });

    const auto& outputBuffer = ServiceLocator::LocateGlobals().getConsoleInformation().GetActiveOutputBuffer();
    const auto size = outputBuffer.GetScreenBufferSize();
    const ScreenInfoTextIterator itInvalidPos(&outputBuffer, { size.X, size.Y });
    VERIFY_IS_FALSE(itInvalidPos);
}

void ScreenInfoTextIteratorTests::EqualsOperator()
{
    const auto it = GetIterator();
    const auto it2 = GetIterator();

    VERIFY_ARE_EQUAL(it, it2);
}

void ScreenInfoTextIteratorTests::NotEqualsOperator()
{
    const auto it = GetIterator();

    COORD oneOff = it._pos;
    oneOff.X++;
    const auto it2 = ScreenInfoTextIterator(it._psi, oneOff);

    VERIFY_ARE_NOT_EQUAL(it, it2);
}

void ScreenInfoTextIteratorTests::PlusEqualsOperator()
{
    auto it = GetIterator();

    ptrdiff_t diffUnit = 3;
    COORD expectedPos = it._pos;
    expectedPos.X += gsl::narrow<SHORT>(diffUnit);
    const auto itExpected = ScreenInfoTextIterator(it._psi, expectedPos);

    it += diffUnit;

    VERIFY_ARE_EQUAL(itExpected, it);
}

void ScreenInfoTextIteratorTests::MinusEqualsOperator()
{
    auto itExpected = GetIterator();

    ptrdiff_t diffUnit = 3;
    COORD pos = itExpected._pos;
    pos.X += gsl::narrow<SHORT>(diffUnit);
    auto itOffset = ScreenInfoTextIterator(itExpected._psi, pos);

    itOffset -= diffUnit;

    VERIFY_ARE_EQUAL(itExpected, itOffset);
}

void ScreenInfoTextIteratorTests::PrefixPlusPlusOperator()
{
    auto itActual = GetIterator();

    COORD expectedPos = itActual._pos;
    expectedPos.X++;
    const auto itExpected = ScreenInfoTextIterator(itActual._psi, expectedPos);

    ++itActual;

    VERIFY_ARE_EQUAL(itExpected, itActual);
}

void ScreenInfoTextIteratorTests::PrefixMinusMinusOperator()
{
    const auto itExpected = GetIterator();

    COORD pos = itExpected._pos;
    pos.X++;
    auto itActual = ScreenInfoTextIterator(itExpected._psi, pos);

    --itActual;

    VERIFY_ARE_EQUAL(itExpected, itActual);
}

void ScreenInfoTextIteratorTests::PostfixPlusPlusOperator()
{
    auto it = GetIterator();

    COORD expectedPos = it._pos;
    expectedPos.X++;
    const auto itExpected = ScreenInfoTextIterator(it._psi, expectedPos);

    ++it;

    VERIFY_ARE_EQUAL(itExpected, it);
}

void ScreenInfoTextIteratorTests::PostfixMinusMinusOperator()
{
    const auto itExpected = GetIterator();

    COORD pos = itExpected._pos;
    pos.X++;
    auto itActual = ScreenInfoTextIterator(itExpected._psi, pos);

    itActual--;

    VERIFY_ARE_EQUAL(itExpected, itActual);
}

void ScreenInfoTextIteratorTests::PlusOperator()
{
    auto it = GetIterator();
    
    ptrdiff_t diffUnit = 3;
    COORD expectedPos = it._pos;
    expectedPos.X += gsl::narrow<SHORT>(diffUnit);
    const auto itExpected = ScreenInfoTextIterator(it._psi, expectedPos);

    const auto itActual = it + diffUnit;

    VERIFY_ARE_EQUAL(itExpected, itActual);
}

void ScreenInfoTextIteratorTests::MinusOperator()
{
    auto itExpected = GetIterator();

    ptrdiff_t diffUnit = 3;
    COORD pos = itExpected._pos;
    pos.X += gsl::narrow<SHORT>(diffUnit);
    auto itOffset = ScreenInfoTextIterator(itExpected._psi, pos);

    const auto itActual = itOffset - diffUnit;

    VERIFY_ARE_EQUAL(itExpected, itActual);
}

void ScreenInfoTextIteratorTests::DifferenceOperator()
{
    const ptrdiff_t expected(3);
    auto it = GetIterator();
    auto it2 = it + expected;

    const ptrdiff_t actual = it2 - it;
    VERIFY_ARE_EQUAL(expected, actual);
}

void ScreenInfoTextIteratorTests::DereferenceOperator()
{
    m_state->FillTextBuffer();
    const auto it = GetIterator();

    const auto& outputBuffer = ServiceLocator::LocateGlobals().getConsoleInformation().GetActiveOutputBuffer();
    const auto cell = outputBuffer.ReadLine(it._pos.Y).at(it._pos.X);
    VERIFY_ARE_EQUAL(1u, cell.Chars().size());

    const auto wcharExpected = cell.Chars().at(0);
    const auto wcharActual = *it;

    VERIFY_ARE_EQUAL(wcharExpected, wcharActual);
}

void ScreenInfoTextIteratorTests::GetPtr()
{
    m_state->FillTextBuffer();
    const auto it = GetIterator();

    const auto& outputBuffer = ServiceLocator::LocateGlobals().getConsoleInformation().GetActiveOutputBuffer();
    const auto cell = outputBuffer.ReadLine(it._pos.Y).at(it._pos.X);
    VERIFY_ARE_EQUAL(1u, cell.Chars().size());

    const auto wcharExpected = cell.Chars().at(0);
    const auto wcharPtrActual = it.getPtr();

    VERIFY_ARE_EQUAL(wcharExpected, *wcharPtrActual);
}