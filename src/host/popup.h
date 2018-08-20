/*++
Copyright (c) Microsoft Corporation

Module Name:
- popup.h

Abstract:
- This file contains the internal structures and definitions used by command line input and editing.

Author:
- Therese Stowell (ThereseS) 15-Nov-1991

Revision History:
- Mike Griese (migrie) Jan 2018:
    Refactored the history and alias functionality into their own files.
- Michael Niksa (miniksa) May 2018:
    Separated out popups from the rest of command line functionality.
--*/

#pragma once

#include "readDataCooked.hpp"
#include "screenInfo.hpp"

#define COPY_TO_CHAR_PROMPT_LENGTH 26
#define COPY_FROM_CHAR_PROMPT_LENGTH 28

#define COMMAND_NUMBER_PROMPT_LENGTH 22

// 5 digit number for command history
#define COMMAND_NUMBER_LENGTH 5
#define MINIMUM_COMMAND_PROMPT_SIZE COMMAND_NUMBER_LENGTH

#define COMMAND_NUMBER_SIZE 8   // size of command number buffer

class CommandHistory;

class Popup final
{
public:
    enum class PopFunc
    {
        None,
        CommandList,
        CopyFromChar,
        CopyToChar,
        CommandNumber
    };

    Popup(SCREEN_INFORMATION& screenInfo, const COORD proposedSize, CommandHistory* const history, const PopFunc func);
    ~Popup();

    void Draw();

    void Update(const SHORT delta, const bool wrap = false);
    void UpdateStoredColors(const WORD newAttr, const WORD newPopupAttr,
                            const WORD oldAttr, const WORD oldPopupAttr);

    void End();

    TextAttribute Attributes;    // text attributes

    SHORT BottomIndex;  // number of command displayed on last line of popup

    // only used during CommandList popup
    SHORT CurrentCommand;

    HRESULT DoCallback(COOKED_READ_DATA& data);

    SHORT Width() const noexcept;
    SHORT Height() const noexcept;

    COORD GetCursorPosition() const noexcept;

    // only used during CommandNumber popup
    void AddNumberToNumberBuffer(const wchar_t wch);
    void DeleteLastFromNumberBuffer() noexcept;
    const std::wstring& CommandNumberInput() const noexcept;
    int ParseCommandNumberInput() const noexcept;

private:

    void _DrawBorder();
    void _DrawPrompt(const UINT id);
    void _DrawList();

    void _UpdateHighlight(const SHORT oldCommand, const SHORT newCommand);

    COORD _CalculateSize(const SCREEN_INFORMATION& screenInfo, const COORD proposedSize);
    COORD _CalculateOrigin(const SCREEN_INFORMATION& screenInfo, const COORD size);

    std::wstring _LoadString(const UINT id);
    static UINT s_LoadStringEx(_In_ HINSTANCE hModule, _In_ UINT wID, _Out_writes_(cchBufferMax) LPWSTR lpBuffer, _In_ UINT cchBufferMax, _In_ WORD wLangId);

    SMALL_RECT _region;  // region popup occupies
    std::vector<CHAR_INFO> _oldContents; // contains data under popup
    COORD _oldScreenSize;

    PopFunc _callbackOption; // routine to call when input is received

    SCREEN_INFORMATION& _screenInfo;
    CommandHistory* const _history;

    // only used during CommandNumber popup
    std::wstring _commandNumberInput;
};
