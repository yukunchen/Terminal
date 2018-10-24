/*++

Copyright (c) Microsoft Corporation

Module Name:
- PopupTestHelper.hpp

Abstract:
- helper functions for unit testing the various popups

Author(s):
- Austin Diviness (AustDi) 06-Sep-2018

--*/


#pragma once

#include "../history.h"
#include "../readDataCooked.hpp"


class PopupTestHelper final
{
public:

    static void InitReadData(COOKED_READ_DATA& cookedReadData,
                             wchar_t* const pBuffer,
                             const size_t cchBuffer,
                             const size_t cursorPosition) noexcept
    {
        cookedReadData._BufferSize = cchBuffer * sizeof(wchar_t);
        cookedReadData._BufPtr = pBuffer + cursorPosition;
        cookedReadData._BackupLimit = pBuffer;
        cookedReadData.OriginalCursorPosition() = { 0, 0 };
        cookedReadData._BytesRead = cursorPosition * sizeof(wchar_t);
        cookedReadData._CurrentPosition = cursorPosition;
        cookedReadData.VisibleCharCount() = cursorPosition;
    }

    static void InitHistory(CommandHistory& history) noexcept
    {
        history.Empty();
        history.Flags |= CLE_ALLOCATED;
        VERIFY_SUCCEEDED(history.Add(L"I'm a little teapot", false));
        VERIFY_SUCCEEDED(history.Add(L"hear me shout", false));
        VERIFY_SUCCEEDED(history.Add(L"here is my handle", false));
        VERIFY_SUCCEEDED(history.Add(L"here is my spout", false));
        VERIFY_ARE_EQUAL(history.GetNumberOfCommands(), 4u);
    }

    static void InitLongHistory(CommandHistory& history) noexcept
    {
        history.Empty();
        history.Flags |= CLE_ALLOCATED;
        VERIFY_SUCCEEDED(history.Add(L"Because I could not stop for Death", false));
        VERIFY_SUCCEEDED(history.Add(L"He kindly stopped for me", false));
        VERIFY_SUCCEEDED(history.Add(L"The carriage held but just Ourselves", false));
        VERIFY_SUCCEEDED(history.Add(L"And Immortality", false));
        VERIFY_SUCCEEDED(history.Add(L"~", false));
        VERIFY_SUCCEEDED(history.Add(L"We slowly drove - He knew no haste", false));
        VERIFY_SUCCEEDED(history.Add(L"And I had put away", false));
        VERIFY_SUCCEEDED(history.Add(L"My labor and my leisure too", false));
        VERIFY_SUCCEEDED(history.Add(L"For His Civility", false));
        VERIFY_SUCCEEDED(history.Add(L"~", false));
        VERIFY_SUCCEEDED(history.Add(L"We passed the School, where Children strove", false));
        VERIFY_SUCCEEDED(history.Add(L"At Recess - in the Ring", false));
        VERIFY_SUCCEEDED(history.Add(L"We passed the Fields of Gazing Grain", false));
        VERIFY_SUCCEEDED(history.Add(L"We passed the Setting Sun", false));
        VERIFY_SUCCEEDED(history.Add(L"~", false));
        VERIFY_ARE_EQUAL(history.GetNumberOfCommands(), 15u);
    }

};
