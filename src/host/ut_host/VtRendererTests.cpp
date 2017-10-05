/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include <wextestclass.h>
#include "../../inc/consoletaeftemplates.hpp"

#include "../../renderer/vt/Xterm256Engine.hpp"
#include "../../renderer/vt/XtermEngine.hpp"
#include "../../renderer/vt/WinTelnetEngine.hpp"
#include "../Settings.hpp"

using namespace WEX::Common;
using namespace WEX::Logging;
using namespace WEX::TestExecution;

namespace Microsoft
{
    namespace Console
    {
        namespace Render
        {
            class VtRendererTest;
        };
    };
};
using namespace Microsoft::Console::Render;

COLORREF g_ColorTable[COLOR_TABLE_SIZE];

class Microsoft::Console::Render::VtRendererTest
{
    TEST_CLASS(VtRendererTest);

    TEST_CLASS_SETUP(ClassSetup)
    {
        g_ColorTable[0] =  RGB( 12,  12,  12); // Black
        g_ColorTable[1] =  RGB( 0,   55, 218); // Dark Blue
        g_ColorTable[2] =  RGB( 19, 161,  14); // Dark Green
        g_ColorTable[3] =  RGB( 58, 150, 221); // Dark Cyan
        g_ColorTable[4] =  RGB(197,  15,  31); // Dark Red
        g_ColorTable[5] =  RGB(136,  23, 152); // Dark Magenta
        g_ColorTable[6] =  RGB(193, 156,   0); // Dark Yellow
        g_ColorTable[7] =  RGB(204, 204, 204); // Dark White
        g_ColorTable[8] =  RGB(118, 118, 118); // Bright Black
        g_ColorTable[9] =  RGB( 59, 120, 255); // Bright Blue
        g_ColorTable[10] = RGB( 22, 198,  12); // Bright Green
        g_ColorTable[11] = RGB( 97, 214, 214); // Bright Cyan
        g_ColorTable[12] = RGB(231,  72,  86); // Bright Red
        g_ColorTable[13] = RGB(180,   0, 158); // Bright Magenta
        g_ColorTable[14] = RGB(249, 241, 165); // Bright Yellow
        g_ColorTable[15] = RGB(242, 242, 242); // White
        return true;
    }

    TEST_CLASS_CLEANUP(ClassCleanup)
    {
        return true;
    }
    TEST_METHOD_SETUP(MethodSetup)
    {
        return true;
    }
    TEST_METHOD_CLEANUP(MethodCleanup)
    {
        return true;
    }

    TEST_METHOD(VtSequenceHelperTests);
    TEST_METHOD(Xterm256Test);
    TEST_METHOD(XtermTest);
    TEST_METHOD(WinTelnetTest);

    std::deque<std::string> qExpectedInput;
    bool WriteCallback(const char* const pch, size_t const cch);
};

bool VtRendererTest::WriteCallback(const char* const pch, size_t const cch)
{
    std::string first = qExpectedInput.front();

    std::string actualString = std::string(pch);
    
    Log::Comment(NoThrowString().Format(L"Expected =\t\"%hs\"", first.c_str()));
    Log::Comment(NoThrowString().Format(L"Actual =\t\"%hs\"", actualString.c_str()));

    VERIFY_ARE_EQUAL(first.length(), cch);
    VERIFY_ARE_EQUAL(first, actualString);

    qExpectedInput.pop_front();

    return true;
}

void VtRendererTest::VtSequenceHelperTests()
{
    wil::unique_hfile hFile = wil::unique_hfile(INVALID_HANDLE_VALUE);
    std::unique_ptr<Xterm256Engine> engine = std::make_unique<Xterm256Engine>(std::move(hFile));
    auto pfn = std::bind(&VtRendererTest::WriteCallback, this, std::placeholders::_1, std::placeholders::_2);

    engine->SetTestCallback(pfn);
    
    qExpectedInput.push_back("\x1b[?12l");
    engine->_StopCursorBlinking();
    
    qExpectedInput.push_back("\x1b[?12h");
    engine->_StartCursorBlinking();
    
    qExpectedInput.push_back("\x1b[?25l");
    engine->_HideCursor();
    
    qExpectedInput.push_back("\x1b[?25h");
    engine->_ShowCursor();
    
    qExpectedInput.push_back("\x1b[0K");
    engine->_EraseLine();
    
    qExpectedInput.push_back("\x1b[M");
    engine->_DeleteLine(1);

    qExpectedInput.push_back("\x1b[2M");
    engine->_DeleteLine(2);
    
    qExpectedInput.push_back("\x1b[L");
    engine->_InsertLine(1);

    qExpectedInput.push_back("\x1b[2L");
    engine->_InsertLine(2);
    
    qExpectedInput.push_back("\x1b[2;3H");
    engine->_CursorPosition({2,1});

    qExpectedInput.push_back("\x1b[1;1H");
    engine->_CursorPosition({0,0});

    qExpectedInput.push_back("\x1b[H");
    engine->_CursorHome();
}

void VtRendererTest::Xterm256Test()
{
    wil::unique_hfile hFile = wil::unique_hfile(INVALID_HANDLE_VALUE);
    std::unique_ptr<Xterm256Engine> engine = std::make_unique<Xterm256Engine>(std::move(hFile));
    auto pfn = std::bind(&VtRendererTest::WriteCallback, this, std::placeholders::_1, std::placeholders::_2);
    engine->SetTestCallback(pfn);

    // TODO: Finish test.

    VERIFY_SUCCEEDED(E_FAIL);
}

void VtRendererTest::XtermTest()
{
    wil::unique_hfile hFile = wil::unique_hfile(INVALID_HANDLE_VALUE);
    std::unique_ptr<XtermEngine> engine = std::make_unique<XtermEngine>(std::move(hFile), g_ColorTable, (WORD)COLOR_TABLE_SIZE);
    auto pfn = std::bind(&VtRendererTest::WriteCallback, this, std::placeholders::_1, std::placeholders::_2);
    engine->SetTestCallback(pfn);

    // TODO: Finish test.

    VERIFY_SUCCEEDED(E_FAIL);
}

void VtRendererTest::WinTelnetTest()
{
    wil::unique_hfile hFile = wil::unique_hfile(INVALID_HANDLE_VALUE);
    std::unique_ptr<XtermEngine> engine = std::make_unique<XtermEngine>(std::move(hFile), g_ColorTable, (WORD)COLOR_TABLE_SIZE);
    auto pfn = std::bind(&VtRendererTest::WriteCallback, this, std::placeholders::_1, std::placeholders::_2);
    engine->SetTestCallback(pfn);

    // TODO: Finish test.

    VERIFY_SUCCEEDED(E_FAIL);
}
