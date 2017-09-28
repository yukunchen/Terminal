/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#include "precomp.h"
#include "WexTestClass.h"

#include "globals.h"
#include "../ConsoleArguments.hpp"

using namespace WEX::Common;
using namespace WEX::Logging;
using namespace WEX::TestExecution;
using namespace std;

class ConsoleArgumentsTests
{
    TEST_CLASS(ConsoleArgumentsTests);

    TEST_METHOD(ArgSplittingTests);
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

void ConsoleArgumentsTests::ArgSplittingTests()
{
    // We're using extra scopes here to automatically cleanup the old args
    // and commandline without needing to manually declare a destructor.
    {
        Log::Comment(L"#1 look for a valid commandline");
        wstring commandline = L"--inpipe foo --outpipe bar this is the commandline";
        Log::Comment(commandline.c_str());
        auto args = CreateAndParse(commandline);
        VERIFY_IS_TRUE(args.IsUsingVtPipe());
        VERIFY_ARE_EQUAL(args.GetVtInPipe(), L"foo");
        VERIFY_ARE_EQUAL(args.GetVtOutPipe(), L"bar");
        VERIFY_ARE_EQUAL(args.GetClientCommandline(), L"this is the commandline");
    }
    {
        Log::Comment(L"#2 a commandline with quotes");
        wstring commandline = L"--inpipe foo --outpipe bar \"this is the commandline\"";
        Log::Comment(commandline.c_str());
        auto args = CreateAndParse(commandline);
        VERIFY_IS_TRUE(args.IsUsingVtPipe());
        VERIFY_ARE_EQUAL(args.GetVtInPipe(), L"foo");
        VERIFY_ARE_EQUAL(args.GetVtOutPipe(), L"bar");
        VERIFY_ARE_EQUAL(args.GetClientCommandline(), L"this is the commandline");
    }
    {
        Log::Comment(L"#3 quotes on an arg");
        wstring commandline = L"--inpipe foo \"--outpipe bar this is the commandline\"";
        Log::Comment(commandline.c_str());
        auto args = CreateAndParse(commandline);
        VERIFY_IS_FALSE(args.IsUsingVtPipe());
        VERIFY_ARE_EQUAL(args.GetVtInPipe(), L"foo");
        VERIFY_ARE_EQUAL(args.GetVtOutPipe(), L"");
        VERIFY_ARE_EQUAL(args.GetClientCommandline(), L"--outpipe bar this is the commandline");
    }
    {
        Log::Comment(L"#4 Many spaces");
        wstring commandline = L"--inpipe  foo   --outpipe    bar       this      is the    commandline";
        Log::Comment(commandline.c_str());
        auto args = CreateAndParse(commandline);
        VERIFY_IS_TRUE(args.IsUsingVtPipe());
        VERIFY_ARE_EQUAL(args.GetVtInPipe(), L"foo");
        VERIFY_ARE_EQUAL(args.GetVtOutPipe(), L"bar");
        VERIFY_ARE_EQUAL(args.GetClientCommandline(), L"this is the commandline");
    }
    {
        Log::Comment(L"#5 tab-delimit");
        wstring commandline = L"--inpipe\tfoo\t--outpipe\tbar\tthis\tis\tthe\tcommandline";
        Log::Comment(commandline.c_str());
        auto args = CreateAndParse(commandline);
        VERIFY_IS_TRUE(args.IsUsingVtPipe());
        VERIFY_ARE_EQUAL(args.GetVtInPipe(), L"foo");
        VERIFY_ARE_EQUAL(args.GetVtOutPipe(), L"bar");
        VERIFY_ARE_EQUAL(args.GetClientCommandline(), L"this is the commandline");
    }
    {
        Log::Comment(L"#5 back-slashes won't escape spaces");
        wstring commandline = L"--inpipe\\ foo\\ --outpipe\\ bar\\ this\\ is\\ the\\ commandline";
        Log::Comment(commandline.c_str());
        auto args = CreateAndParse(commandline);
        VERIFY_IS_FALSE(args.IsUsingVtPipe());
        VERIFY_ARE_EQUAL(args.GetVtInPipe(), L"");
        VERIFY_ARE_EQUAL(args.GetVtOutPipe(), L"");
        VERIFY_ARE_EQUAL(args.GetClientCommandline(), L"--inpipe\\ foo\\ --outpipe\\ bar\\ this\\ is\\ the\\ commandline");
    }
    {
        Log::Comment(L"#6 back-slashes won't escape tabs (but the tabs are still converted to spaces)");
        wstring commandline = L"--inpipe\\\tfoo\\\t--outpipe\\\tbar\\\tthis\\\tis\\\tthe\\\tcommandline";
        Log::Comment(commandline.c_str());
        auto args = CreateAndParse(commandline);
        VERIFY_IS_FALSE(args.IsUsingVtPipe());
        VERIFY_ARE_EQUAL(args.GetVtInPipe(), L"");
        VERIFY_ARE_EQUAL(args.GetVtOutPipe(), L"");
        VERIFY_ARE_EQUAL(args.GetClientCommandline(), L"--inpipe\\ foo\\ --outpipe\\ bar\\ this\\ is\\ the\\ commandline");
    }
    {
        Log::Comment(L"#7 Combo of backslashes and quotes from msdn");
        wstring commandline = L"--inpipe a\\\\\\\\\"b c\" d e";
        Log::Comment(commandline.c_str());
        auto args = CreateAndParse(commandline);
        VERIFY_IS_FALSE(args.IsUsingVtPipe());
        VERIFY_ARE_EQUAL(args.GetVtInPipe(), L"a\\\\b c");
        VERIFY_ARE_EQUAL(args.GetVtOutPipe(), L"");
        VERIFY_ARE_EQUAL(args.GetClientCommandline(), L"d e");
    }
    
}

void ConsoleArgumentsTests::VtPipesTest()
{
    {
        Log::Comment(L"#1 look for a valid commandline");
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
    {
        Log::Comment(L"#1 Check that a simple explicit commandline is found");
        wstring commandline = L"-- foo";
        Log::Comment(commandline.c_str());
        auto args = CreateAndParse(commandline);
        VERIFY_ARE_EQUAL(args.GetClientCommandline(), L"foo");
    }
    {
        Log::Comment(L"#2 Check that a simple implicit commandline is found");
        wstring commandline = L"foo";
        Log::Comment(commandline.c_str());
        auto args = CreateAndParse(commandline);
        VERIFY_ARE_EQUAL(args.GetClientCommandline(), L"foo");
    }
    {
        Log::Comment(L"#3 Check that a implicit commandline with other expected args is treated as a whole client commandline (1)");
        wstring commandline = L"foo -- bar";
        Log::Comment(commandline.c_str());
        auto args = CreateAndParse(commandline);
        VERIFY_ARE_EQUAL(args.GetClientCommandline(), L"foo -- bar");
    }
    {
        Log::Comment(L"#4 Check that a implicit commandline with other expected args is treated as a whole client commandline (2)");
        wstring commandline = L"--inpipe foo foo -- bar";
        Log::Comment(commandline.c_str());
        auto args = CreateAndParse(commandline);
        VERIFY_ARE_EQUAL(args.GetVtInPipe(), L"foo");
        VERIFY_ARE_EQUAL(args.GetClientCommandline(), L"foo -- bar");
    }
    {
        Log::Comment(L"#5 Check that a implicit commandline with other expected args is treated as a whole client commandline (3)");
        wstring commandline = L"console --inpipe foo foo -- bar";
        Log::Comment(commandline.c_str());
        auto args = CreateAndParse(commandline);
        VERIFY_ARE_EQUAL(args.GetVtInPipe(), L"");
        VERIFY_ARE_EQUAL(args.GetClientCommandline(), L"console --inpipe foo foo -- bar");
    }
    {
        Log::Comment(L"#6 Check that a implicit commandline with other expected args is treated as a whole client commandline (4)");
        wstring commandline = L"console --inpipe foo --outpipe foo -- bar";
        Log::Comment(commandline.c_str());
        auto args = CreateAndParse(commandline);
        VERIFY_ARE_EQUAL(args.GetVtInPipe(), L"");
        VERIFY_ARE_EQUAL(args.GetVtOutPipe(), L"");
        VERIFY_IS_FALSE(args.IsUsingVtPipe());
        VERIFY_ARE_EQUAL(args.GetClientCommandline(), L"console --inpipe foo --outpipe foo -- bar");
    }
    {
        Log::Comment(L"#7 Check splitting vt pipes across the explicit commandline does not cause IsUsingVtPipe to be true");
        wstring commandline = L"--inpipe foo -- --outpipe foo bar";
        Log::Comment(commandline.c_str());
        auto args = CreateAndParse(commandline);
        VERIFY_ARE_EQUAL(args.GetVtInPipe(), L"foo");
        VERIFY_ARE_EQUAL(args.GetVtOutPipe(), L"");
        VERIFY_IS_FALSE(args.IsUsingVtPipe());
        VERIFY_ARE_EQUAL(args.GetClientCommandline(), L"--outpipe foo bar");
    }
    {
        Log::Comment(L"#8 Let -- be used as a value of a parameter");
        wstring commandline = L"--inpipe -- --outpipe foo bar";
        Log::Comment(commandline.c_str());
        auto args = CreateAndParse(commandline);
        VERIFY_ARE_EQUAL(args.GetVtInPipe(), L"--");
        VERIFY_ARE_EQUAL(args.GetVtOutPipe(), L"foo");
        VERIFY_IS_TRUE(args.IsUsingVtPipe());
        VERIFY_ARE_EQUAL(args.GetClientCommandline(), L"bar");
    }
    {
        Log::Comment(L"#9 -- by itself does nothing successfully");
        wstring commandline = L"--";
        Log::Comment(commandline.c_str());
        auto args = CreateAndParse(commandline);
        VERIFY_ARE_EQUAL(args.GetClientCommandline(), L"");
    }
    {
        Log::Comment(L"#10 An empty commandline should parse as an empty commandline");
        wstring commandline = L"";
        Log::Comment(commandline.c_str());
        auto args = CreateAndParse(commandline);
        VERIFY_ARE_EQUAL(args.GetClientCommandline(), L"");
    }
}