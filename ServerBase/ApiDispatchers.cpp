#include "stdafx.h"
#include "ApiDispatchers.h"

HANDLE h = INVALID_HANDLE_VALUE; // look up handle again based on what's in message

NTSTATUS ApiDispatchers::ServeGetConsoleCP(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_GETCP_MSG* const a = &m->u.consoleMsgL1.GetConsoleCP;

    DWORD Result = 0;

    if (a->Output)
    {
        Result = pResponders->GetConsoleOutputCodePageImpl(h, &a->CodePage);
    }
    else
    {
        Result = pResponders->GetConsoleInputCodePageImpl(h, &a->CodePage);
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeGetConsoleMode(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_MODE_MSG* const a = &m->u.consoleMsgL1.GetConsoleMode;

    bool IsOutputMode = true; // this needs to check the handle type in the handle data to determine which one to operate on. I haven't done handle extraction yet.

    DWORD Result = 0;

    if (IsOutputMode)
    {
        Result = pResponders->GetConsoleOutputModeImpl(h, &a->Mode);
    }
    else
    {
        Result = pResponders->GetConsoleInputModeImpl(h, &a->Mode);
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeSetConsoleMode(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_MODE_MSG* const a = &m->u.consoleMsgL1.SetConsoleMode;

    bool IsOutputMode = true; // this needs to check the handle type in the handle data to determine which one to operate on. I haven't done handle extraction yet.

    DWORD Result = 0;

    if (IsOutputMode)
    {
        Result = pResponders->SetConsoleOutputModeImpl(h, a->Mode);
    }
    else
    {
        Result = pResponders->SetConsoleInputModeImpl(h, a->Mode);
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeGetNumberOfInputEvents(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_GETNUMBEROFINPUTEVENTS_MSG* const a = &m->u.consoleMsgL1.GetNumberOfConsoleInputEvents;

    return pResponders->GetNumberOfConsoleInputEventsImpl(h, &a->ReadyEvents);
}

NTSTATUS ApiDispatchers::ServeGetConsoleInput(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_GETCONSOLEINPUT_MSG* const a = &m->u.consoleMsgL1.GetConsoleInput;

    DWORD Result = 0;
    DWORD Written = 0;

    if (a->Unicode)
    {
        Result = pResponders->ReadConsoleInputWImpl(h, nullptr, 0, &Written);
    }
    else
    {
        Result = pResponders->ReadConsoleInputAImpl(h, nullptr, 0, &Written);
    }

    a->NumRecords = SUCCEEDED(Result) ? Written : 0;

    return Result;
}

NTSTATUS ApiDispatchers::ServeReadConsole(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_READCONSOLE_MSG* const a = &m->u.consoleMsgL1.ReadConsoleW;

    DWORD Result = 0;
    DWORD Read = 0;

    CONSOLE_READCONSOLE_CONTROL Control;
    Control.dwControlKeyState = a->ControlKeyState;
    Control.dwCtrlWakeupMask = a->CtrlWakeupMask;
    Control.nInitialChars = a->InitialNumBytes;
    Control.nLength = a->NumBytes;

    if (a->Unicode)
    {
        Result = pResponders->ReadConsoleWImpl(h, nullptr, 0, &Read, &Control);
    }
    else
    {
        Result = pResponders->ReadConsoleAImpl(h, nullptr, 0, &Read, &Control);
    }

    //a->NumBytes = SUCCEEDED(Result) ? Read : 0;

    //return Result;

	return ERROR_IO_PENDING;
}

NTSTATUS ApiDispatchers::ServeWriteConsole(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_WRITECONSOLE_MSG* const a = &m->u.consoleMsgL1.WriteConsoleW;

    DWORD Result = 0;
    DWORD Written = 0;

	// ensure we have the input buffer
	if (m->State.InputBuffer == nullptr)
	{
		Result = STATUS_INVALID_PARAMETER;
	}

	if (SUCCEEDED(Result))
	{
		void* const Buffer = m->State.InputBuffer;
		ULONG BufferSize = m->State.InputBufferSize;

		if (a->Unicode)
		{
			Result = pResponders->WriteConsoleWImpl(h, (WCHAR*)Buffer, BufferSize, &Written);
		}
		else
		{
			Result = pResponders->WriteConsoleAImpl(h, nullptr, 0, &Written);
		}

		delete[] Buffer;
	}

    a->NumBytes = SUCCEEDED(Result) ? Written : 0;

    return Result;
}

NTSTATUS ApiDispatchers::ServeGetConsoleLangId(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_LANGID_MSG* const a = &m->u.consoleMsgL1.GetConsoleLangId;

    // TODO: Consider replacing with just calling the Output CP API and converting to LangID here.
    return pResponders->GetConsoleLangId(h, &a->LangId);
}

NTSTATUS ApiDispatchers::ServeFillConsoleOutput(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_FILLCONSOLEOUTPUT_MSG* const a = &m->u.consoleMsgL2.FillConsoleOutput;

    DWORD Result = 0;
    DWORD ElementsWritten = 0;

    switch (a->ElementType)
    {
    case CONSOLE_ATTRIBUTE:
    {
        Result = pResponders->FillConsoleOutputAttributeImpl(h, a->Element, a->Length, a->WriteCoord, &ElementsWritten);
        break;
    }
    case CONSOLE_ASCII:
    {
        Result = pResponders->FillConsoleOutputCharacterAImpl(h, (char)a->Element, a->Length, a->WriteCoord, &ElementsWritten);
        break;
    }
    case CONSOLE_REAL_UNICODE:
    case CONSOLE_FALSE_UNICODE:
    {
        Result = pResponders->FillConsoleOutputCharacterWImpl(h, a->Element, a->Length, a->WriteCoord, &ElementsWritten);
        break;
    }
    default:
    {
        Result = STATUS_UNSUCCESSFUL;
        break;
    }
    }

    a->Length = SUCCEEDED(Result) ? ElementsWritten : 0;

    return Result;
}

NTSTATUS ApiDispatchers::ServeGenerateConsoleCtrlEvent(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_CTRLEVENT_MSG* const a = &m->u.consoleMsgL2.GenerateConsoleCtrlEvent;

    return pResponders->GenerateConsoleCtrlEventImpl(a->ProcessGroupId, a->CtrlEvent);
}

NTSTATUS ApiDispatchers::ServeSetConsoleActiveScreenBuffer(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    return pResponders->SetConsoleActiveScreenBufferImpl(h);
}

NTSTATUS ApiDispatchers::ServeFlushConsoleInputBuffer(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    return pResponders->FlushConsoleInputBuffer(h);
}

NTSTATUS ApiDispatchers::ServeSetConsoleCP(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_SETCP_MSG* const a = &m->u.consoleMsgL2.SetConsoleCP;

    DWORD Result = 0;

    if (a->Output)
    {
        Result = pResponders->SetConsoleOutputCodePageImpl(h, a->CodePage);
    }
    else
    {
        Result = pResponders->SetConsoleInputCodePageImpl(h, a->CodePage);
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeGetConsoleCursorInfo(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_GETCURSORINFO_MSG* const a = &m->u.consoleMsgL2.GetConsoleCursorInfo;

    return pResponders->GetConsoleCursorInfoImpl(h, &a->CursorSize, &a->Visible);
}

NTSTATUS ApiDispatchers::ServeSetConsoleCursorInfo(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_SETCURSORINFO_MSG* const a = &m->u.consoleMsgL2.SetConsoleCursorInfo;

    return pResponders->SetConsoleCursorInfoImpl(h, a->CursorSize, a->Visible);
}

NTSTATUS ApiDispatchers::ServeGetConsoleScreenBufferInfo(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_SCREENBUFFERINFO_MSG* const a = &m->u.consoleMsgL2.GetConsoleScreenBufferInfo;

    CONSOLE_SCREEN_BUFFER_INFOEX ex = { 0 };
    ex.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);
        
    DWORD Result = pResponders->GetConsoleScreenBufferInfoExImpl(h, &ex);

    if (SUCCEEDED(Result))
    {
        a->FullscreenSupported = ex.bFullscreenSupported;
        size_t const ColorTableSizeInBytes = RTL_NUMBER_OF_V2(ex.ColorTable) * sizeof(*ex.ColorTable);
        CopyMemory(a->ColorTable, ex.ColorTable, ColorTableSizeInBytes);
        a->CursorPosition = ex.dwCursorPosition;
        a->MaximumWindowSize = ex.dwMaximumWindowSize;
        a->Size = ex.dwSize;
        a->ScrollPosition.X = ex.srWindow.Left;
        a->ScrollPosition.Y = ex.srWindow.Top;
        a->CurrentWindowSize.X = ex.srWindow.Right - ex.srWindow.Left;
        a->CurrentWindowSize.Y = ex.srWindow.Bottom - ex.srWindow.Top;
        a->Attributes = ex.wAttributes;
        a->PopupAttributes = ex.wPopupAttributes;
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeSetConsoleScreenBufferInfo(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_SCREENBUFFERINFO_MSG* const a = &m->u.consoleMsgL2.SetConsoleScreenBufferInfo;

    CONSOLE_SCREEN_BUFFER_INFOEX ex = { 0 };
    ex.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);
    ex.bFullscreenSupported = a->FullscreenSupported;
    size_t const ColorTableSizeInBytes = RTL_NUMBER_OF_V2(ex.ColorTable) * sizeof(*ex.ColorTable);
    CopyMemory(ex.ColorTable, a->ColorTable, ColorTableSizeInBytes);
    ex.dwCursorPosition = a->CursorPosition;
    ex.dwMaximumWindowSize = a->MaximumWindowSize;
    ex.dwSize = a->Size;
    ex.srWindow = { 0 };
    ex.srWindow.Left = a->ScrollPosition.X;
    ex.srWindow.Top = a->ScrollPosition.Y;
    ex.srWindow.Right = ex.srWindow.Left + a->CurrentWindowSize.X;
    ex.srWindow.Bottom = ex.srWindow.Top + a->CurrentWindowSize.Y;
    ex.wAttributes = a->Attributes;
    ex.wPopupAttributes = a->PopupAttributes;

    return pResponders->SetConsoleScreenBufferInfoExImpl(h, &ex);
}

NTSTATUS ApiDispatchers::ServeSetConsoleScreenBufferSize(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_SETSCREENBUFFERSIZE_MSG* const a = &m->u.consoleMsgL2.SetConsoleScreenBufferSize;

    return pResponders->SetConsoleScreenBufferSizeImpl(h, &a->Size);
}

NTSTATUS ApiDispatchers::ServeSetConsoleCursorPosition(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_SETCURSORPOSITION_MSG* const a = &m->u.consoleMsgL2.SetConsoleCursorPosition;

    return pResponders->SetConsoleCursorPositionImpl(h, &a->CursorPosition);
}

NTSTATUS ApiDispatchers::ServeGetLargestConsoleWindowSize(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_GETLARGESTWINDOWSIZE_MSG* const a = &m->u.consoleMsgL2.GetLargestConsoleWindowSize;

    return pResponders->GetLargestConsoleWindowSizeImpl(h, &a->Size);
}

NTSTATUS ApiDispatchers::ServeScrollConsoleScreenBuffer(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_SCROLLSCREENBUFFER_MSG* const a = &m->u.consoleMsgL2.ScrollConsoleScreenBufferW;

    DWORD Result = 0;

    SMALL_RECT* pClipRectangle = a->Clip ? &a->ClipRectangle : nullptr;

    if (a->Unicode)
    {
        Result = pResponders->ScrollConsoleScreenBufferWImpl(h, &a->ScrollRectangle, &a->DestinationOrigin, pClipRectangle, &a->Fill);
    }
    else
    {
        Result = pResponders->ScrollConsoleScreenBufferAImpl(h, &a->ScrollRectangle, &a->DestinationOrigin, pClipRectangle, &a->Fill);
    }
    
    return Result;
}

NTSTATUS ApiDispatchers::ServeSetConsoleTextAttribute(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_SETTEXTATTRIBUTE_MSG* const a = &m->u.consoleMsgL2.SetConsoleTextAttribute;

    return pResponders->SetConsoleTextAttributeImpl(h, a->Attributes);
}

NTSTATUS ApiDispatchers::ServeSetConsoleWindowInfo(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_SETWINDOWINFO_MSG* const a = &m->u.consoleMsgL2.SetConsoleWindowInfo;
    
    return pResponders->SetConsoleWindowInfoImpl(h, a->Absolute, &a->Window);
}

NTSTATUS ApiDispatchers::ServeReadConsoleOutputString(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_READCONSOLEOUTPUTSTRING_MSG* const a = &m->u.consoleMsgL2.ReadConsoleOutputString;

    DWORD Result = 0;

    switch (a->StringType)
    {
    case CONSOLE_ATTRIBUTE:
    {
        Result = pResponders->ReadConsoleOutputAttributeImpl(h, &a->ReadCoord, nullptr, 0, &a->NumRecords);
        break;
    }
    case CONSOLE_ASCII:
    {
        Result = pResponders->ReadConsoleOutputCharacterAImpl(h, &a->ReadCoord, nullptr, 0, &a->NumRecords);
        break;
    }
    case CONSOLE_REAL_UNICODE:
    case CONSOLE_FALSE_UNICODE:
    {
        Result = pResponders->ReadConsoleOutputCharacterWImpl(h, &a->ReadCoord, nullptr, 0, &a->NumRecords);
        break;
    }
    default:
    {
        Result = STATUS_UNSUCCESSFUL;
        break;
    }
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeWriteConsoleInput(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_WRITECONSOLEINPUT_MSG* const a = &m->u.consoleMsgL2.WriteConsoleInputW;

    DWORD Result = 0;
    DWORD Read = 0;

    if (a->Unicode)
    {
        Result = pResponders->WriteConsoleInputWImpl(h, nullptr, 0, &Read);
    }
    else
    {
        Result = pResponders->WriteConsoleInputAImpl(h, nullptr, 0, &Read);
    }

    a->NumRecords = SUCCEEDED(Result) ? Read : 0;

    return Result;
}

NTSTATUS ApiDispatchers::ServeWriteConsoleOutput(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_WRITECONSOLEOUTPUT_MSG* const a = &m->u.consoleMsgL2.WriteConsoleOutputW;

    DWORD Result = 0;

    COORD TextBufferSize = { 0 };
    COORD TextBufferSourceOrigin = { 0 };
    SMALL_RECT TargetRectangle = { 0 };
    SMALL_RECT AffectedRectangle;

    if (a->Unicode)
    {
        Result = pResponders->WriteConsoleOutputWImpl(h, nullptr, &TextBufferSize, &TextBufferSourceOrigin, &TargetRectangle, &AffectedRectangle);
    }
    else
    {
        Result = pResponders->WriteConsoleOutputAImpl(h, nullptr, &TextBufferSize, &TextBufferSourceOrigin, &TargetRectangle, &AffectedRectangle);
    }

    a->CharRegion = { 0, 0, -1, -1 };
    if (SUCCEEDED(Result))
    {
        a->CharRegion = AffectedRectangle;
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeWriteConsoleOutputString(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_WRITECONSOLEOUTPUTSTRING_MSG* const a = &m->u.consoleMsgL2.WriteConsoleOutputString;

    DWORD Result = 0;

    switch (a->StringType)
    {
    case CONSOLE_ATTRIBUTE:
    {
        Result = pResponders->WriteConsoleOutputAttributeImpl(h, nullptr, 0, &a->WriteCoord, &a->NumRecords);
        break;
    }
    case CONSOLE_ASCII:
    {
        Result = pResponders->WriteConsoleOutputCharacterAImpl(h, nullptr, 0, &a->WriteCoord, &a->NumRecords);
        break;
    }
    case CONSOLE_REAL_UNICODE:
    case CONSOLE_FALSE_UNICODE:
    {
        Result = pResponders->WriteConsoleOutputCharacterWImpl(h, nullptr, 0, &a->WriteCoord, &a->NumRecords);
        break;
    }
    default:
    {
        Result = STATUS_UNSUCCESSFUL;
        break;
    }
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeReadConsoleOutput(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_READCONSOLEOUTPUT_MSG* const a = &m->u.consoleMsgL2.ReadConsoleOutputW;

    DWORD Result = 0;

    COORD TextBufferSize = { 0 };
    COORD TextBufferTargetOrigin = { 0 };
    SMALL_RECT SourceRectangle = { 0 };
    SMALL_RECT ReadRectangle;

    if (a->Unicode)
    {
        Result = pResponders->ReadConsoleOutputW(h, nullptr, &TextBufferSize, &TextBufferTargetOrigin, &SourceRectangle, &ReadRectangle);
    }
    else
    {
        Result = pResponders->ReadConsoleOutputA(h, nullptr, &TextBufferSize, &TextBufferTargetOrigin, &SourceRectangle, &ReadRectangle);
    }

    a->CharRegion = { 0, 0, -1, -1 };
    if (SUCCEEDED(Result))
    {
        a->CharRegion = ReadRectangle;
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeGetConsoleTitle(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_GETTITLE_MSG* const a = &m->u.consoleMsgL2.GetConsoleTitleW;

    DWORD Result = 0;

    if (a->Unicode)
    {
        if (a->Original)
        {
            Result = pResponders->GetConsoleOriginalTitleWImpl(h, nullptr, 0);
        }
        else
        {
            Result = pResponders->GetConsoleTitleWImpl(h, nullptr, 0);
        }
    }
    else
    {
        if (a->Original)
        {
            Result = pResponders->GetConsoleOriginalTitleAImpl(h, nullptr, 0);
        }
        else
        {
            Result = pResponders->GetConsoleTitleAImpl(h, nullptr, 0);
        }
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeSetConsoleTitle(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_SETTITLE_MSG* const a = &m->u.consoleMsgL2.SetConsoleTitleW;

    DWORD Result = 0;

    if (a->Unicode)
    {
        Result = pResponders->SetConsoleTitleWImpl(h, nullptr, 0);
    }
    else
    {
        Result = pResponders->SetConsoleTitleAImpl(h, nullptr, 0);
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeGetConsoleMouseInfo(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_GETMOUSEINFO_MSG* const a = &m->u.consoleMsgL3.GetConsoleMouseInfo;

    return pResponders->GetNumberOfConsoleMouseButtonsImpl(&a->NumButtons);
}

NTSTATUS ApiDispatchers::ServeGetConsoleFontSize(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_GETFONTSIZE_MSG* const a = &m->u.consoleMsgL3.GetConsoleFontSize;

    return pResponders->GetConsoleFontSizeImpl(h, a->FontIndex, &a->FontSize);
}

NTSTATUS ApiDispatchers::ServeGetConsoleCurrentFont(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_CURRENTFONT_MSG* const a = &m->u.consoleMsgL3.GetCurrentConsoleFont;

    CONSOLE_FONT_INFOEX FontInfo = { 0 };
    FontInfo.cbSize = sizeof(FontInfo);

    DWORD Result = pResponders->GetCurrentConsoleFontExImpl(h, a->MaximumWindow, &FontInfo);

    if (SUCCEEDED(Result))
    {
        CopyMemory(a->FaceName, FontInfo.FaceName, RTL_NUMBER_OF_V2(a->FaceName));
        a->FontFamily = FontInfo.FontFamily;
        a->FontIndex = FontInfo.nFont;
        a->FontSize = FontInfo.dwFontSize;
        a->FontWeight = FontInfo.FontWeight;
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeSetConsoleDisplayMode(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_SETDISPLAYMODE_MSG* const a = &m->u.consoleMsgL3.SetConsoleDisplayMode;

    return pResponders->SetConsoleDisplayModeImpl(h, a->dwFlags, &a->ScreenBufferDimensions);
}

NTSTATUS ApiDispatchers::ServeGetConsoleDisplayMode(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_GETDISPLAYMODE_MSG* const a = &m->u.consoleMsgL3.GetConsoleDisplayMode;

    return pResponders->GetConsoleDisplayModeImpl(h, &a->ModeFlags);
}

NTSTATUS ApiDispatchers::ServeAddConsoleAlias(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_ADDALIAS_MSG* const a = &m->u.consoleMsgL3.AddConsoleAliasW;

    DWORD Result = 0;

    if (a->Unicode)
    {
        Result = pResponders->AddConsoleAliasWImpl(nullptr, 0, nullptr, 0, nullptr, 0);
    }
    else
    {
        Result = pResponders->AddConsoleAliasAImpl(nullptr, 0, nullptr, 0, nullptr, 0);
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeGetConsoleAlias(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_GETALIAS_MSG* const a = &m->u.consoleMsgL3.GetConsoleAliasW;

    DWORD Result = 0;

	// Check if input and output buffers are available to us
	if (!m->State.IsInputBufferAvailable() || !m->State.IsOutputBufferAvailable())
	{
		Result = STATUS_INVALID_PARAMETER;
	}

	if (SUCCEEDED(Result))
	{
		// Divide input buffer into the two input strings
		void* ExeBuffer = nullptr;
		ULONG const ExeBufferSize = a->ExeLength;
		void* SourceBuffer = nullptr;
		ULONG const SourceBufferSize = a->SourceLength;;
		
		Result = m->State.UnpackInputBuffer(a->Unicode, 2, ExeBufferSize, ExeBuffer, SourceBufferSize, SourceBuffer);

		// Get pointers to the output result buffer
		void* const OutBuffer = m->State.OutputBuffer;
		ULONG const OutBufferSize = m->State.OutputBufferSize;

		if (SUCCEEDED(Result))
		{
			// The buffers we created were measured in byte values but could be holding either Unicode (UCS-2, a subset of UTF-16) 
			//   or ASCII (which includes single and multibyte codepages and UTF-8) strings.
			// Cast and recalculate lengths to convert from raw byte length into appropriate string types/lengths.

			if (a->Unicode)
			{
				Result = pResponders->GetConsoleAliasWImpl(static_cast<wchar_t*>(SourceBuffer), 
															SourceBufferSize / sizeof(wchar_t), 
															static_cast<wchar_t*>(OutBuffer),
															OutBufferSize / sizeof(wchar_t), 
															static_cast<wchar_t*>(ExeBuffer),
															ExeBufferSize / sizeof(wchar_t));
			}
			else
			{
				Result = pResponders->GetConsoleAliasAImpl(static_cast<char*>(SourceBuffer),
															SourceBufferSize / sizeof(char), 
															static_cast<char*>(OutBuffer), 
															OutBufferSize / sizeof(char),
															static_cast<char*>(ExeBuffer), 
															ExeBufferSize / sizeof(char));
			}
		}
	}

    return Result;
}

NTSTATUS ApiDispatchers::ServeGetConsoleAliasesLength(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_GETALIASESLENGTH_MSG* const a = &m->u.consoleMsgL3.GetConsoleAliasesLengthW;

    DWORD Result = 0;

    if (a->Unicode)
    {
        Result = pResponders->GetConsoleAliasesLengthWImpl(nullptr, 0, &a->AliasesLength);
    }
    else
    {
        Result = pResponders->GetConsoleAliasesLengthAImpl(nullptr, 0, &a->AliasesLength);
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeGetConsoleAliasExesLength(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_GETALIASEXESLENGTH_MSG* const a = &m->u.consoleMsgL3.GetConsoleAliasExesLengthW;

    DWORD Result = 0;

    if (a->Unicode)
    {
        Result = pResponders->GetConsoleAliasExesLengthWImpl(&a->AliasExesLength);
    }
    else
    {
        Result = pResponders->GetConsoleAliasExesLengthAImpl(&a->AliasExesLength);
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeGetConsoleAliases(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_GETALIASES_MSG* const a = &m->u.consoleMsgL3.GetConsoleAliasesW;

    DWORD Result = 0;

    if (a->Unicode)
    {
        Result = pResponders->GetConsoleAliasesWImpl(nullptr, 0, nullptr, 0);
    }
    else
    {
        Result = pResponders->GetConsoleAliasesAImpl(nullptr, 0, nullptr, 0);
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeGetConsoleAliasExes(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_GETALIASEXES_MSG* const a = &m->u.consoleMsgL3.GetConsoleAliasExesW;

    DWORD Result = 0;

    if (a->Unicode)
    {
        Result = pResponders->GetConsoleAliasExesWImpl(nullptr, 0);
    }
    else
    {
        Result = pResponders->GetConsoleAliasExesAImpl(nullptr, 0);
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeGetConsoleWindow(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_GETCONSOLEWINDOW_MSG* const a = &m->u.consoleMsgL3.GetConsoleWindow;

    return pResponders->GetConsoleWindowImpl(&a->hwnd);
}

NTSTATUS ApiDispatchers::ServeGetConsoleSelectionInfo(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_GETSELECTIONINFO_MSG* const a = &m->u.consoleMsgL3.GetConsoleSelectionInfo;

    return pResponders->GetConsoleSelectionInfoImpl(&a->SelectionInfo);
}

NTSTATUS ApiDispatchers::ServeGetConsoleProcessList(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_GETCONSOLEPROCESSLIST_MSG* const a = &m->u.consoleMsgL3.GetConsoleProcessList;

    return pResponders->GetConsoleProcessListImpl(nullptr, 0);
}

NTSTATUS ApiDispatchers::ServeGetConsoleHistory(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_HISTORY_MSG* const a = &m->u.consoleMsgL3.GetConsoleHistory;

    CONSOLE_HISTORY_INFO Info;
    Info.cbSize = sizeof(Info);

    DWORD Result = pResponders->GetConsoleHistoryInfoImpl(&Info);

    if (SUCCEEDED(Result))
    {
        a->HistoryBufferSize = Info.HistoryBufferSize;
        a->NumberOfHistoryBuffers = Info.NumberOfHistoryBuffers;
        a->dwFlags = Info.dwFlags;
    }
    
    return Result;
}

NTSTATUS ApiDispatchers::ServeSetConsoleHistory(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_HISTORY_MSG* const a = &m->u.consoleMsgL3.SetConsoleHistory;

    CONSOLE_HISTORY_INFO Info;
    Info.cbSize = sizeof(Info);
    Info.dwFlags = a->dwFlags;
    Info.HistoryBufferSize = a->HistoryBufferSize;
    Info.NumberOfHistoryBuffers = a->NumberOfHistoryBuffers;

    return pResponders->SetConsoleHistoryInfoImpl(&Info);
}

NTSTATUS ApiDispatchers::ServeSetConsoleCurrentFont(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_CURRENTFONT_MSG* const a = &m->u.consoleMsgL3.SetCurrentConsoleFont;

    CONSOLE_FONT_INFOEX Info;
    Info.cbSize = sizeof(Info);
    Info.dwFontSize = a->FontSize;
    CopyMemory(Info.FaceName, a->FaceName, RTL_NUMBER_OF_V2(Info.FaceName));
    Info.FontFamily = a->FontFamily;
    Info.FontWeight = a->FontWeight;

    return pResponders->SetCurrentConsoleFontExImpl(h, a->MaximumWindow, &Info);
}
