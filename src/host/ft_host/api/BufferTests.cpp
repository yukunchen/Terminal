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
    TEST_CLASS(BufferTests);

    TEST_METHOD(TestSetConsoleActiveScreenBufferInvalid);
};

void BufferTests::TestSetConsoleActiveScreenBufferInvalid()
{
    VERIFY_WIN32_BOOL_FAILED(SetConsoleActiveScreenBuffer(INVALID_HANDLE_VALUE));
    VERIFY_WIN32_BOOL_FAILED(SetConsoleActiveScreenBuffer(nullptr));
}
