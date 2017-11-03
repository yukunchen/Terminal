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
public:
    TEST_CLASS(ConsoleArgumentsTests);

    TEST_METHOD(ArgSplittingTests);
    TEST_METHOD(VtPipesTest);
    TEST_METHOD(ClientCommandlineTests);
    TEST_METHOD(LegacyFormatsTests);

    TEST_METHOD(IsUsingVtPipeTests);
    TEST_METHOD(IsUsingVtHandleTests);
    TEST_METHOD(CombineVtPipeHandleTests);
    TEST_METHOD(IsVtHandleValidTests);
};

ConsoleArguments CreateAndParse(std::wstring& commandline, HANDLE hVtIn, HANDLE hVtOut)
{
    ConsoleArguments args = ConsoleArguments(commandline, hVtIn, hVtOut);
    VERIFY_SUCCEEDED(args.ParseCommandline());
    return args;
}

// Used when you expect args to be invalid
ConsoleArguments CreateAndParseUnsuccessfully(std::wstring& commandline, HANDLE hVtIn, HANDLE hVtOut)
{
    ConsoleArguments args = ConsoleArguments(commandline, hVtIn, hVtOut);
    VERIFY_FAILED(args.ParseCommandline());
    return args;
}

void ArgTestsRunner(LPCWSTR comment, std::wstring commandline, HANDLE hVtIn, HANDLE hVtOut, const ConsoleArguments& expected, bool shouldBeSuccessful)
{
    Log::Comment(comment);
    Log::Comment(commandline.c_str());
    const ConsoleArguments actual = shouldBeSuccessful ?
        CreateAndParse(commandline, hVtIn, hVtOut) :
        CreateAndParseUnsuccessfully(commandline, hVtIn, hVtOut);

    VERIFY_ARE_EQUAL(expected, actual);
}

void ConsoleArgumentsTests::ArgSplittingTests()
{
    std::wstring commandline;

    commandline = L"conhost.exe --inpipe foo --outpipe bar this is the commandline";
    ArgTestsRunner(L"#1 look for a valid commandline",
                   commandline,
                   INVALID_HANDLE_VALUE,
                   INVALID_HANDLE_VALUE,
                   ConsoleArguments(commandline,
                                    L"this is the commandline", // clientCommandLine,
                                    INVALID_HANDLE_VALUE,
                                    INVALID_HANDLE_VALUE,
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
                   INVALID_HANDLE_VALUE,
                   INVALID_HANDLE_VALUE,
                   ConsoleArguments(commandline,
                                    L"this is the commandline", // clientCommandLine
                                    INVALID_HANDLE_VALUE,
                                    INVALID_HANDLE_VALUE,
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
                   INVALID_HANDLE_VALUE,
                   INVALID_HANDLE_VALUE,
                   ConsoleArguments(commandline,
                                    L"--outpipe bar this is the commandline", // clientCommandLine
                                    INVALID_HANDLE_VALUE,
                                    INVALID_HANDLE_VALUE,
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
                   INVALID_HANDLE_VALUE,
                   INVALID_HANDLE_VALUE,
                   ConsoleArguments(commandline,
                                    L"this is the commandline", // clientCommandLine
                                    INVALID_HANDLE_VALUE,
                                    INVALID_HANDLE_VALUE,
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
                   INVALID_HANDLE_VALUE,
                   INVALID_HANDLE_VALUE,
                   ConsoleArguments(commandline,
                                    L"this is the commandline", // clientCommandLine
                                    INVALID_HANDLE_VALUE,
                                    INVALID_HANDLE_VALUE,
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
                   INVALID_HANDLE_VALUE,
                   INVALID_HANDLE_VALUE,
                   ConsoleArguments(commandline,
                                    L"--inpipe\\ foo\\ --outpipe\\ bar\\ this\\ is\\ the\\ commandline", // clientCommandLine
                                    INVALID_HANDLE_VALUE, 
                                    INVALID_HANDLE_VALUE,
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
                   INVALID_HANDLE_VALUE, 
                   INVALID_HANDLE_VALUE,
                   ConsoleArguments(commandline,
                                    L"--inpipe\\ foo\\ --outpipe\\ bar\\ this\\ is\\ the\\ commandline", // clientCommandLine
                                    INVALID_HANDLE_VALUE,
                                    INVALID_HANDLE_VALUE,
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
                   INVALID_HANDLE_VALUE,
                   INVALID_HANDLE_VALUE,
                   ConsoleArguments(commandline,
                                    L"d e", // clientCommandLine
                                    INVALID_HANDLE_VALUE,
                                    INVALID_HANDLE_VALUE,
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
                  INVALID_HANDLE_VALUE,
                  INVALID_HANDLE_VALUE,
                  ConsoleArguments(commandline,
                                   L"", // clientCommandLine
                                   INVALID_HANDLE_VALUE,
                                   INVALID_HANDLE_VALUE, 
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
                   INVALID_HANDLE_VALUE,
                   INVALID_HANDLE_VALUE,
                   ConsoleArguments(commandline,
                                    L"bar", // clientCommandLine
                                    INVALID_HANDLE_VALUE,
                                    INVALID_HANDLE_VALUE,
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
                   INVALID_HANDLE_VALUE,
                   INVALID_HANDLE_VALUE,
                   ConsoleArguments(commandline,
                                    L"", // clientCommandLine
                                    INVALID_HANDLE_VALUE,
                                    INVALID_HANDLE_VALUE,
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
                   INVALID_HANDLE_VALUE,
                   INVALID_HANDLE_VALUE,
                   ConsoleArguments(commandline,
                                    L"foo", // clientCommandLine
                                    INVALID_HANDLE_VALUE,
                                    INVALID_HANDLE_VALUE,
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
                   INVALID_HANDLE_VALUE,
                   INVALID_HANDLE_VALUE,
                   ConsoleArguments(commandline,
                                    L"foo", // clientCommandLine
                                    INVALID_HANDLE_VALUE,
                                    INVALID_HANDLE_VALUE,
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
                   INVALID_HANDLE_VALUE,
                   INVALID_HANDLE_VALUE,
                   ConsoleArguments(commandline,
                                    L"", // clientCommandLine
                                    INVALID_HANDLE_VALUE,
                                    INVALID_HANDLE_VALUE,
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
                   INVALID_HANDLE_VALUE,
                   INVALID_HANDLE_VALUE,
                   ConsoleArguments(commandline,
                                    L"--inpipe foo --outpipe bar", // clientCommandLine
                                    INVALID_HANDLE_VALUE,
                                    INVALID_HANDLE_VALUE,
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
                   INVALID_HANDLE_VALUE,
                   INVALID_HANDLE_VALUE,
                   ConsoleArguments(commandline,
                                    L"", // clientCommandLine
                                    INVALID_HANDLE_VALUE,
                                    INVALID_HANDLE_VALUE,
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
                   INVALID_HANDLE_VALUE,
                   INVALID_HANDLE_VALUE,
                   ConsoleArguments(commandline,
                                    L"", // clientCommandLine
                                    INVALID_HANDLE_VALUE,
                                    INVALID_HANDLE_VALUE,
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
                   INVALID_HANDLE_VALUE,
                   INVALID_HANDLE_VALUE,
                   ConsoleArguments(commandline,
                                    L"", // clientCommandLine
                                    INVALID_HANDLE_VALUE,
                                    INVALID_HANDLE_VALUE,
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
                   INVALID_HANDLE_VALUE,
                   INVALID_HANDLE_VALUE,
                   ConsoleArguments(commandline,
                                    L"foo", // clientCommandLine
                                    INVALID_HANDLE_VALUE,
                                    INVALID_HANDLE_VALUE,
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
                   INVALID_HANDLE_VALUE,
                   INVALID_HANDLE_VALUE,
                   ConsoleArguments(commandline,
                                    L"foo", // clientCommandLine
                                    INVALID_HANDLE_VALUE,
                                    INVALID_HANDLE_VALUE,
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
                   INVALID_HANDLE_VALUE,
                   INVALID_HANDLE_VALUE,
                   ConsoleArguments(commandline,
                                    L"foo -- bar", // clientCommandLine
                                    INVALID_HANDLE_VALUE,
                                    INVALID_HANDLE_VALUE,
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
                   INVALID_HANDLE_VALUE,
                   INVALID_HANDLE_VALUE,
                   ConsoleArguments(commandline,
                                    L"foo -- bar", // clientCommandLine
                                    INVALID_HANDLE_VALUE,
                                    INVALID_HANDLE_VALUE,
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
                   INVALID_HANDLE_VALUE,
                   INVALID_HANDLE_VALUE,
                   ConsoleArguments(commandline,
                                    L"console --inpipe foo foo -- bar", // clientCommandLine
                                    INVALID_HANDLE_VALUE,
                                    INVALID_HANDLE_VALUE,
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
                   INVALID_HANDLE_VALUE,
                   INVALID_HANDLE_VALUE,
                   ConsoleArguments(commandline,
                                    L"console --inpipe foo --outpipe foo -- bar", // clientCommandLine
                                    INVALID_HANDLE_VALUE,
                                    INVALID_HANDLE_VALUE,
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
                   INVALID_HANDLE_VALUE,
                   INVALID_HANDLE_VALUE,
                   ConsoleArguments(commandline,
                                    L"--outpipe foo bar", // clientCommandLine
                                    INVALID_HANDLE_VALUE,
                                    INVALID_HANDLE_VALUE,
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
                   INVALID_HANDLE_VALUE,
                   INVALID_HANDLE_VALUE,
                   ConsoleArguments(commandline,
                                    L"bar", // clientCommandLine
                                    INVALID_HANDLE_VALUE,
                                    INVALID_HANDLE_VALUE,
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
                   INVALID_HANDLE_VALUE,
                   INVALID_HANDLE_VALUE,
                   ConsoleArguments(commandline,
                                    L"", // clientCommandLine
                                    INVALID_HANDLE_VALUE,
                                    INVALID_HANDLE_VALUE,
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
                   INVALID_HANDLE_VALUE,
                   INVALID_HANDLE_VALUE,
                   ConsoleArguments(commandline,
                                    L"", // clientCommandLine
                                    INVALID_HANDLE_VALUE,
                                    INVALID_HANDLE_VALUE,
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
                   INVALID_HANDLE_VALUE,
                   INVALID_HANDLE_VALUE,
                   ConsoleArguments(commandline,
                                    L"", // clientCommandLine
                                    INVALID_HANDLE_VALUE,
                                    INVALID_HANDLE_VALUE,
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
                   INVALID_HANDLE_VALUE,
                   INVALID_HANDLE_VALUE,
                   ConsoleArguments(commandline,
                                    L"", // clientCommandLine
                                    INVALID_HANDLE_VALUE,
                                    INVALID_HANDLE_VALUE,
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
                   INVALID_HANDLE_VALUE,
                   INVALID_HANDLE_VALUE,
                   ConsoleArguments(commandline,
                                    L"", // clientCommandLine
                                    INVALID_HANDLE_VALUE,
                                    INVALID_HANDLE_VALUE,
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
                   INVALID_HANDLE_VALUE,
                   INVALID_HANDLE_VALUE,
                   ConsoleArguments(commandline,
                                    L"", // clientCommandLine
                                    INVALID_HANDLE_VALUE,
                                    INVALID_HANDLE_VALUE,
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
                   INVALID_HANDLE_VALUE,
                   INVALID_HANDLE_VALUE,
                   ConsoleArguments(commandline,
                                    L"", // clientCommandLine
                                    INVALID_HANDLE_VALUE,
                                    INVALID_HANDLE_VALUE,
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
                   INVALID_HANDLE_VALUE,
                   INVALID_HANDLE_VALUE,
                   ConsoleArguments(commandline,
                                    L"", // clientCommandLine
                                    INVALID_HANDLE_VALUE,
                                    INVALID_HANDLE_VALUE,
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
                   INVALID_HANDLE_VALUE,
                   INVALID_HANDLE_VALUE,
                   ConsoleArguments(commandline,
                                    L"", // clientCommandLine
                                    INVALID_HANDLE_VALUE,
                                    INVALID_HANDLE_VALUE,
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
                   INVALID_HANDLE_VALUE,
                   INVALID_HANDLE_VALUE,
                   ConsoleArguments(commandline,
                                    L"", // clientCommandLine
                                    INVALID_HANDLE_VALUE,
                                    INVALID_HANDLE_VALUE,
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
    ConsoleArguments args(L"", INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE);
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

void ConsoleArgumentsTests::IsUsingVtHandleTests()
{
    ConsoleArguments args(L"", INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE);
    VERIFY_IS_FALSE(args.HasVtHandles());

    // Just some assorted positive values that could be valid handles. No specific correlation to anything.
    args._vtInHandle = UlongToHandle(0x12);
    VERIFY_IS_FALSE(args.HasVtHandles());

    args._vtOutHandle = UlongToHandle(0x16);
    VERIFY_IS_TRUE(args.HasVtHandles());

    args._vtInHandle = UlongToHandle(0ul);
    VERIFY_IS_FALSE(args.HasVtHandles());

    args._vtInHandle = UlongToHandle(0x20);
    args._vtOutHandle = UlongToHandle(0ul);
    VERIFY_IS_FALSE(args.HasVtHandles());
}

void ConsoleArgumentsTests::CombineVtPipeHandleTests()
{
    std::wstring commandline;

    // Just some assorted positive values that could be valid handles. No specific correlation to anything.
    HANDLE hInSample = UlongToHandle(0x10);
    HANDLE hOutSample = UlongToHandle(0x24);

    commandline = L"conhost.exe";
    ArgTestsRunner(L"#1 Check that handles with no mode is OK",
                   commandline,
                   hInSample,
                   hOutSample,
                   ConsoleArguments(commandline,
                                    L"", // clientCommandLine
                                    hInSample,
                                    hOutSample,
                                    L"", // vtInPipe
                                    L"", // vtOutPipe
                                    L"", // vtMode
                                    false, // forceV1
                                    false, // headless
                                    true, // createServerHandle
                                    0ul), // serverHandle
                   true); // successful parse?

    commandline = L"conhost.exe --vtmode telnet";
    ArgTestsRunner(L"#2 Check that handles with mode is OK",
                   commandline,
                   hInSample,
                   hOutSample,
                   ConsoleArguments(commandline,
                                    L"", // clientCommandLine
                                    hInSample,
                                    hOutSample,
                                    L"", // vtInPipe
                                    L"", // vtOutPipe
                                    L"telnet", // vtMode
                                    false, // forceV1
                                    false, // headless
                                    true, // createServerHandle
                                    0ul), // serverHandle
                   true); // successful parse?

    commandline = L"conhost.exe --inpipe input";
    ArgTestsRunner(L"#3 Check that handles with vt in pipe specified is NOT OK",
                   commandline,
                   hInSample,
                   hOutSample,
                   ConsoleArguments(commandline,
                                    L"", // clientCommandLine
                                    hInSample,
                                    hOutSample,
                                    L"", // vtInPipe
                                    L"", // vtOutPipe
                                    L"", // vtMode
                                    false, // forceV1
                                    false, // headless
                                    true, // createServerHandle
                                    0ul), // serverHandle
                   false); // successful parse?

    commandline = L"conhost.exe --outpipe output";
    ArgTestsRunner(L"#4 Check that handles with vt out pipe specified is NOT OK",
                   commandline,
                   hInSample,
                   hOutSample,
                   ConsoleArguments(commandline,
                                    L"", // clientCommandLine
                                    hInSample,
                                    hOutSample,
                                    L"", // vtInPipe
                                    L"", // vtOutPipe
                                    L"", // vtMode
                                    false, // forceV1
                                    false, // headless
                                    true, // createServerHandle
                                    0ul), // serverHandle
                   false); // successful parse?

    commandline = L"conhost.exe --outpipe output --inpipe input";
    ArgTestsRunner(L"#5 Check that handles with both vt pipes specified is NOT OK",
                   commandline,
                   hInSample,
                   hOutSample,
                   ConsoleArguments(commandline,
                                    L"", // clientCommandLine
                                    hInSample,
                                    hOutSample,
                                    L"", // vtInPipe
                                    L"", // vtOutPipe
                                    L"", // vtMode
                                    false, // forceV1
                                    false, // headless
                                    true, // createServerHandle
                                    0ul), // serverHandle
                   false); // successful parse?
}

void ConsoleArgumentsTests::IsVtHandleValidTests()
{
    // We use both 0 and INVALID_HANDLE_VALUE as invalid handles since we're not sure
    // exactly what will get passed in on the STDIN/STDOUT handles as it can vary wildly
    // depending on who is passing it.
    VERIFY_IS_FALSE(ConsoleArguments::s_IsValidHandle(0), L"Zero handle invalid.");
    VERIFY_IS_FALSE(ConsoleArguments::s_IsValidHandle(INVALID_HANDLE_VALUE), L"Invalid handle invalid.");
    VERIFY_IS_TRUE(ConsoleArguments::s_IsValidHandle(UlongToHandle(0x4)), L"0x4 is valid.");
}
