/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#include "precomp.h"

// This class is intended to test boundary conditions for:
// SetConsoleActiveScreenBuffer
class BufferTests
{
    BEGIN_TEST_CLASS(BufferTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"conhost.exe")
        TEST_CLASS_PROPERTY(L"ArtifactUnderTest", L"wincon.h")
        TEST_CLASS_PROPERTY(L"ArtifactUnderTest", L"conmsgl1.h")
        TEST_CLASS_PROPERTY(L"ArtifactUnderTest", L"conmsgl2.h")
    END_TEST_CLASS()

    TEST_METHOD(TestSetConsoleActiveScreenBufferInvalid);
};

void BufferTests::TestSetConsoleActiveScreenBufferInvalid()
{
    VERIFY_WIN32_BOOL_FAILED(SetConsoleActiveScreenBuffer(INVALID_HANDLE_VALUE));
    VERIFY_WIN32_BOOL_FAILED(SetConsoleActiveScreenBuffer(nullptr));
}
