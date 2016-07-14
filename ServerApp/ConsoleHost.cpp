#include "stdafx.h"
#pragma once
#include "DriverResponder.h"
#include "ConsoleHost.h"

#define KEY_ENHANCED 0x01000000
#define KEY_TRANSITION_UP 0x80000000
#define KEY_PRESSED 0x8000
#define KEY_TOGGLED 0x01
#define ALTNUMPAD_BIT  0x04000000

const TCHAR g_szClassName[] = TEXT("myWindowClass");
ConsoleHost* g_pConsole;

ConsoleHost::ConsoleHost()
{
    _pInputBuffer = nullptr;
    _pOutputBuffer = nullptr;
	g_pConsole = this;
}

ConsoleHost::~ConsoleHost()
{
    if (_pInputBuffer != nullptr)
    {
        delete _pInputBuffer;
    }
    if (_pOutputBuffer != nullptr)
    {
        delete _pOutputBuffer;
    }
}

DriverResponder* ConsoleHost::GetApiResponder()
{
    return _pDriverResponder;
}


NTSTATUS ConsoleHost::CreateIOBuffers(_Out_ IConsoleInputObject** const ppInputObject,
                         _Out_ IConsoleOutputObject** const ppOutputObject)
{
    _pInputBuffer = new InputBuffer();
    _pOutputBuffer = new OutputBuffer();
    *ppInputObject = _pInputBuffer;
    *ppOutputObject = _pOutputBuffer;

    // return (_pInputBuffer != nullptr && _pOutputBuffer != nullptr)? STATUS_SUCCESS : STATUS_FAILURE;
    return (_pInputBuffer != nullptr && _pOutputBuffer != nullptr)? 0 : -1;
}



// Step 4: the Window Procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_KEYDOWN:
		g_pConsole->GetApiResponder()->NotifyInput();
		g_pConsole->HandleKeyEvent(hwnd, msg, wParam, lParam, nullptr);
        break;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI GoWindow()
{
    WNDCLASSEX wc;
    HWND hwnd;
    MSG Msg;

    //Step 1: Registering the Window Class
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = 0;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = NULL;// hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = g_szClassName;
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassEx(&wc))
    {
        MessageBox(NULL, TEXT("Window Registration Failed!"), TEXT("Error!"),
                   MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    // Step 2: Creating the Window
    hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        g_szClassName,
        TEXT("OpenConsole"),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 480, 120,
        NULL, NULL, NULL, NULL);

    if (hwnd == NULL)
    {
        MessageBox(NULL, TEXT("Window Creation Failed!"), TEXT("Error!"),
                   MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    // Step 3: The Message Loop
    while (GetMessage(&Msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
    return (int)Msg.wParam;
}

void ConsoleHost::CreateWindowThread()
{
	_pWindowThread = new thread(GoWindow);
}


ULONG GetControlKeyState(_In_ const LPARAM lParam)
{
	ULONG ControlKeyState = 0;

	if (GetKeyState(VK_LMENU) & KEY_PRESSED)
	{
		ControlKeyState |= LEFT_ALT_PRESSED;
	}
	if (GetKeyState(VK_RMENU) & KEY_PRESSED)
	{
		ControlKeyState |= RIGHT_ALT_PRESSED;
	}
	if (GetKeyState(VK_LCONTROL) & KEY_PRESSED)
	{
		ControlKeyState |= LEFT_CTRL_PRESSED;
	}
	if (GetKeyState(VK_RCONTROL) & KEY_PRESSED)
	{
		ControlKeyState |= RIGHT_CTRL_PRESSED;
	}
	if (GetKeyState(VK_SHIFT) & KEY_PRESSED)
	{
		ControlKeyState |= SHIFT_PRESSED;
	}
	if (GetKeyState(VK_NUMLOCK) & KEY_TOGGLED)
	{
		ControlKeyState |= NUMLOCK_ON;
	}
	if (GetKeyState(VK_SCROLL) & KEY_TOGGLED)
	{
		ControlKeyState |= SCROLLLOCK_ON;
	}
	if (GetKeyState(VK_CAPITAL) & KEY_TOGGLED)
	{
		ControlKeyState |= CAPSLOCK_ON;
	}
	if (lParam & KEY_ENHANCED)
	{
		ControlKeyState |= ENHANCED_KEY;
	}

	ControlKeyState |= (lParam & ALTNUMPAD_BIT);

	return ControlKeyState;
}

void ConsoleHost::HandleKeyEvent(_In_ const HWND hWnd, _In_ const UINT Message, _In_ const WPARAM wParam, _In_ const LPARAM lParam, _Inout_opt_ PBOOL pfUnlockConsole)
{
    // BOOLEAN ContinueProcessing;
    
    BOOL bGenerateBreak = FALSE;

    // BOGUS for WM_CHAR/WM_DEADCHAR, in which LOWORD(lParam) is a character
    WORD VirtualKeyCode = LOWORD(wParam);
    const ULONG ControlKeyState = GetControlKeyState(lParam);
    const BOOL bKeyDown = !(lParam & KEY_TRANSITION_UP);

    // if (bKeyDown)
    // {
    //     // Log a telemetry flag saying the user interacted with the Console
    //     // Only log when the key is a down press.  Otherwise we're getting many calls with
    //     // Message = WM_CHAR, VirtualKeyCode = VK_TAB, with bKeyDown = false
    //     // when nothing is happening, or the user has merely clicked on the title bar, and
    //     // this can incorrectly mark the session as being interactive.
    //     Telemetry::Instance().SetUserInteractive();
    // }

    // Make sure we retrieve the key info first, or we could chew up unneeded space in the key info table if we bail out early.
    INPUT_RECORD InputEvent;
    InputEvent.Event.KeyEvent.wVirtualKeyCode = VirtualKeyCode;
    InputEvent.Event.KeyEvent.wVirtualScanCode = (BYTE) (HIWORD(lParam));
    // if (Message == WM_CHAR || Message == WM_SYSCHAR || Message == WM_DEADCHAR || Message == WM_SYSDEADCHAR)
    // {
        // RetrieveKeyInfo(hWnd,
        //                 &InputEvent.Event.KeyEvent.wVirtualKeyCode,
        //                 &InputEvent.Event.KeyEvent.wVirtualScanCode,
        //                 !g_ciConsoleInformation.pInputBuffer->ImeMode.InComposition);

    //     VirtualKeyCode = InputEvent.Event.KeyEvent.wVirtualKeyCode;
    // }

    InputEvent.EventType = KEY_EVENT;
    InputEvent.Event.KeyEvent.bKeyDown = bKeyDown;
    InputEvent.Event.KeyEvent.wRepeatCount = LOWORD(lParam);

    if (Message == WM_CHAR || Message == WM_SYSCHAR || Message == WM_DEADCHAR || Message == WM_SYSDEADCHAR)
    {
        // If this is a fake character, zero the scancode.
        if (lParam & 0x02000000)
        {
            InputEvent.Event.KeyEvent.wVirtualScanCode = 0;
        }
        InputEvent.Event.KeyEvent.dwControlKeyState = GetControlKeyState(lParam);
        if (Message == WM_CHAR || Message == WM_SYSCHAR)
        {
            InputEvent.Event.KeyEvent.uChar.UnicodeChar = (WCHAR)wParam;
        }
        else
        {
            InputEvent.Event.KeyEvent.uChar.UnicodeChar = (WCHAR)0;
        }
    }
    else
    {
        // if alt-gr, ignore
        if (lParam & 0x02000000)
        {
            return;
        }
        InputEvent.Event.KeyEvent.dwControlKeyState = ControlKeyState;
        InputEvent.Event.KeyEvent.uChar.UnicodeChar = 0;
    }
        // EventsWritten = WriteInputBuffer(g_ciConsoleInformation.pInputBuffer, &InputEvent, 1);

   auto EventsWritten = _pInputBuffer->WriteInputEvent(&InputEvent, 1);
}