/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#include "precomp.h"
#include "WexTestClass.h"

#include "globals.h"
#include "..\..\server\ConsoleArguments.hpp"

using namespace WEX::Common;
using namespace WEX::Logging;
using namespace WEX::TestExecution;
using namespace std;

class ConsoleArgumentsTests
{
    TEST_CLASS(ConsoleArgumentsTests);

    TEST_METHOD(VtPipesTest);
    TEST_METHOD(ClientCommandlineTests);
};

ConsoleArguments CreateAndParse(wstring& commandline)
{
    ConsoleArguments args = ConsoleArguments(commandline);
    VERIFY_SUCCEEDED(args.ParseCommandline());
    return args;
}

// Used when you expect args to be invalid
ConsoleArguments CreateAndParseUnsuccessfully(wstring& commandline)
{
    ConsoleArguments args = ConsoleArguments(commandline);
    VERIFY_FAILED(args.ParseCommandline());
    return args;
}

void ConsoleArgumentsTests::VtPipesTest()
{
    // We're using extra scopes here to automatically cleanup the old args
    // and commandline without needing to manually declare a desturctor.
    {
        Log::Comment(L"First look for a valid commandline");
        wstring commandline = L"--inpipe foo --outpipe bar";
        Log::Comment(commandline.c_str());
        auto args = CreateAndParse(commandline);
        VERIFY_IS_TRUE(args.IsUsingVtPipe());
        VERIFY_ARE_EQUAL(args.GetVtInPipe(), L"foo");
        VERIFY_ARE_EQUAL(args.GetVtOutPipe(), L"bar");
        VERIFY_ARE_EQUAL(args.GetClientCommandline(), L"");
    }

    {
        Log::Comment(L"#2 In, but no out");
        wstring commandline = L"--inpipe foo bar";
        Log::Comment(commandline.c_str());
        auto args = CreateAndParse(commandline);
        VERIFY_IS_FALSE(args.IsUsingVtPipe());
        VERIFY_ARE_EQUAL(args.GetVtInPipe(), L"foo");
        VERIFY_ARE_EQUAL(args.GetVtOutPipe(), L"");
        VERIFY_ARE_EQUAL(args.GetClientCommandline(), L"bar");
    }
    
    {
        Log::Comment(L"#3 Out, no in");
        wstring commandline = L"--outpipe foo";
        Log::Comment(commandline.c_str());
        auto args = CreateAndParse(commandline);
        VERIFY_IS_FALSE(args.IsUsingVtPipe());
        VERIFY_ARE_EQUAL(args.GetVtInPipe(), L"");
        VERIFY_ARE_EQUAL(args.GetVtOutPipe(), L"foo");
        VERIFY_ARE_EQUAL(args.GetClientCommandline(), L"");
    }

    {
        Log::Comment(L"#4 Neither");
        wstring commandline = L"foo";
        Log::Comment(commandline.c_str());
        auto args = CreateAndParse(commandline);
        VERIFY_IS_FALSE(args.IsUsingVtPipe());
        VERIFY_ARE_EQUAL(args.GetVtInPipe(), L"");
        VERIFY_ARE_EQUAL(args.GetVtOutPipe(), L"");
        VERIFY_ARE_EQUAL(args.GetClientCommandline(), L"foo");
    }

    {
        Log::Comment(L"#5 Mixed (1)");
        wstring commandline = L"--inpipe --outpipe foo";
        Log::Comment(commandline.c_str());
        auto args = CreateAndParse(commandline);
        VERIFY_IS_FALSE(args.IsUsingVtPipe());
        VERIFY_ARE_EQUAL(args.GetVtInPipe(), L"--outpipe");
        VERIFY_ARE_EQUAL(args.GetVtOutPipe(), L"");
        VERIFY_ARE_EQUAL(args.GetClientCommandline(), L"foo");
    }

    {
        Log::Comment(L"#6 Mixed (2)");
        wstring commandline = L"--inpipe --outpipe --outpipe foo";
        Log::Comment(commandline.c_str());
        auto args = CreateAndParse(commandline);
        VERIFY_IS_TRUE(args.IsUsingVtPipe());
        VERIFY_ARE_EQUAL(args.GetVtInPipe(), L"--outpipe");
        VERIFY_ARE_EQUAL(args.GetVtOutPipe(), L"foo");
        VERIFY_ARE_EQUAL(args.GetClientCommandline(), L"");
    }

    {
        Log::Comment(L"#7 Pipe names in client commandline");
        wstring commandline = L"-- --inpipe foo --outpipe bar";
        Log::Comment(commandline.c_str());
        auto args = CreateAndParse(commandline);
        VERIFY_IS_FALSE(args.IsUsingVtPipe());
        VERIFY_ARE_EQUAL(args.GetVtInPipe(), L"");
        VERIFY_ARE_EQUAL(args.GetVtOutPipe(), L"");
        VERIFY_ARE_EQUAL(args.GetClientCommandline(), L"--inpipe foo --outpipe bar");
    }

    {
        Log::Comment(L"#8 Pipe names the same");
        wstring commandline = L"--inpipe foo --outpipe foo";
        Log::Comment(commandline.c_str());
        auto args = CreateAndParse(commandline);
        VERIFY_IS_TRUE(args.IsUsingVtPipe());
        VERIFY_ARE_EQUAL(args.GetVtInPipe(), L"foo");
        VERIFY_ARE_EQUAL(args.GetVtOutPipe(), L"foo");
        VERIFY_ARE_EQUAL(args.GetClientCommandline(), L"");
    }

    {
        Log::Comment(L"#9 Not enough args (1)");
        wstring commandline = L"--inpipe foo --outpipe";
        Log::Comment(commandline.c_str());
        auto args = CreateAndParseUnsuccessfully(commandline);
        VERIFY_IS_FALSE(args.IsUsingVtPipe());
        VERIFY_ARE_EQUAL(args.GetVtInPipe(), L"foo");
        VERIFY_ARE_EQUAL(args.GetVtOutPipe(), L"");
        VERIFY_ARE_EQUAL(args.GetClientCommandline(), L"");
    }

    {
        Log::Comment(L"#10 Not enough args (2)");
        wstring commandline = L"--inpipe";
        Log::Comment(commandline.c_str());
        auto args = CreateAndParseUnsuccessfully(commandline);
        VERIFY_IS_FALSE(args.IsUsingVtPipe());
        VERIFY_ARE_EQUAL(args.GetVtInPipe(), L"");
        VERIFY_ARE_EQUAL(args.GetVtOutPipe(), L"");
        VERIFY_ARE_EQUAL(args.GetClientCommandline(), L"");
    }
}
void ConsoleArgumentsTests::ClientCommandlineTests()
{
    VERIFY_SUCCEEDED(E_FAIL);
}