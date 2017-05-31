//
//    Copyright (C) Microsoft.  All rights reserved.
//
#include <precomp.h>
#include <windows.h>
#include <wincon.h>
#include <stdlib.h>

void usage()
{
    printf("usage:\n");    
    printf("colortool %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s \n"
        ,"<BLACK>"
        ,"<DARK BLUE>"
        ,"<DARK GREEN>"
        ,"<DARK CYAN>"
        ,"<DARK RED>"
        ,"<DARK MAGENTA>"
        ,"<DARK YELLOW>"
        ,"<DARK WHITE>"
        ,"<BRIGHT BLACK>"
        ,"<BRIGHT BLUE>"
        ,"<BRIGHT GREEN>"
        ,"<BRIGHT CYAN>"
        ,"<BRIGHT RED>"
        ,"<BRIGHT MAGENTA>"
        ,"<BRIGHT YELLOW>"
        ,"<WHITE>"
    );    
    printf("Colors may be specified as hex values (e.g. #002b36) or RGB (e.g. 0,43,54).\n");
    printf("Hex values start with a '#', RGB are three numbers < 256, seperated ONLY by commas.\n");
}

void ParseError(wchar_t* arg)
{
    wprintf(L"Error parsing arg:\"%ls\"", arg);
    exit(EXIT_FAILURE);
}

DWORD ParseHex(wchar_t* arg)
{
    int red = 0;
    int green = 0;
    int blue = 0;
    int* pColor = &red;
    for (int i = 0; i < 6; i++)
    {
        *pColor *= 16;
        wchar_t wch = arg[i+1];
        if (wch >= L'0' && wch <= L'9')
        {
            *pColor += wch - L'0';
        }
        else if (wch >= L'a' && wch <= L'f')
        {
            *pColor += wch - L'a' + 10;
        }
        else if (wch >= L'A' && wch <= L'F')
        {
            *pColor += wch - L'A' + 10;
        }
        else
        {
            ParseError(arg);
        }

        if (i == 1) pColor = &green;
        else if (i == 3) pColor = &blue;
    }

    // printf("parsed hex: (%d, %d, %d)\n", red, green, blue);
    return RGB(red, green, blue);
}

DWORD ParseRgb(wchar_t* arg)
{
    int red = 0;
    int green = 0;
    int blue = 0;
    int* pColor = &red;
    for (int i = 0; i < wcslen(arg); i++)
    {
        wchar_t wch = arg[i];
        if (wch == ',')
        {
            if (pColor == &red) pColor = &green;
            else if (pColor == &green) pColor = &blue;
            else ParseError(arg);
        }
        else 
        {
            *pColor *= 10;
            // printf("   parse: %lc, color: %d\n", wch, *pColor);
            if (wch >= L'0' && wch <= L'9')
            {
                *pColor += wch - L'0';
            }
            else
            {
                ParseError(arg);
            }
        }

    }

    return RGB(red, green, blue);
}

DWORD ParseColor(wchar_t* arg)
{
    // wprintf(L"parsing arg: %ls\n", arg);
    if (arg[0] == L'#')
    {
        return ParseHex(arg);
    }
    else
    {
        return ParseRgb(arg);
    }
}

void PrintTable()
{
    wchar_t* test = L"gYw";
    wchar_t* FGs[] = {
        // L"m"
        L"1m"
        ,L"30m"
        ,L"1;30m"
        ,L"31m"
        ,L"1;31m"
        ,L"32m"
        ,L"1;32m"
        ,L"33m"
        ,L"1;33m"
        ,L"34m"
        ,L"1;34m"
        ,L"35m"
        ,L"1;35m"
        ,L"36m"
        ,L"1;36m"
        ,L"37m"
        ,L"1;37m"
    };
    wchar_t* BGs[] = {
        L"m"
        ,L"40m"
        // ,L"100m"
        ,L"41m"
        // ,L"101m"
        ,L"42m"
        // ,L"102m"
        ,L"43m"
        // ,L"103m"
        ,L"44m"
        // ,L"104m"
        ,L"45m"
        // ,L"105m"
        ,L"46m"
        // ,L"106m"
        ,L"47m"
        // ,L"107m"
    };
    int numFGs = 17;
    int numBGs = 9;
    // printf("                 40m     41m     42m     43m     44m     45m     46m     47m\n");

    wprintf(L"\t");
    for (int i = 0; i < numBGs; i++)
    {
        wprintf(L"  %ls  \t", BGs[i]);
    }
    wprintf(L"\n");
    for (int i = 0; i < numFGs; i++)
    {
        wprintf(L"%ls", FGs[i]);
        for (int j = 0; j < numBGs; j++)
        {
            wprintf(L"\t\x1b[%ls  \x1b[%ls%ls  \x1b[0m", BGs[j], FGs[i], test);
        }
        wprintf(L"\n");
    }

}

int __cdecl wmain(int argc, WCHAR* argv[])
{
    if (argc != 17 && argc != 1)
    {
        usage();
        return -1;
    }

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

    if (hOut != INVALID_HANDLE_VALUE)
    {
        DWORD outModes;
        GetConsoleMode(hOut, &outModes);
        outModes |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, outModes);

        if (argc == 1)
        {
            PrintTable();
        }
        else
        {
            CONSOLE_SCREEN_BUFFER_INFOEX sbiex = { 0 };
            sbiex.cbSize = sizeof(sbiex);

            BOOL fSuccess = GetConsoleScreenBufferInfoEx(hOut, &sbiex);

            if (fSuccess)
            {
                sbiex.srWindow.Bottom++; // hack because the API sucks at roundtrip

                CONSOLE_SCREEN_BUFFER_INFOEX sbiexBackup = sbiex;

                for (int i = 0; i < 16; i++)
                {
                    sbiex.ColorTable[i] = ParseColor(argv[i+1]);
                }

                SetConsoleScreenBufferInfoEx(hOut, &sbiex);
                PrintTable();
                // Run everything else. Do stuff.

                // Before exiting... set colors back to not affect any other applications.
                // SetConsoleScreenBufferInfoEx(hOut, &sbiexBackup);
            }
        }
    }

    return 0;

}

