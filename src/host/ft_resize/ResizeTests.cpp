/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#include "precomp.h"
#include "WexTestClass.h"

using namespace WEX::Logging;
using namespace WEX::TestExecution;
using namespace WEX::Common;

const WCHAR c_wszLoremIpsum[] = L"きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き";
const WCHAR c_wchNewLine = L'\n';

const WCHAR c_wszExpectedPreResize[] =
    L"きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き________________"
    L"きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き________________"
    L"きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き________________"
    L"きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き________________"
    L"きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き________________"
    L"きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き________________"
    L"きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き________________"
    L"きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き________________"
    L"きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き________________"
    L"きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_________________________________________________________________________________________________";

const WCHAR c_wszExpectedPostResize[] =
    L"きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き______"
    L"きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き______"
    L"きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き______"
    L"きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き______"
    L"きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き______"
    L"きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き______"
    L"きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き______"
    L"きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き______"
    L"きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き______"
    L"きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き__________________________________________________________________________________흩쵚฀"
    L"きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き________________"
    L"きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き________________"
    L"きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き________________"
    L"きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き________________"
    L"きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き________________"
    L"きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き________________"
    L"きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き________________"
    L"きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き________________"
    L"きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き________________"
    L"きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_きABCDEFGHIJKLMNOPQRSTUVWXYZきき0123456789き_________________________________________________________________________________________________";

class ResizeTests
{
    HANDLE m_hScreenBuffer = INVALID_HANDLE_VALUE;

    enum ResizeDirection
    {
        SizingUp,
        SizingDown
    };

    TEST_CLASS(ResizeTests);

    static bool s_IsRunningConhostV2WithLineWrap()
    {
        bool fRet = false;

        DWORD dwForceV2;
        DWORD cbForceV2 = sizeof(dwForceV2);
        if (ERROR_SUCCESS == RegGetValue(HKEY_CURRENT_USER, L"Console", L"ForceV2", RRF_RT_DWORD, NULL, &dwForceV2, &cbForceV2))
        {
            DWORD dwLineWrap;
            DWORD cbLineWrap = sizeof(dwLineWrap);
            if (ERROR_SUCCESS == RegGetValue(HKEY_CURRENT_USER, L"Console", L"LineWrap", RRF_RT_DWORD, NULL, &dwLineWrap, &cbLineWrap))
            {
                fRet = (cbForceV2 == sizeof(DWORD) &&
                        cbLineWrap == sizeof(DWORD) &&
                        dwForceV2 == (DWORD)1 &&
                        dwLineWrap == (DWORD)1);
            }
        }

        return fRet;
    }

    void m_GetCurrentBufferSize(_Out_ PCOORD pcoordOut) const
    {
        CONSOLE_SCREEN_BUFFER_INFO currentScreenBufferInfo;
        VERIFY_WIN32_BOOL_SUCCEEDED(GetConsoleScreenBufferInfo(m_hScreenBuffer, &currentScreenBufferInfo));
        (*pcoordOut) = currentScreenBufferInfo.dwSize;
    }

    void m_LogCurrentConsoleScreenBufferInfo(_In_ PCWSTR pwszLead) const
    {
        CONSOLE_SCREEN_BUFFER_INFO currentScreenBufferInfo;
        if (VERIFY_WIN32_BOOL_SUCCEEDED_RETURN(GetConsoleScreenBufferInfo(m_hScreenBuffer, &currentScreenBufferInfo)))
        {
            Log::Comment(String().Format(L"%s: Screen buffer: %d columns %d rows",
                                         pwszLead,
                                         currentScreenBufferInfo.dwSize.X,
                                         currentScreenBufferInfo.dwSize.Y));
            Log::Comment(String().Format(L"%s: Current window size: %d columns %d rows",
                                         pwszLead,
                                         currentScreenBufferInfo.srWindow.Right - currentScreenBufferInfo.srWindow.Left,
                                         currentScreenBufferInfo.srWindow.Bottom - currentScreenBufferInfo.srWindow.Top));
            Log::Comment(String().Format(L"%s: Current window origin: (%d, %d)",
                                         pwszLead,
                                         currentScreenBufferInfo.srWindow.Left,
                                         currentScreenBufferInfo.srWindow.Top));
            Log::Comment(String().Format(L"%s: Maximum allowable window size: %d columns %d rows",
                                         pwszLead,
                                         currentScreenBufferInfo.dwMaximumWindowSize.X,
                                         currentScreenBufferInfo.dwMaximumWindowSize.Y));
        }
    }

    void m_ResizeWindow(_In_ const SHORT desiredX, _In_ const SHORT desiredY) const
    {
        // adjust window size
        SMALL_RECT srTargetSize;
        srTargetSize.Left = 0;
        srTargetSize.Top = 0;
        srTargetSize.Right = desiredX;
        srTargetSize.Bottom = desiredY;
        VERIFY_WIN32_BOOL_SUCCEEDED(SetConsoleWindowInfo(m_hScreenBuffer, TRUE /*bAbsolute*/, &srTargetSize));
    }

    void m_ResizeBuffer(_In_ const SHORT desiredX, _In_ const SHORT desiredY) const
    {
        // adjust buffer size
        COORD coordBufferSize;
        coordBufferSize.X = desiredX;
        coordBufferSize.Y = desiredY;
        VERIFY_WIN32_BOOL_SUCCEEDED(SetConsoleScreenBufferSize(m_hScreenBuffer, coordBufferSize));
    }

    void m_ResizeWindowAndBuffer(_In_ const SHORT desiredX, _In_ const SHORT desiredY, _In_ const ResizeDirection direction) const
    {
        // when sizing down, you must change the window size before the buffer. when sizing up, the opposite is true.
        if (direction == SizingDown)
        {
            m_ResizeWindow(desiredX, desiredY);
            m_ResizeBuffer(desiredX+1, desiredY+1);
        }
        else
        {
            m_ResizeBuffer(desiredX+1, desiredY+1);
            m_ResizeWindow(desiredX, desiredY);
        }
    }

    void m_GetFullBufferText(_Outptr_ PWSTR * const ppszBufferText, _Out_ size_t * const pCchBufferText) const
    {
        COORD coordBufferSize;
        m_GetCurrentBufferSize(&coordBufferSize);
        const DWORD cchBuffer = coordBufferSize.X * coordBufferSize.Y;
        Log::Comment(String().Format(L"current buffer cch: %d", cchBuffer));

        PWSTR pszBuffer = new WCHAR[cchBuffer+1];
        VERIFY_IS_NOT_NULL(pszBuffer);

        COORD coordOrigin;
        coordOrigin.X = 0;
        coordOrigin.Y = 0;
        DWORD cchRead = 0;
        VERIFY_WIN32_BOOL_SUCCEEDED(ReadConsoleOutputCharacterW(m_hScreenBuffer, pszBuffer, cchBuffer, coordOrigin, &cchRead));
        Log::Comment(String().Format(L"read-back buffer cch: %d", cchRead));

        // each DBCS char uses two screen buffers
        VERIFY_IS_GREATER_THAN_OR_EQUAL(cchBuffer, cchRead);

        // properly terminate the buffer as C-style string
        pszBuffer[cchRead] = L'\0';

        *ppszBufferText = pszBuffer;
        *pCchBufferText = cchRead;
    }

    TEST_METHOD_SETUP(MethodSetup)
    {
        m_hScreenBuffer = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE,
                                                    0 /*dwShareMode*/,
                                                    NULL /*lpSecurityAttributes*/,
                                                    CONSOLE_TEXTMODE_BUFFER,
                                                    NULL /*lpReserved*/);
        VERIFY_ARE_NOT_EQUAL(m_hScreenBuffer, INVALID_HANDLE_VALUE);

        VERIFY_WIN32_BOOL_SUCCEEDED(SetConsoleActiveScreenBuffer(m_hScreenBuffer));

        m_LogCurrentConsoleScreenBufferInfo(L"BeforeInitialSizing");
        m_ResizeWindowAndBuffer(80, 20, SizingDown);
        m_LogCurrentConsoleScreenBufferInfo(L"AfterInitialSizing");

        // write 10 rows of ~120 char sample text
        for (UINT cNumRows = 0; cNumRows < 10; cNumRows++)
        {
            UINT cNumWritten = 0;
            do
            {
                DWORD cchWritten = 0;
                // The array includes a trailing NULL char, which is written to console and displays as a blank space
                VERIFY_WIN32_BOOL_SUCCEEDED(WriteConsole(m_hScreenBuffer, c_wszLoremIpsum, ARRAYSIZE(c_wszLoremIpsum), &cchWritten, NULL /*lpReserved*/));
                VERIFY_ARE_EQUAL(ARRAYSIZE(c_wszLoremIpsum), cchWritten);
                cNumWritten += (UINT)cchWritten;
            }
            while (cNumWritten <= 120);

            DWORD cchNewline = 0;
            VERIFY_WIN32_BOOL_SUCCEEDED(WriteConsole(m_hScreenBuffer, &c_wchNewLine, 1, &cchNewline, NULL /*lpReserved*/));
            VERIFY_ARE_EQUAL(cchNewline, (DWORD)1);
        }

        Log::Comment(String().Format(L"%s", c_wszLoremIpsum));

        return true;
    }

    TEST_METHOD_CLEANUP(MethodCleanup)
    {
        if (m_hScreenBuffer != INVALID_HANDLE_VALUE)
        {
            CloseHandle(m_hScreenBuffer);
        }
        return true;
    }

    // The output we get from ReadConsoleOutputCharacterW contains embedded nulls and spaces. This function replaces
    // instances of a single embedded null with '^', and all spaces with '_' to aid debugging.
    void m_UnNullBuffer(_In_reads_(cchBuffer) PWSTR pszBuffer, _In_ const size_t cchBuffer)
    {
        bool fLastWasNull = false;
        for (UINT i = 0; i < cchBuffer; i++)
        {
            if (pszBuffer[i] == L'\0')
            {
                if (fLastWasNull)
                {
                    break;
                }
                else
                {
                    pszBuffer[i] = L'^';
                    fLastWasNull = true;
                }
                continue;
            }

            fLastWasNull = false;

            if (pszBuffer[i] == L' ')
            {
                pszBuffer[i] = L'_';
            }
        }
    }

    TEST_METHOD(TestResize)
    {
        // first, get the current buffer of text
        PWSTR pszPreResizeBuffer;
        size_t cchPreResizeBuffer;
        m_GetFullBufferText(&pszPreResizeBuffer, &cchPreResizeBuffer);

        // now resize it 5 columns smaller, and then get the text
        m_ResizeWindowAndBuffer(75, 20, SizingDown);

        PWSTR pszPostResizeBuffer;
        size_t cchPostResizeBuffer;
        m_GetFullBufferText(&pszPostResizeBuffer, &cchPostResizeBuffer);

        // now resize it back to its original size
        m_ResizeWindowAndBuffer(80, 20, SizingUp);

        PWSTR pszUnresizedBuffer;
        size_t cchUnresizedBuffer;
        m_GetFullBufferText(&pszUnresizedBuffer, &cchUnresizedBuffer);

        // We only want to perform verification if we're running on a machine that supports line wrap
        if (s_IsRunningConhostV2WithLineWrap())
        {
            // strip out embedded nulls and convert spaces
            m_UnNullBuffer(pszPreResizeBuffer, cchPreResizeBuffer);
            m_UnNullBuffer(pszPostResizeBuffer, cchPostResizeBuffer);
            m_UnNullBuffer(pszUnresizedBuffer, cchUnresizedBuffer);

            // validate state
            Log::Comment(String().Format(L"pszPreResizeBuffer:\n\"%s\"\n", pszPreResizeBuffer));
            Log::Comment(String().Format(L"c_wszExpectedPreResize:\n\"%s\"\n", c_wszExpectedPreResize));
            Log::Comment(String().Format(L"pszPostResizeBuffer:\n\"%s\"\n", pszPostResizeBuffer));
            Log::Comment(String().Format(L"c_wszExpectedPostResize:\n\"%s\"\n", c_wszExpectedPostResize));
            Log::Comment(String().Format(L"pszUnresizedBuffer:\n\"%s\"\n", pszUnresizedBuffer));
            VERIFY_ARE_EQUAL(0, memcmp(pszPreResizeBuffer, c_wszExpectedPreResize, cchPreResizeBuffer*sizeof(WCHAR)));
            VERIFY_ARE_EQUAL(0, memcmp(pszPostResizeBuffer, c_wszExpectedPostResize, cchPostResizeBuffer*sizeof(WCHAR)));
            VERIFY_ARE_EQUAL(cchPreResizeBuffer, cchUnresizedBuffer);
            VERIFY_ARE_EQUAL(0, memcmp(pszPreResizeBuffer, pszUnresizedBuffer, cchPreResizeBuffer*sizeof(WCHAR)));
        }
        else
        {
            Log::Comment(L"Skipping validation since this build doesn't support conhostv2 with linewrapping.");
        }

        delete [] pszPreResizeBuffer;
        delete [] pszPostResizeBuffer;
        delete [] pszUnresizedBuffer;
    }
};
