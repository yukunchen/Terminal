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

class ConsoleArgumentsTests
{
    TEST_CLASS(ConsoleArgumentsTests);

    TEST_METHOD(ArgSplittingTests);
    TEST_METHOD(VtPipesTest);
    TEST_METHOD(ClientCommandlineTests);
    TEST_METHOD(LegacyFormatsTests);

    TEST_METHOD(IsUsingVtPipeTests);
};

ConsoleArguments CreateAndParse(std::wstring& commandline)
{
    ConsoleArguments args = ConsoleArguments(commandline);
    VERIFY_SUCCEEDED(args.ParseCommandline());
    return args;
}

// Used when you expect args to be invalid
ConsoleArguments CreateAndParseUnsuccessfully(std::wstring& commandline)
{
    ConsoleArguments args = ConsoleArguments(commandline);
    VERIFY_FAILED(args.ParseCommandline());
    return args;
}

void ArgTestsRunner(LPCWSTR comment, std::wstring commandline, const ConsoleArguments& expected, bool shouldBeSuccessful)
{
    Log::Comment(comment);
    Log::Comment(commandline.c_str());
    const ConsoleArguments actual = shouldBeSuccessful ?
        CreateAndParse(commandline) :
        CreateAndParseUnsuccessfully(commandline);

    VERIFY_ARE_EQUAL(expected, actual);
}

void ConsoleArgumentsTests::ArgSplittingTests()
{
    std::wstring commandline;

    commandline = L"conhost.exe --inpipe foo --outpipe bar this is the commandline";
    ArgTestsRunner(L"#1 look for a valid commandline",
                   commandline,
                   ConsoleArguments(commandline,
                                    L"this is the commandline", // clientCommandLine
                                    L"foo", // vtInPipe
                                    L"bar", // vtOutPipe
                                    L"", // vtMode
                                    false, // forceV1
                                    false, // headless
                                    true, // createServerHandle
                                    0), // serverHandle
                   true); // successful parse?

    commandline = L"conhost.exe --inpipe foo --outpipe bar \"this is the commandline\"";
    ArgTestsRunner(L"#2 a commandline with quotes",
                   commandline,
                   ConsoleArguments(commandline,
                                    L"this is the commandline", // clientCommandLine
                                    L"foo", // vtInPipe
                                    L"bar", // vtOutPipe
                                    L"", // vtMode
                                    false, // forceV1
                                    false, // headless
                                    true, // createServerHandle
                                    0), // serverHandle
                   true); // successful parse?

    commandline = L"conhost.exe --inpipe foo \"--outpipe bar this is the commandline\"";
    ArgTestsRunner(L"#3 quotes on an arg",
                   commandline,
                   ConsoleArguments(commandline,
                                    L"--outpipe bar this is the commandline", // clientCommandLine
                                    L"foo", // vtInPipe
                                    L"", // vtOutPipe
                                    L"", // vtMode
                                    false, // forceV1
                                    false, // headless
                                    true, // createServerHandle
                                    0), // serverHandle
                   true); // successful parse?

    commandline = L"conhost.exe --inpipe  foo   --outpipe    bar       this      is the    commandline";
    ArgTestsRunner(L"#4 Many spaces",
                   commandline,
                   ConsoleArguments(commandline,
                                    L"this is the commandline", // clientCommandLine
                                    L"foo", // vtInPipe
                                    L"bar", // vtOutPipe
                                    L"", // vtMode
                                    false, // forceV1
                                    false, // headless
                                    true, // createServerHandle
                                    0), // serverHandle
                   true); // successful parse?

    commandline = L"conhost.exe --inpipe\tfoo\t--outpipe\tbar\tthis\tis\tthe\tcommandline";
    ArgTestsRunner(L"#5\ttab\tdelimit",
                   commandline,
                   ConsoleArguments(commandline,
                                    L"this is the commandline", // clientCommandLine
                                    L"foo", // vtInPipe
                                    L"bar", // vtOutPipe
                                    L"", // vtMode
                                    false, // forceV1
                                    false, // headless
                                    true, // createServerHandle
                                    0), // serverHandle
                   true); // successful parse?

    commandline = L"conhost.exe --inpipe\\ foo\\ --outpipe\\ bar\\ this\\ is\\ the\\ commandline";
    ArgTestsRunner(L"#6 back-slashes won't escape spaces",
                   commandline,
                   ConsoleArguments(commandline,
                                    L"--inpipe\\ foo\\ --outpipe\\ bar\\ this\\ is\\ the\\ commandline", // clientCommandLine
                                    L"", // vtInPipe
                                    L"", // vtOutPipe
                                    L"", // vtMode
                                    false, // forceV1
                                    false, // headless
                                    true, // createServerHandle
                                    0), // serverHandle
                   true); // successful parse?

    commandline = L"conhost.exe --inpipe\\\tfoo\\\t--outpipe\\\tbar\\\tthis\\\tis\\\tthe\\\tcommandline";
    ArgTestsRunner(L"#7 back-slashes won't escape tabs (but the tabs are still converted to spaces)",
                   commandline,
                   ConsoleArguments(commandline,
                                    L"--inpipe\\ foo\\ --outpipe\\ bar\\ this\\ is\\ the\\ commandline", // clientCommandLine
                                    L"", // vtInPipe
                                    L"", // vtOutPipe
                                    L"", // vtMode
                                    false, // forceV1
                                    false, // headless
                                    true, // createServerHandle
                                    0), // serverHandle
                   true); // successful parse?

    commandline = L"conhost.exe --inpipe a\\\\\\\\\"b c\" d e";
    ArgTestsRunner(L"#8 Combo of backslashes and quotes from msdn",
                   commandline,
                   ConsoleArguments(commandline,
                                    L"d e", // clientCommandLine
                                    L"a\\\\b c", // vtInPipe
                                    L"", // vtOutPipe
                                    L"", // vtMode
                                    false, // forceV1
                                    false, // headless
                                    true, // createServerHandle
                                    0), // serverHandle
                   true); // successful parse?
}

void ConsoleArgumentsTests::VtPipesTest()
{
    std::wstring commandline;

    commandline = L"conhost.exe --inpipe foo --outpipe bar";
    ArgTestsRunner(L"#1 look for a valid commandline",
                  commandline,
                  ConsoleArguments(commandline,
                                   L"", // clientCommandLine
                                   L"foo", // vtInPipe
                                   L"bar", // vtOutPipe
                                   L"", // vtMode
                                   false, // forceV1
                                   false, // headless
                                   true, // createServerHandle
                                   0), // serverHandle
                  true); // successful parse?

    commandline = L"conhost.exe --inpipe foo bar";
    ArgTestsRunner(L"#2 In, but no out",
                   commandline,
                   ConsoleArguments(commandline,
                                    L"bar", // clientCommandLine
                                    L"foo", // vtInPipe
                                    L"", // vtOutPipe
                                    L"", // vtMode
                                    false, // forceV1
                                    false, // headless
                                    true, // createServerHandle
                                    0), // serverHandle
                   true); // successful parse?

    commandline = L"conhost.exe --outpipe foo";
    ArgTestsRunner(L"#3 Out, no in",
                   commandline,
                   ConsoleArguments(commandline,
                                    L"", // clientCommandLine
                                    L"", // vtInPipe
                                    L"foo", // vtOutPipe
                                    L"", // vtMode
                                    false, // forceV1
                                    false, // headless
                                    true, // createServerHandle
                                    0), // serverHandle
                   true); // successful parse?

    commandline = L"conhost.exe foo";
    ArgTestsRunner(L"#4 Neither",
                   commandline,
                   ConsoleArguments(commandline,
                                    L"foo", // clientCommandLine
                                    L"", // vtInPipe
                                    L"", // vtOutPipe
                                    L"", // vtMode
                                    false, // forceV1
                                    false, // headless
                                    true, // createServerHandle
                                    0), // serverHandle
                   true); // successful parse?

    commandline = L"conhost.exe --inpipe --outpipe foo";
    ArgTestsRunner(L"#5 Mixed (1)",
                   commandline,
                   ConsoleArguments(commandline,
                                    L"foo", // clientCommandLine
                                    L"--outpipe", // vtInPipe
                                    L"", // vtOutPipe
                                    L"", // vtMode
                                    false, // forceV1
                                    false, // headless
                                    true, // createServerHandle
                                    0), // serverHandle
                   true); // successful parse?

    commandline = L"conhost.exe --inpipe --outpipe --outpipe foo";
    ArgTestsRunner(L"#6 Mixed (2)",
                   commandline,
                   ConsoleArguments(commandline,
                                    L"", // clientCommandLine
                                    L"--outpipe", // vtInPipe
                                    L"foo", // vtOutPipe
                                    L"", // vtMode
                                    false, // forceV1
                                    false, // headless
                                    true, // createServerHandle
                                    0), // serverHandle
                   true); // successful parse?

    commandline = L"conhost.exe -- --inpipe foo --outpipe bar";
    ArgTestsRunner(L"#7 Pipe names in client commandline",
                   commandline,
                   ConsoleArguments(commandline,
                                    L"--inpipe foo --outpipe bar", // clientCommandLine
                                    L"", // vtInPipe
                                    L"", // vtOutPipe
                                    L"", // vtMode
                                    false, // forceV1
                                    false, // headless
                                    true, // createServerHandle
                                    0), // serverHandle
                   true); // successful parse?

    commandline = L"conhost.exe --inpipe foo --outpipe foo";
    ArgTestsRunner(L"#8 Pipe names the same",
                   commandline,
                   ConsoleArguments(commandline,
                                    L"", // clientCommandLine
                                    L"foo", // vtInPipe
                                    L"foo", // vtOutPipe
                                    L"", // vtMode
                                    false, // forceV1
                                    false, // headless
                                    true, // createServerHandle
                                    0), // serverHandle
                   true); // successful parse?

    commandline = L"conhost.exe --inpipe foo --outpipe";
    ArgTestsRunner(L"#9 Not enough args (1)",
                   commandline,
                   ConsoleArguments(commandline,
                                    L"", // clientCommandLine
                                    L"foo", // vtInPipe
                                    L"", // vtOutPipe
                                    L"", // vtMode
                                    false, // forceV1
                                    false, // headless
                                    true, // createServerHandle
                                    0), // serverHandle
                   false); // successful parse?
    
    commandline = L"conhost.exe --inpipe";
    ArgTestsRunner(L"#10 Not enough args (2)",
                   commandline,
                   ConsoleArguments(commandline,
                                    L"", // clientCommandLine
                                    L"", // vtInPipe
                                    L"", // vtOutPipe
                                    L"", // vtMode
                                    false, // forceV1
                                    false, // headless
                                    true, // createServerHandle
                                    0), // serverHandle
                   false); // successful parse?
}

void ConsoleArgumentsTests::ClientCommandlineTests()
{
    std::wstring commandline;

    commandline = L"conhost.exe -- foo";
    ArgTestsRunner(L"#1 Check that a simple explicit commandline is found",
                   commandline,
                   ConsoleArguments(commandline,
                                    L"foo", // clientCommandLine
                                    L"", // vtInPipe
                                    L"", // vtOutPipe
                                    L"", // vtMode
                                    false, // forceV1
                                    false, // headless
                                    true, // createServerHandle
                                    0), // serverHandle
                   true); // successful parse?

    commandline = L"conhost.exe foo";
    ArgTestsRunner(L"#2 Check that a simple implicit commandline is found",
                   commandline,
                   ConsoleArguments(commandline,
                                    L"foo", // clientCommandLine
                                    L"", // vtInPipe
                                    L"", // vtOutPipe
                                    L"", // vtMode
                                    false, // forceV1
                                    false, // headless
                                    true, // createServerHandle
                                    0), // serverHandle
                   true); // successful parse?

    commandline = L"conhost.exe foo -- bar";
    ArgTestsRunner(L"#3 Check that a implicit commandline with other expected args is treated as a whole client commandline (1)",
                   commandline,
                   ConsoleArguments(commandline,
                                    L"foo -- bar", // clientCommandLine
                                    L"", // vtInPipe
                                    L"", // vtOutPipe
                                    L"", // vtMode
                                    false, // forceV1
                                    false, // headless
                                    true, // createServerHandle
                                    0), // serverHandle
                   true); // successful parse?

    commandline = L"conhost.exe --inpipe foo foo -- bar";
    ArgTestsRunner(L"#4 Check that a implicit commandline with other expected args is treated as a whole client commandline (2)",
                   commandline,
                   ConsoleArguments(commandline,
                                    L"foo -- bar", // clientCommandLine
                                    L"foo", // vtInPipe
                                    L"", // vtOutPipe
                                    L"", // vtMode
                                    false, // forceV1
                                    false, // headless
                                    true, // createServerHandle
                                    0), // serverHandle
                   true); // successful parse?

    commandline = L"conhost.exe console --inpipe foo foo -- bar";
    ArgTestsRunner(L"#5 Check that a implicit commandline with other expected args is treated as a whole client commandline (3)",
                   commandline,
                   ConsoleArguments(commandline,
                                    L"console --inpipe foo foo -- bar", // clientCommandLine
                                    L"", // vtInPipe
                                    L"", // vtOutPipe
                                    L"", // vtMode
                                    false, // forceV1
                                    false, // headless
                                    true, // createServerHandle
                                    0), // serverHandle
                   true); // successful parse?
   
    commandline = L"conhost.exe console --inpipe foo --outpipe foo -- bar";
    ArgTestsRunner(L"#6 Check that a implicit commandline with other expected args is treated as a whole client commandline (4)",
                   commandline,
                   ConsoleArguments(commandline,
                                    L"console --inpipe foo --outpipe foo -- bar", // clientCommandLine
                                    L"", // vtInPipe
                                    L"", // vtOutPipe
                                    L"", // vtMode
                                    false, // forceV1
                                    false, // headless
                                    true, // createServerHandle
                                    0), // serverHandle
                   true); // successful parse?

    commandline = L"conhost.exe --inpipe foo -- --outpipe foo bar";
    ArgTestsRunner(L"#7 Check splitting vt pipes across the explicit commandline does not pull both pipe names out",
                   commandline,
                   ConsoleArguments(commandline,
                                    L"--outpipe foo bar", // clientCommandLine
                                    L"foo", // vtInPipe
                                    L"", // vtOutPipe
                                    L"", // vtMode
                                    false, // forceV1
                                    false, // headless
                                    true, // createServerHandle
                                    0), // serverHandle
                   true); // successful parse?

    commandline = L"conhost.exe --inpipe -- --outpipe foo bar";
    ArgTestsRunner(L"#8 Let -- be used as a value of a parameter",
                   commandline,
                   ConsoleArguments(commandline,
                                    L"bar", // clientCommandLine
                                    L"--", // vtInPipe
                                    L"foo", // vtOutPipe
                                    L"", // vtMode
                                    false, // forceV1
                                    false, // headless
                                    true, // createServerHandle
                                    0), // serverHandle
                   true); // successful parse?

    commandline = L"conhost.exe --";
    ArgTestsRunner(L"#9 -- by itself does nothing successfully",
                   commandline,
                   ConsoleArguments(commandline,
                                    L"", // clientCommandLine
                                    L"", // vtInPipe
                                    L"", // vtOutPipe
                                    L"", // vtMode
                                    false, // forceV1
                                    false, // headless
                                    true, // createServerHandle
                                    0), // serverHandle
                   true); // successful parse?

    commandline = L"conhost.exe";
    ArgTestsRunner(L"#10 An empty commandline should parse as an empty commandline",
                   commandline,
                   ConsoleArguments(commandline,
                                    L"", // clientCommandLine
                                    L"", // vtInPipe
                                    L"", // vtOutPipe
                                    L"", // vtMode
                                    false, // forceV1
                                    false, // headless
                                    true, // createServerHandle
                                    0), // serverHandle
                   true); // successful parse?
}

void ConsoleArgumentsTests::LegacyFormatsTests()
{
    std::wstring commandline;

    commandline = L"conhost.exe 0x4";
    ArgTestsRunner(L"#1 Check that legacy launch mechanisms via the system loader with a server handle ID work",
                   commandline,
                   ConsoleArguments(commandline,
                                    L"", // clientCommandLine
                                    L"", // vtInPipe
                                    L"", // vtOutPipe
                                    L"", // vtMode
                                    false, // forceV1
                                    false, // headless
                                    false, // createServerHandle
                                    4ul), // serverHandle
                   true); // successful parse?

    commandline = L"conhost.exe --server 0x4";
    ArgTestsRunner(L"#2 Check that launch mechanism with parameterized server handle ID works",
                   commandline,
                   ConsoleArguments(commandline,
                                    L"", // clientCommandLine
                                    L"", // vtInPipe
                                    L"", // vtOutPipe
                                    L"", // vtMode
                                    false, // forceV1
                                    false, // headless
                                    false, // createServerHandle
                                    4ul), // serverHandle
                   true); // successful parse?

    commandline = L"conhost.exe 0x4 0x8";
    ArgTestsRunner(L"#3 Check that two handle IDs fails (1)",
                   commandline,
                   ConsoleArguments(commandline,
                                    L"", // clientCommandLine
                                    L"", // vtInPipe
                                    L"", // vtOutPipe
                                    L"", // vtMode
                                    false, // forceV1
                                    false, // headless
                                    false, // createServerHandle
                                    4ul), // serverHandle
                   false); // successful parse?

    commandline = L"conhost.exe --server 0x4 0x8";
    ArgTestsRunner(L"#4 Check that two handle IDs fails (2)",
                   commandline,
                   ConsoleArguments(commandline,
                                    L"", // clientCommandLine
                                    L"", // vtInPipe
                                    L"", // vtOutPipe
                                    L"", // vtMode
                                    false, // forceV1
                                    false, // headless
                                    false, // createServerHandle
                                    4ul), // serverHandle
                   false); // successful parse?

    commandline = L"conhost.exe 0x4 --server 0x8";
    ArgTestsRunner(L"#5 Check that two handle IDs fails (3)",
                   commandline,
                   ConsoleArguments(commandline,
                                    L"", // clientCommandLine
                                    L"", // vtInPipe
                                    L"", // vtOutPipe
                                    L"", // vtMode
                                    false, // forceV1
                                    false, // headless
                                    false, // createServerHandle
                                    4ul), // serverHandle
                   false); // successful parse?

    commandline = L"conhost.exe --server 0x4 --server 0x8";
    ArgTestsRunner(L"#6 Check that two handle IDs fails (4)",
                   commandline,
                   ConsoleArguments(commandline,
                                    L"", // clientCommandLine
                                    L"", // vtInPipe
                                    L"", // vtOutPipe
                                    L"", // vtMode
                                    false, // forceV1
                                    false, // headless
                                    false, // createServerHandle
                                    4ul), // serverHandle
                   false); // successful parse?

    commandline = L"conhost.exe 0x4 -ForceV1";
    ArgTestsRunner(L"#7 Check that ConDrv handle + -ForceV1 succeeds",
                   commandline,
                   ConsoleArguments(commandline,
                                    L"", // clientCommandLine
                                    L"", // vtInPipe
                                    L"", // vtOutPipe
                                    L"", // vtMode
                                    true, // forceV1
                                    false, // headless
                                    false, // createServerHandle
                                    4ul), // serverHandle
                   true); // successful parse?

    commandline = L"conhost.exe -ForceV1";
    ArgTestsRunner(L"#8 Check that -ForceV1 parses on its own",
                   commandline,
                   ConsoleArguments(commandline,
                                    L"", // clientCommandLine
                                    L"", // vtInPipe
                                    L"", // vtOutPipe
                                    L"", // vtMode
                                    true, // forceV1
                                    false, // headless
                                    true, // createServerHandle
                                    0), // serverHandle
                   true); // successful parse?
}

void ConsoleArgumentsTests::IsUsingVtPipeTests()
{
    ConsoleArguments args(L"");
    VERIFY_IS_FALSE(args.IsUsingVtPipe());

    args._vtInPipe = L"foo";
    VERIFY_IS_FALSE(args.IsUsingVtPipe());

    args._vtOutPipe = L"bar";
    VERIFY_IS_TRUE(args.IsUsingVtPipe());

    args._vtInPipe = L"";
    VERIFY_IS_FALSE(args.IsUsingVtPipe());

    args._vtInPipe = args._vtOutPipe;
    args._vtOutPipe = L"";
    VERIFY_IS_FALSE(args.IsUsingVtPipe());
}
