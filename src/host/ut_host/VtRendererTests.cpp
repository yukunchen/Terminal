/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include <wextestclass.h>
#include "../../inc/consoletaeftemplates.hpp"
#include "../../types/inc/Viewport.hpp"

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
using namespace Microsoft::Console::Types;

COLORREF g_ColorTable[COLOR_TABLE_SIZE];

// Sometimes when we're expecting the renderengine to not write anything,
// we'll add this to the expected input, and manually write this to the callback
// to make sure nothing else gets written. 
// We don't use null because that will confuse the VERIFY macros re: string length.
const char* const EMPTY_CALLBACK_SENTINEL = "\xff";

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

    // Defining a TEST_METHOD_CLEANUP seemed to break x86 test pass. Not sure why,
    //  something about the clipboard tests and
    //  YOU_CAN_ONLY_DESIGNATE_ONE_CLASS_METHOD_TO_BE_A_TEST_METHOD_SETUP_METHOD
    // It's probably more correct to leave it out anyways.

    TEST_METHOD(VtSequenceHelperTests);
    
    TEST_METHOD(Xterm256TestInvalidate);
    TEST_METHOD(Xterm256TestColors);
    TEST_METHOD(Xterm256TestCursor);
    
    TEST_METHOD(XtermTestInvalidate);
    TEST_METHOD(XtermTestColors);
    TEST_METHOD(XtermTestCursor);

    TEST_METHOD(WinTelnetTestInvalidate);
    TEST_METHOD(WinTelnetTestColors);
    TEST_METHOD(WinTelnetTestCursor);

    void Test16Colors(VtEngine* engine);

    std::deque<std::string> qExpectedInput;
    bool WriteCallback(const char* const pch, size_t const cch);
    void TestPaint(VtEngine& engine, std::function<void()> pfn);
    void TestPaintXterm(XtermEngine& engine, std::function<void()> pfn);
    Viewport SetUpViewport();
};

Viewport VtRendererTest::SetUpViewport()
{
    SMALL_RECT view = {};
    view.Top = view.Left = 0;
    view.Bottom = 31;
    view.Right = 79;

    return Viewport::FromInclusive(view);
}

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

// Function Description:
// - Small helper to do a series of testing wrapped by StartPaint/EndPaint calls
// Arguments:
// - engine: the engine to operate on
// - pfn: A function pointer to some test code to run.
// Return Value:
// - <none>
void VtRendererTest::TestPaint(VtEngine& engine, std::function<void()> pfn)
{
    engine.StartPaint();
    pfn();
    engine.EndPaint();
}

// Function Description:
// - Small helper to do a series of testing wrapped by StartPaint/EndPaint calls
//  Also expects \x1b[?25l and \x1b[?25h on start/stop, for cursor visibility
// Arguments:
// - engine: the engine to operate on
// - pfn: A function pointer to some test code to run.
// Return Value:
// - <none>
void VtRendererTest::TestPaintXterm(XtermEngine& engine, std::function<void()> pfn)
{
    qExpectedInput.push_back("\x1b[?25l");
    HRESULT hr = engine.StartPaint();
    if (hr == S_FALSE || engine._WillWriteSingleChar())
    {
        // Still do what the caller passed in, but without expecting the wrapping VT sequences.
        qExpectedInput.pop_front();
        pfn();
    }
    else
    {
        pfn();
        qExpectedInput.push_back("\x1b[?25h");
    }
    
    engine.EndPaint();
}

void VtRendererTest::VtSequenceHelperTests()
{
    wil::unique_hfile hFile = wil::unique_hfile(INVALID_HANDLE_VALUE);
    std::unique_ptr<Xterm256Engine> engine = std::make_unique<Xterm256Engine>(std::move(hFile), SetUpViewport());
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
    engine->_CursorPosition({2, 1});

    qExpectedInput.push_back("\x1b[1;1H");
    engine->_CursorPosition({0, 0});

    qExpectedInput.push_back("\x1b[H");
    engine->_CursorHome();

    qExpectedInput.push_back("\x1b[8;32;80t");
    engine->_ResizeWindow(80, 32);

}

void VtRendererTest::Xterm256TestInvalidate()
{
    wil::unique_hfile hFile = wil::unique_hfile(INVALID_HANDLE_VALUE);
    std::unique_ptr<Xterm256Engine> engine = std::make_unique<Xterm256Engine>(std::move(hFile), SetUpViewport());
    auto pfn = std::bind(&VtRendererTest::WriteCallback, this, std::placeholders::_1, std::placeholders::_2);
    engine->SetTestCallback(pfn);

    Viewport view = SetUpViewport();

    Log::Comment(NoThrowString().Format(
        L"Make sure that invalidating all invalidates the whole viewport."
    ));
    engine->InvalidateAll();
    TestPaintXterm(*engine, [&]()
    {
        VERIFY_ARE_EQUAL(view.ToExclusive(), engine->_srcInvalid);
    });

    Log::Comment(NoThrowString().Format(
        L"Make sure that invalidating anything only invalidates that portion"
    ));
    SMALL_RECT invalid = {1, 1, 1, 1};
    engine->Invalidate(&invalid);
    TestPaintXterm(*engine, [&]()
    {
        VERIFY_ARE_EQUAL(invalid, engine->_srcInvalid);
    });

    Log::Comment(NoThrowString().Format(
        L"Make sure that scrolling only invalidates part of the viewport, and sends the right sequences"
    ));
    COORD scrollDelta = {0, 1};
    engine->InvalidateScroll(&scrollDelta);
    TestPaintXterm(*engine, [&]()
    {
        Log::Comment(NoThrowString().Format(
            L"---- Scrolled one down, only top line is invalid. ----"
        ));
        invalid = view.ToInclusive();
        invalid.Bottom = 1;

        VERIFY_ARE_EQUAL(invalid, engine->_srcInvalid);
        // We would expect a CUP here, but the cursor is already at the home position
        // qExpectedInput.push_back("\x1b[H"); 
        
        qExpectedInput.push_back("\x1b[L"); // insert a line
        VERIFY_SUCCEEDED(engine->ScrollFrame());
    });

    scrollDelta = {0, 3};
    engine->InvalidateScroll(&scrollDelta);
    TestPaintXterm(*engine, [&]()
    {
        Log::Comment(NoThrowString().Format(
            L"---- Scrolled three down, only top 3 lines are invalid. ----"
        ));
        invalid = view.ToInclusive();
        invalid.Bottom = 3;

        VERIFY_ARE_EQUAL(invalid, engine->_srcInvalid);
        // We would expect a CUP here, but the cursor is already at the home position
        qExpectedInput.push_back("\x1b[3L"); // insert 3 lines
        VERIFY_SUCCEEDED(engine->ScrollFrame());
    });

    scrollDelta = {0, -1};
    engine->InvalidateScroll(&scrollDelta);
    TestPaintXterm(*engine, [&]()
    {
        Log::Comment(NoThrowString().Format(
            L"---- Scrolled one up, only bottom line is invalid. ----"
        ));
        invalid = view.ToInclusive();
        invalid.Top = invalid.Bottom - 1;

        VERIFY_ARE_EQUAL(invalid, engine->_srcInvalid);
        // We would expect a CUP here, but the cursor is already at the home position
        // qExpectedInput.push_back("\x1b[H"); 
        
        qExpectedInput.push_back("\x1b[M"); // delete a line
        VERIFY_SUCCEEDED(engine->ScrollFrame());
    });

    scrollDelta = {0, -3};
    engine->InvalidateScroll(&scrollDelta);
    TestPaintXterm(*engine, [&]()
    {
        Log::Comment(NoThrowString().Format(
            L"---- Scrolled three up, only bottom 3 lines are invalid. ----"
        ));
        invalid = view.ToInclusive();
        invalid.Top = invalid.Bottom - 3;

        VERIFY_ARE_EQUAL(invalid, engine->_srcInvalid);
        // We would expect a CUP here, but the cursor is already at the home position
        qExpectedInput.push_back("\x1b[3M"); // delete 3 lines
        VERIFY_SUCCEEDED(engine->ScrollFrame());
    });

    Log::Comment(NoThrowString().Format(
        L"Multiple scrolls are coalesced"
    ));

    scrollDelta = {0, 1};
    engine->InvalidateScroll(&scrollDelta);
    scrollDelta = {0, 2};
    engine->InvalidateScroll(&scrollDelta);
    TestPaintXterm(*engine, [&]()
    {
        Log::Comment(NoThrowString().Format(
            L"---- Scrolled three down, only top 3 lines are invalid. ----"
        ));
        invalid = view.ToInclusive();
        invalid.Bottom = 3;

        VERIFY_ARE_EQUAL(invalid, engine->_srcInvalid);
        // We would expect a CUP here, but the cursor is already at the home position
        qExpectedInput.push_back("\x1b[3L"); // insert 3 lines
        VERIFY_SUCCEEDED(engine->ScrollFrame());
    });

    scrollDelta = {0, 1};
    engine->InvalidateScroll(&scrollDelta);
    Log::Comment(NoThrowString().Format(
        VerifyOutputTraits<SMALL_RECT>::ToString(engine->_srcInvalid)
    ));
    
    scrollDelta = {0, -1};
    engine->InvalidateScroll(&scrollDelta);
    Log::Comment(NoThrowString().Format(
        VerifyOutputTraits<SMALL_RECT>::ToString(engine->_srcInvalid)
    ));
    
    TestPaintXterm(*engine, [&]()
    {
        Log::Comment(NoThrowString().Format(
            L"---- Scrolled one down and one up, nothing should change ----"
            L" But it still does for now MSFT:14169294"
        ));
        invalid = view.ToInclusive();
        VERIFY_ARE_EQUAL(invalid, engine->_srcInvalid);

        qExpectedInput.push_back(EMPTY_CALLBACK_SENTINEL);
        VERIFY_SUCCEEDED(engine->ScrollFrame());
        WriteCallback(EMPTY_CALLBACK_SENTINEL, 1);
    });
}

void VtRendererTest::Xterm256TestColors()
{
    wil::unique_hfile hFile = wil::unique_hfile(INVALID_HANDLE_VALUE);
    // std::unique_ptr<Xterm256Engine> engine = std::make_unique<Xterm256Engine>(std::move(hFile));
    std::unique_ptr<Xterm256Engine> engine = std::make_unique<Xterm256Engine>(std::move(hFile), SetUpViewport());
    auto pfn = std::bind(&VtRendererTest::WriteCallback, this, std::placeholders::_1, std::placeholders::_2);
    engine->SetTestCallback(pfn);

    // Viewport view = Viewport::FromInclusive(SetUpViewport(*engine));
    Viewport view = SetUpViewport();

    Log::Comment(NoThrowString().Format(
        L"Test changing the text attributes"
    ));

    Log::Comment(NoThrowString().Format(
        L"Begin by setting some test values - FG,BG = (1,2,3), (4,5,6) to start"
        L"These values were picked for ease of formatting raw COLORREF values."
    ));
    qExpectedInput.push_back("\x1b[38;2;1;2;3m"); 
    qExpectedInput.push_back("\x1b[48;2;5;6;7m"); 
    engine->UpdateDrawingBrushes(0x00030201, 0x00070605, 0, false);

    TestPaintXterm(*engine, [&]()
    {
        Log::Comment(NoThrowString().Format(
            L"----Change only the BG----"
        ));
        qExpectedInput.push_back("\x1b[48;2;7;8;9m"); 
        engine->UpdateDrawingBrushes(0x00030201, 0x00090807, 0, false);

        Log::Comment(NoThrowString().Format(
            L"----Change only the FG----"
        ));
        qExpectedInput.push_back("\x1b[38;2;10;11;12m"); 
        engine->UpdateDrawingBrushes(0x000c0b0a, 0x00090807, 0, false);

    });

    TestPaintXterm(*engine, [&]()
    {
        Log::Comment(NoThrowString().Format(
            L"Make sure that color setting persists across EndPaint/StartPaint"
        ));
        qExpectedInput.push_back(EMPTY_CALLBACK_SENTINEL); 
        engine->UpdateDrawingBrushes(0x000c0b0a, 0x00090807, 0, false);
        WriteCallback(EMPTY_CALLBACK_SENTINEL, 1); // This will make sure nothing was written to the callback

    });
}

void VtRendererTest::Xterm256TestCursor()
{
    wil::unique_hfile hFile = wil::unique_hfile(INVALID_HANDLE_VALUE);
    // std::unique_ptr<Xterm256Engine> engine = std::make_unique<Xterm256Engine>(std::move(hFile));
    std::unique_ptr<Xterm256Engine> engine = std::make_unique<Xterm256Engine>(std::move(hFile), SetUpViewport());
    auto pfn = std::bind(&VtRendererTest::WriteCallback, this, std::placeholders::_1, std::placeholders::_2);
    engine->SetTestCallback(pfn);

    // Viewport view = Viewport::FromInclusive(SetUpViewport(*engine));
    Viewport view = SetUpViewport();

    Log::Comment(NoThrowString().Format(
        L"Test moving the cursor around. Every sequence should have both params to CUP explicitly."
    ));
    TestPaintXterm(*engine, [&]()
    {
        qExpectedInput.push_back("\x1b[2;2H"); 
        engine->_MoveCursor({1, 1});

        Log::Comment(NoThrowString().Format(
            L"----Only move X coord----"
        ));
        qExpectedInput.push_back("\x1b[31;2H"); 
        engine->_MoveCursor({1, 30});

        Log::Comment(NoThrowString().Format(
            L"----Only move Y coord----"
        ));
        qExpectedInput.push_back("\x1b[31;31H"); 
        engine->_MoveCursor({30, 30});

        Log::Comment(NoThrowString().Format(
            L"----Sending the same move sends nothing----"
        ));
        qExpectedInput.push_back(EMPTY_CALLBACK_SENTINEL); 
        engine->_MoveCursor({30, 30});
        WriteCallback(EMPTY_CALLBACK_SENTINEL, 1);

        Log::Comment(NoThrowString().Format(
            L"----moving home sends a simple sequence----"
        ));
        qExpectedInput.push_back("\x1b[H"); 
        engine->_MoveCursor({0, 0});

        Log::Comment(NoThrowString().Format(
            L"----move into the line to test some other sequnces----"
        ));
        qExpectedInput.push_back("\x1b[1;8H"); 
        engine->_MoveCursor({7, 0});

        Log::Comment(NoThrowString().Format(
            L"----move down one line (x stays the same)----"
        ));
        qExpectedInput.push_back("\n"); 
        engine->_MoveCursor({7, 1});

        Log::Comment(NoThrowString().Format(
            L"----move to the start of the next line----"
        ));
        qExpectedInput.push_back("\r\n"); 
        engine->_MoveCursor({0, 2});

        Log::Comment(NoThrowString().Format(
            L"----move into the line to test some other sequnces----"
        ));
        qExpectedInput.push_back("\x1b[2;8H"); 
        engine->_MoveCursor({7, 1});

        Log::Comment(NoThrowString().Format(
            L"----move to the start of this line (y stays the same)----"
        ));
        qExpectedInput.push_back("\r"); 
        engine->_MoveCursor({0, 1});

    });

    TestPaintXterm(*engine, [&]()
    {
        Log::Comment(NoThrowString().Format(
            L"Sending the same move across paint calls sends nothing."
            L"The cursor's last \"real\" position was 0,0"
        ));
        qExpectedInput.push_back(EMPTY_CALLBACK_SENTINEL); 
        engine->_MoveCursor({0, 1});
        WriteCallback(EMPTY_CALLBACK_SENTINEL, 1);

        Log::Comment(NoThrowString().Format(
            L"Paint some text at 0,0, then try moving the cursor to where it currently is."
        ));
        qExpectedInput.push_back("\x1b[2;2H"); 
        qExpectedInput.push_back("asdfghjkl");

        const wchar_t* const line = L"asdfghjkl";
        const unsigned char rgWidths[] = {1, 1, 1, 1, 1, 1, 1, 1, 1};
        engine->PaintBufferLine(line, rgWidths, 9, {1,1}, false);

        qExpectedInput.push_back(EMPTY_CALLBACK_SENTINEL); 
        engine->_MoveCursor({10, 1});
        WriteCallback(EMPTY_CALLBACK_SENTINEL, 1);

    });

    // Note that only PaintBufferLine updates the "Real" cursor position, which 
    //  the cursor is moved back to at the end of each paint
    TestPaintXterm(*engine, [&]()
    {
        Log::Comment(NoThrowString().Format(
            L"Sending the same move across paint calls sends nothing."
        ));
        qExpectedInput.push_back(EMPTY_CALLBACK_SENTINEL); 
        engine->_MoveCursor({10, 1});
        WriteCallback(EMPTY_CALLBACK_SENTINEL, 1);
    });
}

void VtRendererTest::XtermTestInvalidate()
{
    wil::unique_hfile hFile = wil::unique_hfile(INVALID_HANDLE_VALUE);
    std::unique_ptr<XtermEngine> engine = std::make_unique<XtermEngine>(std::move(hFile), SetUpViewport(), g_ColorTable, (WORD)COLOR_TABLE_SIZE, false);
    auto pfn = std::bind(&VtRendererTest::WriteCallback, this, std::placeholders::_1, std::placeholders::_2);
    engine->SetTestCallback(pfn);

    Viewport view = SetUpViewport();

    Log::Comment(NoThrowString().Format(
        L"Make sure that invalidating all invalidates the whole viewport."
    ));
    engine->InvalidateAll();
    TestPaintXterm(*engine, [&]()
    {
        VERIFY_ARE_EQUAL(view.ToExclusive(), engine->_srcInvalid);
    });

    Log::Comment(NoThrowString().Format(
        L"Make sure that invalidating anything only invalidates that portion"
    ));
    SMALL_RECT invalid = {1, 1, 1, 1};
    engine->Invalidate(&invalid);
    TestPaintXterm(*engine, [&]()
    {
        VERIFY_ARE_EQUAL(invalid, engine->_srcInvalid);
    });

    Log::Comment(NoThrowString().Format(
        L"Make sure that scrolling only invalidates part of the viewport, and sends the right sequences"
    ));
    COORD scrollDelta = {0, 1};
    engine->InvalidateScroll(&scrollDelta);
    TestPaintXterm(*engine, [&]()
    {
        Log::Comment(NoThrowString().Format(
            L"---- Scrolled one down, only top line is invalid. ----"
        ));
        invalid = view.ToInclusive();
        invalid.Bottom = 1;

        VERIFY_ARE_EQUAL(invalid, engine->_srcInvalid);
        // We would expect a CUP here, but the cursor is already at the home position
        // qExpectedInput.push_back("\x1b[H"); 
        
        qExpectedInput.push_back("\x1b[L"); // insert a line
        VERIFY_SUCCEEDED(engine->ScrollFrame());
    });

    scrollDelta = {0, 3};
    engine->InvalidateScroll(&scrollDelta);
    TestPaintXterm(*engine, [&]()
    {
        Log::Comment(NoThrowString().Format(
            L"---- Scrolled three down, only top 3 lines are invalid. ----"
        ));
        invalid = view.ToInclusive();
        invalid.Bottom = 3;

        VERIFY_ARE_EQUAL(invalid, engine->_srcInvalid);
        // We would expect a CUP here, but the cursor is already at the home position
        qExpectedInput.push_back("\x1b[3L"); // insert 3 lines
        VERIFY_SUCCEEDED(engine->ScrollFrame());
    });

    scrollDelta = {0, -1};
    engine->InvalidateScroll(&scrollDelta);
    TestPaintXterm(*engine, [&]()
    {
        Log::Comment(NoThrowString().Format(
            L"---- Scrolled one up, only bottom line is invalid. ----"
        ));
        invalid = view.ToInclusive();
        invalid.Top = invalid.Bottom - 1;

        VERIFY_ARE_EQUAL(invalid, engine->_srcInvalid);
        // We would expect a CUP here, but the cursor is already at the home position
        // qExpectedInput.push_back("\x1b[H"); 
        
        qExpectedInput.push_back("\x1b[M"); // delete a line
        VERIFY_SUCCEEDED(engine->ScrollFrame());
    });

    scrollDelta = {0, -3};
    engine->InvalidateScroll(&scrollDelta);
    TestPaintXterm(*engine, [&]()
    {
        Log::Comment(NoThrowString().Format(
            L"---- Scrolled three up, only bottom 3 lines are invalid. ----"
        ));
        invalid = view.ToInclusive();
        invalid.Top = invalid.Bottom - 3;

        VERIFY_ARE_EQUAL(invalid, engine->_srcInvalid);
        // We would expect a CUP here, but the cursor is already at the home position
        qExpectedInput.push_back("\x1b[3M"); // delete 3 lines
        VERIFY_SUCCEEDED(engine->ScrollFrame());
    });

    Log::Comment(NoThrowString().Format(
        L"Multiple scrolls are coalesced"
    ));

    scrollDelta = {0, 1};
    engine->InvalidateScroll(&scrollDelta);
    scrollDelta = {0, 2};
    engine->InvalidateScroll(&scrollDelta);
    TestPaintXterm(*engine, [&]()
    {
        Log::Comment(NoThrowString().Format(
            L"---- Scrolled three down, only top 3 lines are invalid. ----"
        ));
        invalid = view.ToInclusive();
        invalid.Bottom = 3;

        VERIFY_ARE_EQUAL(invalid, engine->_srcInvalid);
        // We would expect a CUP here, but the cursor is already at the home position
        qExpectedInput.push_back("\x1b[3L"); // insert 3 lines
        VERIFY_SUCCEEDED(engine->ScrollFrame());
    });

    scrollDelta = {0, 1};
    engine->InvalidateScroll(&scrollDelta);
    Log::Comment(NoThrowString().Format(
        VerifyOutputTraits<SMALL_RECT>::ToString(engine->_srcInvalid)
    ));
    
    scrollDelta = {0, -1};
    engine->InvalidateScroll(&scrollDelta);
    Log::Comment(NoThrowString().Format(
        VerifyOutputTraits<SMALL_RECT>::ToString(engine->_srcInvalid)
    ));
    
    TestPaintXterm(*engine, [&]()
    {
        Log::Comment(NoThrowString().Format(
            L"---- Scrolled one down and one up, nothing should change ----"
            L" But it still does for now MSFT:14169294"
        ));
        invalid = view.ToInclusive();
        VERIFY_ARE_EQUAL(invalid, engine->_srcInvalid);

        qExpectedInput.push_back(EMPTY_CALLBACK_SENTINEL);
        VERIFY_SUCCEEDED(engine->ScrollFrame());
        WriteCallback(EMPTY_CALLBACK_SENTINEL, 1);
    });
}

void VtRendererTest::XtermTestColors()
{
    wil::unique_hfile hFile = wil::unique_hfile(INVALID_HANDLE_VALUE);
    std::unique_ptr<XtermEngine> engine = std::make_unique<XtermEngine>(std::move(hFile), SetUpViewport(), g_ColorTable, (WORD)COLOR_TABLE_SIZE, false);
    auto pfn = std::bind(&VtRendererTest::WriteCallback, this, std::placeholders::_1, std::placeholders::_2);
    engine->SetTestCallback(pfn);

    Viewport view = SetUpViewport();

    Log::Comment(NoThrowString().Format(
        L"Test changing the text attributes"
    ));

    Log::Comment(NoThrowString().Format(
        L"Begin by setting the default colors - FG,BG = BRIGHT_WHITE,DARK_BLACK"
    ));
    qExpectedInput.push_back("\x1b[1m\x1b[37m"); // Foreground BRIGHT_WHITE
    qExpectedInput.push_back("\x1b[40m"); // Background DARK_BLACK
    engine->UpdateDrawingBrushes(g_ColorTable[15], g_ColorTable[0], 0, false);

    TestPaintXterm(*engine, [&]()
    {
        Log::Comment(NoThrowString().Format(
            L"----Change only the BG----"
        ));
        qExpectedInput.push_back("\x1b[41m"); // Background DARK_RED
        engine->UpdateDrawingBrushes(g_ColorTable[15], g_ColorTable[4], 0, false);

        Log::Comment(NoThrowString().Format(
            L"----Change only the FG----"
        ));
        qExpectedInput.push_back("\x1b[22m\x1b[37m"); // Foreground DARK_WHITE
        engine->UpdateDrawingBrushes(g_ColorTable[7], g_ColorTable[4], 0, false);

        Log::Comment(NoThrowString().Format(
            L"----Change only the BG to something not in the table----"
        ));
        qExpectedInput.push_back("\x1b[40m"); // Background DARK_BLACK
        engine->UpdateDrawingBrushes(g_ColorTable[7], 0x000000, 0, false);


        Log::Comment(NoThrowString().Format(
            L"----Back to defaults----"
        ));
        qExpectedInput.push_back("\x1b[1m\x1b[37m"); // Foreground BRIGHT_WHITE
        qExpectedInput.push_back("\x1b[40m"); // Background DARK_BLACK
        engine->UpdateDrawingBrushes(g_ColorTable[15], g_ColorTable[0], 0, false);
    });

    TestPaintXterm(*engine, [&]()
    {
        Log::Comment(NoThrowString().Format(
            L"Make sure that color setting persists across EndPaint/StartPaint"
        ));
        qExpectedInput.push_back(EMPTY_CALLBACK_SENTINEL); 
        engine->UpdateDrawingBrushes(g_ColorTable[15], g_ColorTable[0], 0, false);
        WriteCallback(EMPTY_CALLBACK_SENTINEL, 1); // This will make sure nothing was written to the callback

    });

}

void VtRendererTest::XtermTestCursor()
{
    wil::unique_hfile hFile = wil::unique_hfile(INVALID_HANDLE_VALUE);
    std::unique_ptr<XtermEngine> engine = std::make_unique<XtermEngine>(std::move(hFile), SetUpViewport(), g_ColorTable, (WORD)COLOR_TABLE_SIZE, false);
    auto pfn = std::bind(&VtRendererTest::WriteCallback, this, std::placeholders::_1, std::placeholders::_2);
    engine->SetTestCallback(pfn);

    Viewport view = SetUpViewport();
    
    Log::Comment(NoThrowString().Format(
        L"Test moving the cursor around. Every sequence should have both params to CUP explicitly."
    ));
    TestPaintXterm(*engine, [&]()
    {
        qExpectedInput.push_back("\x1b[2;2H"); 
        engine->_MoveCursor({1, 1});

        Log::Comment(NoThrowString().Format(
            L"----Only move X coord----"
        ));
        qExpectedInput.push_back("\x1b[31;2H"); 
        engine->_MoveCursor({1, 30});

        Log::Comment(NoThrowString().Format(
            L"----Only move Y coord----"
        ));
        qExpectedInput.push_back("\x1b[31;31H"); 
        engine->_MoveCursor({30, 30});

        Log::Comment(NoThrowString().Format(
            L"----Sending the same move sends nothing----"
        ));
        qExpectedInput.push_back(EMPTY_CALLBACK_SENTINEL); 
        engine->_MoveCursor({30, 30});
        WriteCallback(EMPTY_CALLBACK_SENTINEL, 1);

        Log::Comment(NoThrowString().Format(
            L"----moving home sends a simple sequence----"
        ));
        qExpectedInput.push_back("\x1b[H"); 
        engine->_MoveCursor({0, 0});

        Log::Comment(NoThrowString().Format(
            L"----move into the line to test some other sequnces----"
        ));
        qExpectedInput.push_back("\x1b[1;8H"); 
        engine->_MoveCursor({7, 0});

        Log::Comment(NoThrowString().Format(
            L"----move down one line (x stays the same)----"
        ));
        qExpectedInput.push_back("\n"); 
        engine->_MoveCursor({7, 1});

        Log::Comment(NoThrowString().Format(
            L"----move to the start of the next line----"
        ));
        qExpectedInput.push_back("\r\n"); 
        engine->_MoveCursor({0, 2});

        Log::Comment(NoThrowString().Format(
            L"----move into the line to test some other sequnces----"
        ));
        qExpectedInput.push_back("\x1b[2;8H"); 
        engine->_MoveCursor({7, 1});

        Log::Comment(NoThrowString().Format(
            L"----move to the start of this line (y stays the same)----"
        ));
        qExpectedInput.push_back("\r"); 
        engine->_MoveCursor({0,1});

    });

    TestPaintXterm(*engine, [&]()
    {
        Log::Comment(NoThrowString().Format(
            L"Sending the same move across paint calls sends nothing."
            L"The cursor's last \"real\" position was 0,0"
        ));
        qExpectedInput.push_back(EMPTY_CALLBACK_SENTINEL); 
        engine->_MoveCursor({0,1});
        WriteCallback(EMPTY_CALLBACK_SENTINEL, 1);

        Log::Comment(NoThrowString().Format(
            L"Paint some text at 0,0, then try moving the cursor to where it currently is."
        ));
        qExpectedInput.push_back("\x1b[2;2H"); 
        qExpectedInput.push_back("asdfghjkl");

        const wchar_t* const line = L"asdfghjkl";
        const unsigned char rgWidths[] = {1, 1, 1, 1, 1, 1, 1, 1, 1};
        engine->PaintBufferLine(line, rgWidths, 9, {1,1}, false);

        qExpectedInput.push_back(EMPTY_CALLBACK_SENTINEL); 
        engine->_MoveCursor({10, 1});
        WriteCallback(EMPTY_CALLBACK_SENTINEL, 1);

    });

    // Note that only PaintBufferLine updates the "Real" cursor position, which 
    //  the cursor is moved back to at the end of each paint
    TestPaintXterm(*engine, [&]()
    {
        Log::Comment(NoThrowString().Format(
            L"Sending the same move across paint calls sends nothing."
        ));
        qExpectedInput.push_back(EMPTY_CALLBACK_SENTINEL); 
        engine->_MoveCursor({10, 1});
        WriteCallback(EMPTY_CALLBACK_SENTINEL, 1);
    });

}

void VtRendererTest::WinTelnetTestInvalidate()
{
    wil::unique_hfile hFile = wil::unique_hfile(INVALID_HANDLE_VALUE);
    std::unique_ptr<WinTelnetEngine> engine = std::make_unique<WinTelnetEngine>(std::move(hFile), SetUpViewport(), g_ColorTable, (WORD)COLOR_TABLE_SIZE);
    auto pfn = std::bind(&VtRendererTest::WriteCallback, this, std::placeholders::_1, std::placeholders::_2);
    engine->SetTestCallback(pfn);

    Viewport view = SetUpViewport();

    Log::Comment(NoThrowString().Format(
        L"Make sure that invalidating all invalidates the whole viewport."
    ));
    engine->InvalidateAll();
    TestPaint(*engine, [&]()
    {
        VERIFY_ARE_EQUAL(view.ToExclusive(), engine->_srcInvalid);
    });

    Log::Comment(NoThrowString().Format(
        L"Make sure that invalidating anything only invalidates that portion"
    ));
    SMALL_RECT invalid = {1, 1, 1, 1};
    engine->Invalidate(&invalid);
    TestPaint(*engine, [&]()
    {
        VERIFY_ARE_EQUAL(invalid, engine->_srcInvalid);
    });
    
    Log::Comment(NoThrowString().Format(
        L"Make sure that scrolling invalidates the whole viewport, and sends no VT sequences"
    ));
    COORD scrollDelta = {0, 1};
    engine->InvalidateScroll(&scrollDelta);
    TestPaint(*engine, [&]()
    {
        VERIFY_ARE_EQUAL(view.ToExclusive(), engine->_srcInvalid);
        qExpectedInput.push_back(EMPTY_CALLBACK_SENTINEL); // sentinel
        VERIFY_SUCCEEDED(engine->ScrollFrame());
        WriteCallback(EMPTY_CALLBACK_SENTINEL, 1); // This will make sure nothing was written to the callback
    });

    scrollDelta = {0, -1};
    engine->InvalidateScroll(&scrollDelta);
    TestPaint(*engine, [&]()
    {
        VERIFY_ARE_EQUAL(view.ToExclusive(), engine->_srcInvalid);
        qExpectedInput.push_back(EMPTY_CALLBACK_SENTINEL);
        VERIFY_SUCCEEDED(engine->ScrollFrame());
        WriteCallback(EMPTY_CALLBACK_SENTINEL, 1); // This will make sure nothing was written to the callback
    });

    scrollDelta = {1, 0};
    engine->InvalidateScroll(&scrollDelta);
    TestPaint(*engine, [&]()
    {
        VERIFY_ARE_EQUAL(view.ToExclusive(), engine->_srcInvalid);
        qExpectedInput.push_back(EMPTY_CALLBACK_SENTINEL);
        VERIFY_SUCCEEDED(engine->ScrollFrame());
        WriteCallback(EMPTY_CALLBACK_SENTINEL, 1); // This will make sure nothing was written to the callback
    });

    scrollDelta = {-1, 0};
    engine->InvalidateScroll(&scrollDelta);
    TestPaint(*engine, [&]()
    {
        VERIFY_ARE_EQUAL(view.ToExclusive(), engine->_srcInvalid);
        qExpectedInput.push_back(EMPTY_CALLBACK_SENTINEL);
        VERIFY_SUCCEEDED(engine->ScrollFrame());
        WriteCallback(EMPTY_CALLBACK_SENTINEL, 1); // This will make sure nothing was written to the callback
    });

    scrollDelta = {1, -1};
    engine->InvalidateScroll(&scrollDelta);
    TestPaint(*engine, [&]()
    {
        VERIFY_ARE_EQUAL(view.ToExclusive(), engine->_srcInvalid);
        qExpectedInput.push_back(EMPTY_CALLBACK_SENTINEL);
        VERIFY_SUCCEEDED(engine->ScrollFrame());
        WriteCallback(EMPTY_CALLBACK_SENTINEL, 1); // This will make sure nothing was written to the callback
    });
    
}

void VtRendererTest::WinTelnetTestColors()
{
    wil::unique_hfile hFile = wil::unique_hfile(INVALID_HANDLE_VALUE);
    std::unique_ptr<WinTelnetEngine> engine = std::make_unique<WinTelnetEngine>(std::move(hFile), SetUpViewport(), g_ColorTable, (WORD)COLOR_TABLE_SIZE);
    auto pfn = std::bind(&VtRendererTest::WriteCallback, this, std::placeholders::_1, std::placeholders::_2);
    engine->SetTestCallback(pfn);

    Viewport view = SetUpViewport();

    Log::Comment(NoThrowString().Format(
        L"Test changing the text attributes"
    ));

    Log::Comment(NoThrowString().Format(
        L"Begin by setting the default colors - FG,BG = BRIGHT_WHITE,DARK_BLACK"
    ));
    qExpectedInput.push_back("\x1b[1m\x1b[37m"); // Foreground BRIGHT_WHITE
    qExpectedInput.push_back("\x1b[40m"); // Background DARK_BLACK
    engine->UpdateDrawingBrushes(g_ColorTable[15], g_ColorTable[0], 0, false);

    TestPaint(*engine, [&]()
    {
        Log::Comment(NoThrowString().Format(
            L"----Change only the BG----"
        ));
        qExpectedInput.push_back("\x1b[41m"); // Background DARK_RED
        engine->UpdateDrawingBrushes(g_ColorTable[15], g_ColorTable[4], 0, false);

        Log::Comment(NoThrowString().Format(
            L"----Change only the FG----"
        ));
        qExpectedInput.push_back("\x1b[22m\x1b[37m"); // Foreground DARK_WHITE
        engine->UpdateDrawingBrushes(g_ColorTable[7], g_ColorTable[4], 0, false);

        Log::Comment(NoThrowString().Format(
            L"----Change only the BG to something not in the table----"
        ));
        qExpectedInput.push_back("\x1b[40m"); // Background DARK_BLACK
        engine->UpdateDrawingBrushes(g_ColorTable[7], 0x000000, 0, false);


        Log::Comment(NoThrowString().Format(
            L"----Back to defaults----"
        ));
        qExpectedInput.push_back("\x1b[1m\x1b[37m"); // Foreground BRIGHT_WHITE
        qExpectedInput.push_back("\x1b[40m"); // Background DARK_BLACK
        engine->UpdateDrawingBrushes(g_ColorTable[15], g_ColorTable[0], 0, false);
    });

    TestPaint(*engine, [&]()
    {
        Log::Comment(NoThrowString().Format(
            L"Make sure that color setting persists across EndPaint/StartPaint"
        ));
        qExpectedInput.push_back(EMPTY_CALLBACK_SENTINEL); 
        engine->UpdateDrawingBrushes(g_ColorTable[15], g_ColorTable[0], 0, false);
        WriteCallback(EMPTY_CALLBACK_SENTINEL, 1); // This will make sure nothing was written to the callback

    });
}

void VtRendererTest::WinTelnetTestCursor()
{
    wil::unique_hfile hFile = wil::unique_hfile(INVALID_HANDLE_VALUE);
    std::unique_ptr<WinTelnetEngine> engine = std::make_unique<WinTelnetEngine>(std::move(hFile), SetUpViewport(), g_ColorTable, (WORD)COLOR_TABLE_SIZE);
    auto pfn = std::bind(&VtRendererTest::WriteCallback, this, std::placeholders::_1, std::placeholders::_2);
    engine->SetTestCallback(pfn);

    Viewport view = SetUpViewport();

    Log::Comment(NoThrowString().Format(
        L"Test moving the cursor around. Every sequence should have both params to CUP explicitly."
    ));
    TestPaint(*engine, [&]()
    {
        qExpectedInput.push_back("\x1b[2;2H"); 
        engine->_MoveCursor({1, 1});

        Log::Comment(NoThrowString().Format(
            L"----Only move X coord----"
        ));
        qExpectedInput.push_back("\x1b[31;2H"); 
        engine->_MoveCursor({1, 30});

        Log::Comment(NoThrowString().Format(
            L"----Only move Y coord----"
        ));
        qExpectedInput.push_back("\x1b[31;31H"); 
        engine->_MoveCursor({30, 30});

        Log::Comment(NoThrowString().Format(
            L"----Sending the same move sends nothing----"
        ));
        qExpectedInput.push_back(EMPTY_CALLBACK_SENTINEL); 
        engine->_MoveCursor({30, 30});
        WriteCallback(EMPTY_CALLBACK_SENTINEL, 1);

        // The "real" location is the last place the cursor was moved to not 
        //  during the course of VT operations - eg the last place text was written,
        //  or the cursor was manually painted at (MSFT 13310327)
        Log::Comment(NoThrowString().Format(
            L"Make sure the cursor gets moved back to the last real location it was at"
        ));
        qExpectedInput.push_back("\x1b[1;1H");
        // EndPaint will send this sequence for us.
    });

    TestPaint(*engine, [&]()
    {
        Log::Comment(NoThrowString().Format(
            L"Sending the same move across paint calls sends nothing."
            L"The cursor's last \"real\" position was 0,0"
        ));
        qExpectedInput.push_back(EMPTY_CALLBACK_SENTINEL); 
        engine->_MoveCursor({0, 0});
        WriteCallback(EMPTY_CALLBACK_SENTINEL, 1);

        Log::Comment(NoThrowString().Format(
            L"Paint some text at 0,0, then try moving the cursor to where it currently is."
        ));
        qExpectedInput.push_back("\x1b[2;2H"); 
        qExpectedInput.push_back("asdfghjkl");

        const wchar_t* const line = L"asdfghjkl";
        const unsigned char rgWidths[] = {1, 1, 1, 1, 1, 1, 1, 1, 1};
        engine->PaintBufferLine(line, rgWidths, 9, {1,1}, false);

        qExpectedInput.push_back(EMPTY_CALLBACK_SENTINEL); 
        engine->_MoveCursor({10, 1});
        WriteCallback(EMPTY_CALLBACK_SENTINEL, 1);

    });

    // Note that only PaintBufferLine updates the "Real" cursor position, which 
    //  the cursor is moved back to at the end of each paint
    TestPaint(*engine, [&]()
    {
        Log::Comment(NoThrowString().Format(
            L"Sending the same move across paint calls sends nothing."
        ));
        qExpectedInput.push_back(EMPTY_CALLBACK_SENTINEL); 
        engine->_MoveCursor({10, 1});
        WriteCallback(EMPTY_CALLBACK_SENTINEL, 1);
    });
}
