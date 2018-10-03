/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "clipboard.hpp"

#include "..\..\host\dbcs.h"
#include "..\..\host\scrolling.hpp"
#include "..\..\host\output.h"

#include "..\..\types\inc\convert.hpp"
#include "..\..\types\inc\viewport.hpp"

#include "..\inc\ServiceLocator.hpp"

#pragma hdrstop

using namespace Microsoft::Console::Interactivity::Win32;
using namespace Microsoft::Console::Types;

#pragma region Public Methods

void Clipboard::Copy()
{
    try
    {
        // store selection in clipboard
        StoreSelectionToClipboard();
        Selection::Instance().ClearSelection();   // clear selection in console
    }
    CATCH_LOG();
}

/*++

Perform paste request into old app by pulling out clipboard
contents and writing them to the console's input buffer

--*/
void Clipboard::Paste()
{
    HANDLE ClipboardDataHandle;

    // Clear any selection or scrolling that may be active.
    Selection::Instance().ClearSelection();
    Scrolling::s_ClearScroll();

    // Get paste data from clipboard
    if (!OpenClipboard(ServiceLocator::LocateConsoleWindow()->GetWindowHandle()))
    {
        return;
    }

    ClipboardDataHandle = GetClipboardData(CF_UNICODETEXT);
    if (ClipboardDataHandle == nullptr)
    {
        CloseClipboard();
        return;
    }

    PWCHAR pwstr = (PWCHAR)GlobalLock(ClipboardDataHandle);
    StringPaste(pwstr, (ULONG)GlobalSize(ClipboardDataHandle) / sizeof(WCHAR));
    GlobalUnlock(ClipboardDataHandle);

    CloseClipboard();
}

Clipboard& Clipboard::Instance()
{
    static Clipboard clipboard;
    return clipboard;
}

// Routine Description:
// - This routine pastes given Unicode string into the console window.
// Arguments:
// - pData - Unicode string that is pasted to the console window
// - cchData - Size of the Unicode String in characters
// Return Value:
// - None
void Clipboard::StringPaste(_In_reads_(cchData) const wchar_t* const pData,
                            const size_t cchData)
{
    if (pData == nullptr)
    {
        return;
    }

    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();

    try
    {
        std::deque<std::unique_ptr<IInputEvent>> inEvents = TextToKeyEvents(pData, cchData);
        gci.pInputBuffer->Write(inEvents);
    }
    catch (...)
    {
        LOG_HR(wil::ResultFromCaughtException());
    }
}

#pragma endregion

#pragma region Private Methods

// Routine Description:
// - converts a wchar_t* into a series of KeyEvents as if it was typed
// from the keyboard
// Arguments:
// - pData - the text to convert
// - cchData - the size of pData, in wchars
// Return Value:
// - deque of KeyEvents that represent the string passed in
// Note:
// - will throw exception on error
std::deque<std::unique_ptr<IInputEvent>> Clipboard::TextToKeyEvents(_In_reads_(cchData) const wchar_t* const pData,
                                                                    const size_t cchData)
{
    THROW_IF_NULL_ALLOC(pData);

    std::deque<std::unique_ptr<IInputEvent>> keyEvents;

    for (size_t i = 0; i < cchData; ++i)
    {
        wchar_t currentChar = pData[i];

        const bool charAllowed = FilterCharacterOnPaste(&currentChar);
        // filter out linefeed if it's not the first char and preceded
        // by a carriage return
        const bool skipLinefeed = (i != 0 &&
                                   currentChar == UNICODE_LINEFEED &&
                                   pData[i - 1] == UNICODE_CARRIAGERETURN);

        if (!charAllowed || skipLinefeed)
        {
            continue;
        }

        if (currentChar == 0)
        {
            break;
        }

        // MSFT:12123975 / WSL GH#2006
        // If you paste text with ONLY linefeed line endings (unix style) in wsl,
        //      then we faithfully pass those along, which the underlying terminal
        //      interprets as C-j. In nano, C-j is mapped to "Justify text", which
        //      causes the pasted text to get broken at the width of the terminal.
        // This behavior doesn't occur in gnome-terminal, and nothing like it occurs
        //      in vi or emacs.
        // This change doesn't break pasting text into any of those applications
        //      with CR/LF (Windows) line endings either. That apparently always
        //      worked right.
        if (IsInVirtualTerminalInputMode() && currentChar == UNICODE_LINEFEED)
        {
            currentChar = UNICODE_CARRIAGERETURN;
        }

        const UINT codepage = ServiceLocator::LocateGlobals().getConsoleInformation().OutputCP;
        std::deque<std::unique_ptr<KeyEvent>> convertedEvents = CharToKeyEvents(currentChar, codepage);
        while (!convertedEvents.empty())
        {
            keyEvents.push_back(std::move(convertedEvents.front()));
            convertedEvents.pop_front();
        }
    }
    return keyEvents;
}

// Routine Description:
// - Copies the selected area onto the global system clipboard.
// - NOTE: Throws on allocation and other clipboard failures.
void Clipboard::StoreSelectionToClipboard()
{
    const auto& selection = Selection::Instance();

    // See if there is a selection to get
    if (!selection.IsAreaSelected())
    {
        return;
    }

    // read selection area.
    const auto selectionRects = selection.GetSelectionRects();
    const bool lineSelection = Selection::Instance().IsLineSelection();

    const auto& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    const auto& screenInfo = gci.GetActiveOutputBuffer();

    const auto text = RetrieveTextFromBuffer(screenInfo,
                                             lineSelection,
                                             selectionRects);

    CopyTextToSystemClipboard(text);
}

std::vector<std::wstring> Clipboard::RetrieveTextFromBuffer(const SCREEN_INFORMATION& screenInfo,
                                                            const bool lineSelection,
                                                            const std::vector<SMALL_RECT>& selectionRects)
{
    std::vector<std::wstring> text;

    // for each row in the selection
    for (UINT i = 0; i < selectionRects.size(); i++)
    {
        const SMALL_RECT highlightRow = selectionRects[i];
        const SMALL_RECT selection = Selection::Instance().GetSelectionRectangle();
        const UINT iRow = selection.Top + i;

        // recalculate string length again as the width of the highlight row might have been reduced in the bisect call
        const short stringLength = highlightRow.Right - highlightRow.Left + 1;

        // this is the source location X/Y coordinates within the active screen buffer to start copying from
        const COORD sourcePoint{ highlightRow.Left, highlightRow.Top };

        // our output buffer is 1 dimensional and is just as long as the string, so the "rectangle" should
        // specify just a line.
        // length of 80 runs from left 0 to right 79. therefore -1.
        const SMALL_RECT targetRect{ 0, 0, stringLength - 1, 0 };

        // retrieve the data from the screen buffer
        std::vector<OutputCell> outputCells;
        std::wstring selectionText;
        std::vector<std::vector<OutputCell>> cells = ReadRectFromScreenBuffer(screenInfo,
                                                                              sourcePoint,
                                                                              Viewport::FromInclusive(targetRect));
        // we only care about one row so reduce it here
        outputCells = cells.at(0);
        // allocate a string buffer
        selectionText.reserve(outputCells.size() + 2); // + 2 for \r\n if we munged it

        // copy char data into the string buffer, skipping trailing bytes
        for (auto& cell : outputCells)
        {
            if (!cell.DbcsAttr().IsTrailing())
            {
                for (const wchar_t wch : cell.Chars())
                {
                    selectionText.push_back(wch);
                }
            }
        }

        // trim trailing spaces
        const bool mungeData = (GetKeyState(VK_SHIFT) & KEY_PRESSED) == 0;
        if (mungeData)
        {
            const ROW& Row = screenInfo.GetTextBuffer().GetRowByOffset(iRow);

            // FOR LINE SELECTION ONLY: if the row was wrapped, don't remove the spaces at the end.
            if (!lineSelection || !Row.GetCharRow().WasWrapForced())
            {
                while (!selectionText.empty() && selectionText.back() == UNICODE_SPACE)
                {
                    selectionText.pop_back();
                }
            }

            // apply CR/LF to the end of the final string, unless we're the last line.
            // a.k.a if we're earlier than the bottom, then apply CR/LF.
            if (i < selectionRects.size() - 1)
            {
                // FOR LINE SELECTION ONLY: if the row was wrapped, do not apply CR/LF.
                // a.k.a. if the row was NOT wrapped, then we can assume a CR/LF is proper
                // always apply \r\n for box selection
                if (!lineSelection || !screenInfo.GetTextBuffer().GetRowByOffset(iRow).GetCharRow().WasWrapForced())
                {
                    selectionText.push_back(UNICODE_CARRIAGERETURN);
                    selectionText.push_back(UNICODE_LINEFEED);
                }
            }
        }

        text.emplace_back(selectionText);
    }

    return text;
}


// Routine Description:
// - Copies the text given onto the global system clipboard.
// Arguments:
// - rows - Rows of text data to copy
void Clipboard::CopyTextToSystemClipboard(const std::vector<std::wstring>& rows)
{
    std::wstring finalString;

    // Concatenate strings into one giant string to put onto the clipboard.
    for (const auto& str : rows)
    {
        finalString += str;
    }

    // allocate the final clipboard data
    const size_t cchNeeded = finalString.size() + 1;
    const size_t cbNeeded = sizeof(wchar_t) * cchNeeded;
    wil::unique_hglobal globalHandle(GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, cbNeeded));
    THROW_LAST_ERROR_IF_NULL(globalHandle.get());

    PWSTR pwszClipboard = (PWSTR)GlobalLock(globalHandle.get());
    THROW_LAST_ERROR_IF_NULL(pwszClipboard);

    // The pattern gets a bit strange here because there's no good wil built-in for global lock of this type.
    // Try to copy then immediately unlock. Don't throw until after (so the hglobal won't be freed until we unlock).
    const HRESULT hr = StringCchCopyW(pwszClipboard, cchNeeded, finalString.data());
    GlobalUnlock(globalHandle.get());
    THROW_IF_FAILED(hr);

    // Set global data to clipboard
    THROW_LAST_ERROR_IF(!OpenClipboard(ServiceLocator::LocateConsoleWindow()->GetWindowHandle()));
    THROW_LAST_ERROR_IF(!EmptyClipboard());
    THROW_LAST_ERROR_IF_NULL(SetClipboardData(CF_UNICODETEXT, globalHandle.get()));
    THROW_LAST_ERROR_IF(!CloseClipboard());

    // only free if we failed.
    // the memory has to remain allocated if we successfully placed it on the clipboard.
    // Releasing the smart pointer will leave it allocated as we exit scope.
    globalHandle.release();
}

// Returns true if the character should be emitted to the paste stream
// -- in some cases, we will change what character should be emitted, as in the case of "smart quotes"
// Returns false if the character should not be emitted (e.g. <TAB>)
bool Clipboard::FilterCharacterOnPaste(_Inout_ WCHAR * const pwch)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    bool fAllowChar = true;
    if (gci.GetFilterOnPaste() &&
        (WI_IsFlagSet(gci.pInputBuffer->InputMode, ENABLE_PROCESSED_INPUT)))
    {
        switch (*pwch)
        {
            // swallow tabs to prevent inadvertant tab expansion
        case UNICODE_TAB:
        {
            fAllowChar = false;
            break;
        }

        // Replace Unicode space with standard space
        case UNICODE_NBSP:
        case UNICODE_NARROW_NBSP:
        {
            *pwch = UNICODE_SPACE;
            break;
        }

        // Replace "smart quotes" with "dumb ones"
        case UNICODE_LEFT_SMARTQUOTE:
        case UNICODE_RIGHT_SMARTQUOTE:
        {
            *pwch = UNICODE_QUOTE;
            break;
        }

        // Replace Unicode dashes with a standard hypen
        case UNICODE_EM_DASH:
        case UNICODE_EN_DASH:
        {
            *pwch = UNICODE_HYPHEN;
            break;
        }
        }
    }

    return fAllowChar;
}

#pragma endregion
