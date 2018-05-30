/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#define CP_USA 437

class FillOutputTests
{
    TEST_CLASS(FillOutputTests);

    TEST_METHOD(WriteNarrowGlyphAscii)
    {
        HANDLE hConsole = GetStdOutputHandle();
        DWORD charsWritten = 0;
        VERIFY_WIN32_BOOL_SUCCEEDED(FillConsoleOutputCharacterA(hConsole,
                                                                'a',
                                                                1,
                                                                { 0, 0 },
                                                                &charsWritten));
        VERIFY_ARE_EQUAL(1u, charsWritten);

        // test a box drawing character
        const UINT previousCodepage = GetConsoleOutputCP();
        VERIFY_WIN32_BOOL_SUCCEEDED(SetConsoleOutputCP(CP_USA));

        charsWritten = 0;
        VERIFY_WIN32_BOOL_SUCCEEDED(FillConsoleOutputCharacterA(hConsole,
                                                                '\xCE', // U+256C box drawing double vertical and horizontal
                                                                1,
                                                                { 0, 0 },
                                                                &charsWritten));
        VERIFY_ARE_EQUAL(1u, charsWritten);
        VERIFY_SUCCEEDED(SetConsoleOutputCP(previousCodepage));
    }

    TEST_METHOD(WriteNarrowGlyphUnicode)
    {
        HANDLE hConsole = GetStdOutputHandle();
        DWORD charsWritten = 0;
        VERIFY_WIN32_BOOL_SUCCEEDED(FillConsoleOutputCharacterW(hConsole,
                                                                L'a',
                                                                1,
                                                                { 0, 0 },
                                                                &charsWritten));
        VERIFY_ARE_EQUAL(1u, charsWritten);
    }

    TEST_METHOD(WriteWideGlyphUnicode)
    {
        HANDLE hConsole = GetStdOutputHandle();
        DWORD charsWritten = 0;
        VERIFY_WIN32_BOOL_SUCCEEDED(FillConsoleOutputCharacterW(hConsole,
                                                                L'\x304F',
                                                                1,
                                                                { 0, 0 },
                                                                &charsWritten));
        VERIFY_ARE_EQUAL(1u, charsWritten);
    }
};
