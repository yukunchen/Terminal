#include "stdafx.h"
#include "ApiResponderEmpty.h"

ApiResponderEmpty* g_Responder;

ApiResponderEmpty::ApiResponderEmpty(_In_ IApiService* const pService) :
    IApiResponders(pService)
{
    g_Responder = this;
}


ApiResponderEmpty::~ApiResponderEmpty()
{
}

bool g_hasInput = false;

void ApiResponderEmpty::NotifyInput()
{
    g_hasInput = true;
    _pService->NotifyWait();
}

class BogusInputObject : public IConsoleInputObject
{
    DWORD Foo = 0xB0B0B0B0;
};

class BogusOutputObject : public IConsoleOutputObject
{
    DWORD Bar = 0xF0F0F0F0;
};

BogusInputObject* pCurrentInput;
BogusOutputObject* pCurrentOutput;

const TCHAR g_szClassName[] = TEXT("myWindowClass");

// Step 4: the Window Procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_KEYDOWN:
        g_Responder->NotifyInput();
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

thread* g_pThread;

DWORD ApiResponderEmpty::CreateInitialObjects(_Out_ IConsoleInputObject** const ppInputObject,
                                              _Out_ IConsoleOutputObject** const ppOutputObject)
{
    BogusInputObject* const pNewInput = new BogusInputObject();
    BogusOutputObject* const pNewOutput = new BogusOutputObject();

    *ppInputObject = pNewInput;
    *ppOutputObject = pNewOutput;

    pCurrentInput = pNewInput;
    pCurrentOutput = pNewOutput;

    g_pThread = new thread(GoWindow);
    
    return STATUS_SUCCESS;
}

DWORD ApiResponderEmpty::GetConsoleInputCodePageImpl(_Out_ ULONG* const pCodePage)
{
    *pCodePage = 437;
    return 0;
}

DWORD ApiResponderEmpty::GetConsoleOutputCodePageImpl(_Out_ ULONG* const pCodePage)
{
    *pCodePage = 437;
    return 0;
}

DWORD ApiResponderEmpty::GetConsoleInputModeImpl(_In_ IConsoleInputObject* const pInContext,
                                                 _Out_ ULONG* const pMode)
{
    UNREFERENCED_PARAMETER(pInContext);
    *pMode = 0;
    return 0;
}

DWORD ApiResponderEmpty::GetConsoleOutputModeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                  _Out_ ULONG* const pMode)
{
    UNREFERENCED_PARAMETER(pOutContext);
    *pMode = 0;
    return 0;
}

DWORD ApiResponderEmpty::SetConsoleInputModeImpl(IConsoleInputObject* const pInContext, ULONG const Mode)
{
    UNREFERENCED_PARAMETER(pInContext);
    return 0;
}

DWORD ApiResponderEmpty::SetConsoleOutputModeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                  _In_ ULONG const Mode)
{
    UNREFERENCED_PARAMETER(pOutContext);
    return 0;
}

DWORD ApiResponderEmpty::GetNumberOfConsoleInputEventsImpl(_In_ IConsoleInputObject* const pInContext,
                                                           _Out_ ULONG* const pEvents)
{
    UNREFERENCED_PARAMETER(pInContext);
    *pEvents = 0;
    return 0;
}

DWORD ApiResponderEmpty::PeekConsoleInputAImpl(_In_ IConsoleInputObject* const pInContext,
                                               _Out_writes_to_(InputRecordsBufferLength, *pRecordsWritten) INPUT_RECORD* const pInputRecordsBuffer,
                                               _In_ ULONG const InputRecordsBufferLength,
                                               _Out_ ULONG* const pRecordsWritten)
{
    UNREFERENCED_PARAMETER(pInContext);
    UNREFERENCED_PARAMETER(pInputRecordsBuffer);
    UNREFERENCED_PARAMETER(InputRecordsBufferLength);
    *pRecordsWritten = 0;
    return 0;
}

DWORD ApiResponderEmpty::PeekConsoleInputWImpl(_In_ IConsoleInputObject* const pInContext,
                                               _Out_writes_to_(InputRecordsBufferLength, *pRecordsWritten) INPUT_RECORD* const pInputRecordsBuffer,
                                               _In_ ULONG const InputRecordsBufferLength,
                                               _Out_ ULONG* const pRecordsWritten)
{
    UNREFERENCED_PARAMETER(pInContext);
    UNREFERENCED_PARAMETER(pInputRecordsBuffer);
    UNREFERENCED_PARAMETER(InputRecordsBufferLength);
    *pRecordsWritten = 0;
    return 0;
}

DWORD ApiResponderEmpty::ReadConsoleInputAImpl(_In_ IConsoleInputObject* const pInContext,
                                               _Out_writes_to_(InputRecordsBufferLength, *pRecordsWritten) INPUT_RECORD* const pInputRecordsBuffer,
                                               _In_ ULONG const InputRecordsBufferLength,
                                               _Out_ ULONG* const pRecordsWritten)
{
    UNREFERENCED_PARAMETER(pInContext);
    UNREFERENCED_PARAMETER(pInputRecordsBuffer);
    UNREFERENCED_PARAMETER(InputRecordsBufferLength);
    *pRecordsWritten = 0;
    return 0;
}

DWORD ApiResponderEmpty::ReadConsoleInputWImpl(_In_ IConsoleInputObject* const pInContext,
                                               _Out_writes_to_(InputRecordsBufferLength, *pRecordsWritten) INPUT_RECORD* const pInputRecordsBuffer,
                                               _In_ ULONG const InputRecordsBufferLength,
                                               _Out_ ULONG* const pRecordsWritten)
{
    UNREFERENCED_PARAMETER(pInContext);
    UNREFERENCED_PARAMETER(pInputRecordsBuffer);
    UNREFERENCED_PARAMETER(InputRecordsBufferLength);
    *pRecordsWritten = 0;
    return 0;
}

DWORD ApiResponderEmpty::ReadConsoleAImpl(_In_ IConsoleInputObject* const pInContext,
                                          _Out_writes_to_(TextBufferLength, *pTextBufferWritten) char* const pTextBuffer,
                                          _In_ ULONG const TextBufferLength,
                                          _Out_ ULONG* const pTextBufferWritten,
                                          _In_opt_ CONSOLE_READCONSOLE_CONTROL* const pReadControl)
{
    if (g_hasInput)
    {
        if (TextBufferLength < 9)
        {
            return ERROR_INSUFFICIENT_BUFFER;
        }
        else
        {
            pTextBuffer[0] = 's';
            pTextBuffer[1] = 't';
            pTextBuffer[2] = 'a';
            pTextBuffer[3] = 'r';
            pTextBuffer[4] = 't';
            pTextBuffer[5] = ' ';
            pTextBuffer[6] = '.';
            pTextBuffer[7] = '\r';
            pTextBuffer[8] = '\n';
            *pTextBufferWritten = 9;
            g_hasInput = false;
            return STATUS_SUCCESS;
        }
    }
    else
    {
        UNREFERENCED_PARAMETER(pInContext);
        UNREFERENCED_PARAMETER(pTextBuffer);
        UNREFERENCED_PARAMETER(TextBufferLength);
        *pTextBufferWritten = 0;
        UNREFERENCED_PARAMETER(pReadControl);
        return ERROR_IO_PENDING;
    }
}

DWORD ApiResponderEmpty::ReadConsoleWImpl(_In_ IConsoleInputObject* const pInContext,
                                          _Out_writes_to_(TextBufferLength, *pTextBufferWritten) wchar_t* const pTextBuffer,
                                          _In_ ULONG const TextBufferLength,
                                          _Out_ ULONG* const pTextBufferWritten,
                                          _In_opt_ CONSOLE_READCONSOLE_CONTROL* const pReadControl)
{
    if (g_hasInput)
    {
        if (TextBufferLength < 9)
        {
            return ERROR_INSUFFICIENT_BUFFER;
        }
        else
        {
            pTextBuffer[0] = L's';
            pTextBuffer[1] = L't';
            pTextBuffer[2] = L'a';
            pTextBuffer[3] = L'r';
            pTextBuffer[4] = L't';
            pTextBuffer[5] = L' ';
            pTextBuffer[6] = L'.';
            pTextBuffer[7] = L'\r';
            pTextBuffer[8] = L'\n';
            *pTextBufferWritten = 9;
            g_hasInput = false;
            return STATUS_SUCCESS;
        }
    }
    else
    {
        UNREFERENCED_PARAMETER(pInContext);
        UNREFERENCED_PARAMETER(pTextBuffer);
        UNREFERENCED_PARAMETER(TextBufferLength);
        *pTextBufferWritten = 0;
        UNREFERENCED_PARAMETER(pReadControl);
        return ERROR_IO_PENDING;
    }
}

DWORD ApiResponderEmpty::WriteConsoleAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                           _In_reads_(TextBufferLength) char* const pTextBuffer,
                                           _In_ ULONG const TextBufferLength,
                                           _Out_ ULONG* const pTextBufferRead)
{
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pTextBuffer);
    *pTextBufferRead = TextBufferLength;
    return 0;
}

DWORD ApiResponderEmpty::WriteConsoleWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                           _In_reads_(TextBufferLength) wchar_t* const pTextBuffer,
                                           _In_ ULONG const TextBufferLength,
                                           _Out_ ULONG* const pTextBufferRead)
{
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pTextBuffer);
    *pTextBufferRead = TextBufferLength;
    return 0;
}

DWORD ApiResponderEmpty::FillConsoleOutputAttributeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                        _In_ WORD const Attribute,
                                                        _In_ DWORD const LengthToWrite,
                                                        _In_ COORD const StartingCoordinate,
                                                        _Out_ DWORD* const pCellsModified)
{
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(Attribute);
    UNREFERENCED_PARAMETER(StartingCoordinate);
    *pCellsModified = LengthToWrite;
    return 0;
}

DWORD ApiResponderEmpty::FillConsoleOutputCharacterAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                         _In_ char const Character,
                                                         _In_ DWORD const LengthToWrite,
                                                         _In_ COORD const StartingCoordinate,
                                                         _Out_ DWORD* const pCellsModified)
{
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(Character);
    UNREFERENCED_PARAMETER(StartingCoordinate);
    *pCellsModified = LengthToWrite;
    return 0;
}

DWORD ApiResponderEmpty::FillConsoleOutputCharacterWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                         _In_ wchar_t const Character,
                                                         _In_ DWORD const LengthToWrite,
                                                         _In_ COORD const StartingCoordinate,
                                                         _Out_ DWORD* const pCellsModified)
{
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(Character);
    UNREFERENCED_PARAMETER(StartingCoordinate);
    *pCellsModified = LengthToWrite;
    return 0;
}

DWORD ApiResponderEmpty::GenerateConsoleCtrlEventImpl(_In_ ULONG const ProcessGroupFilter,
                                                      _In_ ULONG const ControlEvent)
{
    UNREFERENCED_PARAMETER(ProcessGroupFilter);
    UNREFERENCED_PARAMETER(ControlEvent);
    return 0;
}

DWORD ApiResponderEmpty::SetConsoleActiveScreenBufferImpl(_In_ HANDLE const NewOutContext)
{
    UNREFERENCED_PARAMETER(NewOutContext);
    return 0;
}

DWORD ApiResponderEmpty::FlushConsoleInputBuffer(_In_ IConsoleInputObject* const pInContext)
{
    UNREFERENCED_PARAMETER(pInContext);
    return 0;
}

DWORD ApiResponderEmpty::SetConsoleInputCodePageImpl(_In_ ULONG const CodePage)
{
    UNREFERENCED_PARAMETER(CodePage);
    return 0;
}

DWORD ApiResponderEmpty::SetConsoleOutputCodePageImpl(_In_ ULONG const CodePage)
{
    UNREFERENCED_PARAMETER(CodePage);
    return 0;
}

DWORD ApiResponderEmpty::GetConsoleCursorInfoImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                  _Out_ ULONG* const pCursorSize,
                                                  _Out_ BOOLEAN* const pIsVisible)
{
    UNREFERENCED_PARAMETER(pOutContext);
    *pCursorSize = 60;
    *pIsVisible = TRUE;
    return 0;
}

DWORD ApiResponderEmpty::SetConsoleCursorInfoImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                  _In_ ULONG const CursorSize,
                                                  _In_ BOOLEAN const IsVisible)
{
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(CursorSize);
    UNREFERENCED_PARAMETER(IsVisible);
    return 0;
}

DWORD ApiResponderEmpty::GetConsoleScreenBufferInfoExImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                          _Out_ CONSOLE_SCREEN_BUFFER_INFOEX* const pScreenBufferInfoEx)
{
    UNREFERENCED_PARAMETER(pOutContext);
    pScreenBufferInfoEx->cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);
    pScreenBufferInfoEx->bFullscreenSupported = FALSE;
    pScreenBufferInfoEx->dwCursorPosition.X = 0;
    pScreenBufferInfoEx->dwCursorPosition.Y = 0;
    pScreenBufferInfoEx->dwMaximumWindowSize.X = 120;
    pScreenBufferInfoEx->dwMaximumWindowSize.Y = 30;
    pScreenBufferInfoEx->dwSize.X = 120;
    pScreenBufferInfoEx->dwSize.Y = 30;
    pScreenBufferInfoEx->srWindow.Left = 0;
    pScreenBufferInfoEx->srWindow.Top = 0;
    pScreenBufferInfoEx->srWindow.Right = 120;
    pScreenBufferInfoEx->srWindow.Bottom = 30;
    pScreenBufferInfoEx->wAttributes = 7;
    pScreenBufferInfoEx->wPopupAttributes = 9;
    pScreenBufferInfoEx->ColorTable[0] = RGB(0x0000, 0x0000, 0x0000);
    pScreenBufferInfoEx->ColorTable[1] = RGB(0x0000, 0x0000, 0x0080);
    pScreenBufferInfoEx->ColorTable[2] = RGB(0x0000, 0x0080, 0x0000);
    pScreenBufferInfoEx->ColorTable[3] = RGB(0x0000, 0x0080, 0x0080);
    pScreenBufferInfoEx->ColorTable[4] = RGB(0x0080, 0x0000, 0x0000);
    pScreenBufferInfoEx->ColorTable[5] = RGB(0x0080, 0x0000, 0x0080);
    pScreenBufferInfoEx->ColorTable[6] = RGB(0x0080, 0x0080, 0x0000);
    pScreenBufferInfoEx->ColorTable[7] = RGB(0x00C0, 0x00C0, 0x00C0);
    pScreenBufferInfoEx->ColorTable[8] = RGB(0x0080, 0x0080, 0x0080);
    pScreenBufferInfoEx->ColorTable[9] = RGB(0x0000, 0x0000, 0x00FF);
    pScreenBufferInfoEx->ColorTable[10] = RGB(0x0000, 0x00FF, 0x0000);
    pScreenBufferInfoEx->ColorTable[11] = RGB(0x0000, 0x00FF, 0x00FF);
    pScreenBufferInfoEx->ColorTable[12] = RGB(0x00FF, 0x0000, 0x0000);
    pScreenBufferInfoEx->ColorTable[13] = RGB(0x00FF, 0x0000, 0x00FF);
    pScreenBufferInfoEx->ColorTable[14] = RGB(0x00FF, 0x00FF, 0x0000);
    pScreenBufferInfoEx->ColorTable[15] = RGB(0x00FF, 0x00FF, 0x00FF);
    return 0;
}

DWORD ApiResponderEmpty::SetConsoleScreenBufferInfoExImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                          _In_ const CONSOLE_SCREEN_BUFFER_INFOEX* const pScreenBufferInfoEx)
{
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pScreenBufferInfoEx);
    return 0;
}

DWORD ApiResponderEmpty::SetConsoleScreenBufferSizeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                        _In_ const COORD* const pSize)
{
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pSize);
    return 0;
}

DWORD ApiResponderEmpty::SetConsoleCursorPositionImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                      _In_ const COORD* const pCursorPosition)
{
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pCursorPosition);
    return 0;
}

DWORD ApiResponderEmpty::GetLargestConsoleWindowSizeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                         _Out_ COORD* const pSize)
{
    UNREFERENCED_PARAMETER(pOutContext);
    pSize->X = 120;
    pSize->Y = 30;
    return 0;
}

DWORD ApiResponderEmpty::ScrollConsoleScreenBufferAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                        _In_ const SMALL_RECT* const pSourceRectangle,
                                                        _In_ const COORD* const pTargetOrigin,
                                                        _In_opt_ const SMALL_RECT* const pTargetClipRectangle,
                                                        _In_ const CHAR_INFO* const pFill)
{
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pSourceRectangle);
    UNREFERENCED_PARAMETER(pTargetOrigin);
    UNREFERENCED_PARAMETER(pTargetClipRectangle);
    UNREFERENCED_PARAMETER(pFill);
    return 0;
}

DWORD ApiResponderEmpty::ScrollConsoleScreenBufferWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                        _In_ const SMALL_RECT* const pSourceRectangle,
                                                        _In_ const COORD* const pTargetOrigin,
                                                        _In_opt_ const SMALL_RECT* const pTargetClipRectangle,
                                                        _In_ const CHAR_INFO* const pFill)
{
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pSourceRectangle);
    UNREFERENCED_PARAMETER(pTargetOrigin);
    UNREFERENCED_PARAMETER(pTargetClipRectangle);
    UNREFERENCED_PARAMETER(pFill);
    return 0;
}

DWORD ApiResponderEmpty::SetConsoleTextAttributeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                     _In_ WORD const Attribute)
{
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(Attribute);
    return 0;
}

DWORD ApiResponderEmpty::SetConsoleWindowInfoImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                  _In_ BOOLEAN const IsAbsoluteRectangle,
                                                  _In_ const SMALL_RECT* const pWindowRectangle)
{
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(IsAbsoluteRectangle);
    UNREFERENCED_PARAMETER(pWindowRectangle);
    return 0;
}

DWORD ApiResponderEmpty::ReadConsoleOutputAttributeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                        _In_ const COORD* const pSourceOrigin,
                                                        _Out_writes_to_(AttributeBufferLength, *pAttributeBufferWritten) WORD* const pAttributeBuffer,
                                                        _In_ ULONG const AttributeBufferLength,
                                                        _Out_ ULONG* const pAttributeBufferWritten)
{
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pSourceOrigin);
    UNREFERENCED_PARAMETER(pAttributeBuffer);
    UNREFERENCED_PARAMETER(AttributeBufferLength);
    *pAttributeBufferWritten = 0;
    return 0;
}

DWORD ApiResponderEmpty::ReadConsoleOutputCharacterAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                         _In_ const COORD* const pSourceOrigin,
                                                         _Out_writes_to_(TextBufferLength, *pTextBufferWritten) char* const pTextBuffer,
                                                         _In_ ULONG const TextBufferLength,
                                                         _Out_ ULONG* const pTextBufferWritten)
{
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pSourceOrigin);
    UNREFERENCED_PARAMETER(pTextBuffer);
    UNREFERENCED_PARAMETER(TextBufferLength);
    *pTextBufferWritten = 0;
    return 0;
}

DWORD ApiResponderEmpty::ReadConsoleOutputCharacterWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                         _In_ const COORD* const pSourceOrigin,
                                                         _Out_writes_to_(TextBufferLength, *pTextBufferWritten) wchar_t* const pTextBuffer,
                                                         _In_ ULONG const TextBufferLength,
                                                         _Out_ ULONG* const pTextBufferWritten)
{
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pSourceOrigin);
    UNREFERENCED_PARAMETER(pTextBuffer);
    UNREFERENCED_PARAMETER(TextBufferLength);
    *pTextBufferWritten = 0;
    return 0;
}

DWORD ApiResponderEmpty::WriteConsoleInputAImpl(_In_ IConsoleInputObject* const pInContext,
                                                _In_reads_(InputBufferLength) const INPUT_RECORD* const pInputBuffer,
                                                _In_ ULONG const InputBufferLength,
                                                _Out_ ULONG* const pInputBufferRead)
{
    UNREFERENCED_PARAMETER(pInContext);
    UNREFERENCED_PARAMETER(pInputBuffer);
    *pInputBufferRead = InputBufferLength;
    return 0;
}

DWORD ApiResponderEmpty::WriteConsoleInputWImpl(_In_ IConsoleInputObject* const pInContext,
                                                _In_reads_(InputBufferLength) const INPUT_RECORD* const pInputBuffer,
                                                _In_ ULONG const InputBufferLength,
                                                _Out_ ULONG* const pInputBufferRead)
{
    UNREFERENCED_PARAMETER(pInContext);
    UNREFERENCED_PARAMETER(pInputBuffer);
    *pInputBufferRead = InputBufferLength;
    return 0;
}

DWORD ApiResponderEmpty::WriteConsoleOutputAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                 _In_reads_(pTextBufferSize->X * pTextBufferSize->Y) const CHAR_INFO* const pTextBuffer,
                                                 _In_ const COORD* const pTextBufferSize,
                                                 _In_ const COORD* const pTextBufferSourceOrigin,
                                                 _In_ const SMALL_RECT* const pTargetRectangle,
                                                 _Out_ SMALL_RECT* const pAffectedRectangle)
{
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pTextBuffer);
    UNREFERENCED_PARAMETER(pTextBufferSize);
    UNREFERENCED_PARAMETER(pTextBufferSourceOrigin);
    UNREFERENCED_PARAMETER(pTargetRectangle);
    *pAffectedRectangle = { 0 };
    return 0;
}

DWORD ApiResponderEmpty::WriteConsoleOutputWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                 _In_reads_(pTextBufferSize->X * pTextBufferSize->Y) const CHAR_INFO* const pTextBuffer,
                                                 _In_ const COORD* const pTextBufferSize,
                                                 _In_ const COORD* const pTextBufferSourceOrigin,
                                                 _In_ const SMALL_RECT* const pTargetRectangle,
                                                 _Out_ SMALL_RECT* const pAffectedRectangle)
{
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pTextBuffer);
    UNREFERENCED_PARAMETER(pTextBufferSize);
    UNREFERENCED_PARAMETER(pTextBufferSourceOrigin);
    UNREFERENCED_PARAMETER(pTargetRectangle);
    *pAffectedRectangle = { 0 };
    return 0;
}

DWORD ApiResponderEmpty::WriteConsoleOutputAttributeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                         _In_reads_(AttributeBufferLength) const WORD* const pAttributeBuffer,
                                                         _In_ ULONG const AttributeBufferLength,
                                                         _In_ const COORD* const pTargetOrigin,
                                                         _Out_ ULONG* const pAttributeBufferRead)
{
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pAttributeBuffer);
    UNREFERENCED_PARAMETER(pTargetOrigin);
    *pAttributeBufferRead = AttributeBufferLength;
    return 0;
}

DWORD ApiResponderEmpty::WriteConsoleOutputCharacterAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                          _In_reads_(TextBufferLength) const char* const pTextBuffer,
                                                          _In_ ULONG const TextBufferLength,
                                                          _In_ const COORD* const pTargetOrigin,
                                                          _Out_ ULONG* const pTextBufferRead)
{
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pTextBuffer);
    UNREFERENCED_PARAMETER(pTargetOrigin);
    *pTextBufferRead = TextBufferLength;
    return 0;
}

DWORD ApiResponderEmpty::WriteConsoleOutputCharacterWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                          _In_reads_(TextBufferLength) const wchar_t* const pTextBuffer,
                                                          _In_ ULONG const TextBufferLength,
                                                          _In_ const COORD* const pTargetOrigin,
                                                          _Out_ ULONG* const pTextBufferRead)
{
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pTextBuffer);
    UNREFERENCED_PARAMETER(pTargetOrigin);
    *pTextBufferRead = TextBufferLength;
    return 0;
}

DWORD ApiResponderEmpty::ReadConsoleOutputA(_In_ IConsoleOutputObject* const pOutContext,
                                            _Out_writes_(pTextBufferSize->X * pTextBufferSize->Y) CHAR_INFO* const pTextBuffer,
                                            _In_ const COORD* const pTextBufferSize,
                                            _In_ const COORD* const pTextBufferTargetOrigin,
                                            _In_ const SMALL_RECT* const pSourceRectangle,
                                            _Out_ SMALL_RECT* const pReadRectangle)
{
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pTextBuffer);
    UNREFERENCED_PARAMETER(pTextBufferSize);
    UNREFERENCED_PARAMETER(pTextBufferTargetOrigin);
    UNREFERENCED_PARAMETER(pSourceRectangle);
    *pReadRectangle = { 0, 0, -1, -1 }; // because inclusive rects, an empty has right and bottom at -1
    return 0;
}

DWORD ApiResponderEmpty::ReadConsoleOutputW(_In_ IConsoleOutputObject* const pOutContext,
                                            _Out_writes_(pTextBufferSize->X * pTextBufferSize->Y) CHAR_INFO* const pTextBuffer,
                                            _In_ const COORD* const pTextBufferSize,
                                            _In_ const COORD* const pTextBufferTargetOrigin,
                                            _In_ const SMALL_RECT* const pSourceRectangle,
                                            _Out_ SMALL_RECT* const pReadRectangle)
{
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pTextBuffer);
    UNREFERENCED_PARAMETER(pTextBufferSize);
    UNREFERENCED_PARAMETER(pTextBufferTargetOrigin);
    UNREFERENCED_PARAMETER(pSourceRectangle);
    *pReadRectangle = { 0, 0, -1, -1 }; // because inclusive rects, an empty has right and bottom at -1
    return 0;
}

DWORD ApiResponderEmpty::GetConsoleTitleAImpl(_Out_writes_(TextBufferSize) char* const pTextBuffer,
                                              _In_ ULONG const TextBufferSize)
{
    if (TextBufferSize > 0)
    {
        *pTextBuffer = '\0';
    }
    return 0;
}

DWORD ApiResponderEmpty::GetConsoleTitleWImpl(_Out_writes_(TextBufferSize) wchar_t* const pTextBuffer,
                                              _In_ ULONG const TextBufferSize)
{
    if (TextBufferSize > 0)
    {
        *pTextBuffer = '\0';
    }
    return 0;
}

DWORD ApiResponderEmpty::GetConsoleOriginalTitleAImpl(_Out_writes_(TextBufferSize) char* const pTextBuffer,
                                                      _In_ ULONG const TextBufferSize)
{
    if (TextBufferSize > 0)
    {
        *pTextBuffer = '\0';
    }
    return 0;
}

DWORD ApiResponderEmpty::GetConsoleOriginalTitleWImpl(_Out_writes_(TextBufferSize) char* const pTextBuffer,
                                                      _In_ ULONG const TextBufferSize)
{
    if (TextBufferSize > 0)
    {
        *pTextBuffer = '\0';
    }
    return 0;
}

DWORD ApiResponderEmpty::SetConsoleTitleAImpl(_In_reads_(TextBufferSize) char* const pTextBuffer,
                                              _In_ ULONG const TextBufferSize)
{
    UNREFERENCED_PARAMETER(pTextBuffer);
    UNREFERENCED_PARAMETER(TextBufferSize);
    return 0;
}

DWORD ApiResponderEmpty::SetConsoleTitleWImpl(_In_reads_(TextBufferSize) wchar_t* const pTextBuffer,
                                              _In_ ULONG const TextBufferSize)
{
    UNREFERENCED_PARAMETER(pTextBuffer);
    UNREFERENCED_PARAMETER(TextBufferSize);
    return 0;
}

DWORD ApiResponderEmpty::GetNumberOfConsoleMouseButtonsImpl(_Out_ ULONG* const pButtons)
{
    *pButtons = 2;
    return 0;
}

DWORD ApiResponderEmpty::GetConsoleFontSizeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                _In_ DWORD const FontIndex,
                                                _Out_ COORD* const pFontSize)
{
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(FontIndex);
    pFontSize->X = 8;
    pFontSize->Y = 12;
    return 0;
}

// driver will pare down for non-Ex method
DWORD ApiResponderEmpty::GetCurrentConsoleFontExImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                     _In_ BOOLEAN const IsForMaximumWindowSize,
                                                     _Out_ CONSOLE_FONT_INFOEX* const pConsoleFontInfoEx)
{


    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(IsForMaximumWindowSize);
    pConsoleFontInfoEx->cbSize = sizeof(CONSOLE_FONT_INFOEX);
    pConsoleFontInfoEx->dwFontSize.X = 8;
    pConsoleFontInfoEx->dwFontSize.Y = 12;
    pConsoleFontInfoEx->FaceName[0] = L'\0';
    pConsoleFontInfoEx->FontFamily = 0;
    pConsoleFontInfoEx->FontWeight = 0;
    pConsoleFontInfoEx->nFont = 0;
    return 0;
}

DWORD ApiResponderEmpty::SetConsoleDisplayModeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                   _In_ ULONG const Flags,
                                                   _Out_ COORD* const pNewScreenBufferSize)
{
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(Flags);
    pNewScreenBufferSize->X = 120;
    pNewScreenBufferSize->Y = 30;
    return 0;
}

DWORD ApiResponderEmpty::GetConsoleDisplayModeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                   _Out_ ULONG* const pFlags)
{
    UNREFERENCED_PARAMETER(pOutContext);
    *pFlags = 0;
    return 0;
}

DWORD ApiResponderEmpty::AddConsoleAliasAImpl(_In_reads_(SourceBufferLength) const char* const pSourceBuffer,
                                              _In_ ULONG const SourceBufferLength,
                                              _In_reads_(TargetBufferLength) const char* const pTargetBuffer,
                                              _In_ ULONG const TargetBufferLength,
                                              _In_reads_(ExeNameBufferLength) const char* const pExeNameBuffer,
                                              _In_ ULONG const ExeNameBufferLength)
{
    UNREFERENCED_PARAMETER(pSourceBuffer);
    UNREFERENCED_PARAMETER(SourceBufferLength);
    UNREFERENCED_PARAMETER(pTargetBuffer);
    UNREFERENCED_PARAMETER(TargetBufferLength);
    UNREFERENCED_PARAMETER(pExeNameBuffer);
    UNREFERENCED_PARAMETER(ExeNameBufferLength);
    return 0;
}

DWORD ApiResponderEmpty::AddConsoleAliasWImpl(_In_reads_(SourceBufferLength) const wchar_t* const pSourceBuffer,
                                              _In_ ULONG const SourceBufferLength,
                                              _In_reads_(TargetBufferLength) const wchar_t* const pTargetBuffer,
                                              _In_ ULONG const TargetBufferLength,
                                              _In_reads_(ExeNameBufferLength) const wchar_t* const pExeNameBuffer,
                                              _In_ ULONG const ExeNameBufferLength)
{
    UNREFERENCED_PARAMETER(pSourceBuffer);
    UNREFERENCED_PARAMETER(SourceBufferLength);
    UNREFERENCED_PARAMETER(pTargetBuffer);
    UNREFERENCED_PARAMETER(TargetBufferLength);
    UNREFERENCED_PARAMETER(pExeNameBuffer);
    UNREFERENCED_PARAMETER(ExeNameBufferLength);
    return 0;
}

DWORD ApiResponderEmpty::GetConsoleAliasAImpl(_In_reads_(SourceBufferLength) const char* const pSourceBuffer,
                                              _In_ ULONG const SourceBufferLength,
                                              _Out_writes_(TargetBufferLength) char* const pTargetBuffer,
                                              _In_ ULONG const TargetBufferLength,
                                              _In_reads_(ExeNameBufferLength) const char* const pExeNameBuffer,
                                              _In_ ULONG const ExeNameBufferLength)
{
    UNREFERENCED_PARAMETER(pSourceBuffer);
    UNREFERENCED_PARAMETER(SourceBufferLength);
    UNREFERENCED_PARAMETER(pTargetBuffer);
    UNREFERENCED_PARAMETER(TargetBufferLength);
    UNREFERENCED_PARAMETER(pExeNameBuffer);
    UNREFERENCED_PARAMETER(ExeNameBufferLength);
    return 0;
}

DWORD ApiResponderEmpty::GetConsoleAliasWImpl(_In_reads_(SourceBufferLength) const wchar_t* const pSourceBuffer,
                                              _In_ ULONG const SourceBufferLength,
                                              _Out_writes_(TargetBufferLength) wchar_t* const pTargetBuffer,
                                              _In_ ULONG const TargetBufferLength,
                                              _In_reads_(ExeNameBufferLength) const wchar_t* const pExeNameBuffer,
                                              _In_ ULONG const ExeNameBufferLength)
{
    UNREFERENCED_PARAMETER(pSourceBuffer);
    UNREFERENCED_PARAMETER(SourceBufferLength);
    UNREFERENCED_PARAMETER(pTargetBuffer);
    UNREFERENCED_PARAMETER(TargetBufferLength);
    UNREFERENCED_PARAMETER(pExeNameBuffer);
    UNREFERENCED_PARAMETER(ExeNameBufferLength);
    return 0;
}

DWORD ApiResponderEmpty::GetConsoleAliasesLengthAImpl(_In_reads_(ExeNameBufferLength) const char* const pExeNameBuffer,
                                                      _In_ ULONG const ExeNameBufferLength,
                                                      _Out_ ULONG* const pAliasesBufferRequired)
{
    UNREFERENCED_PARAMETER(pExeNameBuffer);
    UNREFERENCED_PARAMETER(ExeNameBufferLength);
    *pAliasesBufferRequired = 0;
    return 0;
}

DWORD ApiResponderEmpty::GetConsoleAliasesLengthWImpl(_In_reads_(ExeNameBufferLength) const wchar_t* const pExeNameBuffer,
                                                      _In_ ULONG const ExeNameBufferLength,
                                                      _Out_ ULONG* const pAliasesBufferRequired)
{
    UNREFERENCED_PARAMETER(pExeNameBuffer);
    UNREFERENCED_PARAMETER(ExeNameBufferLength);
    *pAliasesBufferRequired = 0;
    return 0;
}

DWORD ApiResponderEmpty::GetConsoleAliasExesLengthAImpl(_Out_ ULONG* const pAliasExesBufferRequired)
{
    *pAliasExesBufferRequired = 0;
    return 0;
}

DWORD ApiResponderEmpty::GetConsoleAliasExesLengthWImpl(_Out_ ULONG* const pAliasExesBufferRequired)
{
    *pAliasExesBufferRequired = 0;
    return 0;
}

DWORD ApiResponderEmpty::GetConsoleAliasesAImpl(_In_reads_(ExeNameBufferLength) const char* const pExeNameBuffer,
                                                _In_ ULONG const ExeNameBufferLength,
                                                _Out_writes_(AliasBufferLength) char* const pAliasBuffer,
                                                _In_ ULONG const AliasBufferLength)
{
    UNREFERENCED_PARAMETER(pExeNameBuffer);
    UNREFERENCED_PARAMETER(ExeNameBufferLength);
    UNREFERENCED_PARAMETER(pAliasBuffer);
    UNREFERENCED_PARAMETER(AliasBufferLength);
    return 0;
}

DWORD ApiResponderEmpty::GetConsoleAliasesWImpl(_In_reads_(ExeNameBufferLength) const wchar_t* const pExeNameBuffer,
                                                _In_ ULONG const ExeNameBufferLength,
                                                _Out_writes_(AliasBufferLength) wchar_t* const pAliasBuffer,
                                                _In_ ULONG const AliasBufferLength)
{
    UNREFERENCED_PARAMETER(pExeNameBuffer);
    UNREFERENCED_PARAMETER(ExeNameBufferLength);
    UNREFERENCED_PARAMETER(pAliasBuffer);
    UNREFERENCED_PARAMETER(AliasBufferLength);
    return 0;
}

DWORD ApiResponderEmpty::GetConsoleAliasExesAImpl(_Out_writes_(AliasExesBufferLength) char* const pAliasExesBuffer,
                                                  _In_ ULONG const AliasExesBufferLength)
{
    UNREFERENCED_PARAMETER(pAliasExesBuffer);
    UNREFERENCED_PARAMETER(AliasExesBufferLength);
    return 0;
}

DWORD ApiResponderEmpty::GetConsoleAliasExesWImpl(_Out_writes_(AliasExesBufferLength) wchar_t* const pAliasExesBuffer,
                                                  _In_ ULONG const AliasExesBufferLength)
{
    UNREFERENCED_PARAMETER(pAliasExesBuffer);
    UNREFERENCED_PARAMETER(AliasExesBufferLength);
    return 0;
}

DWORD ApiResponderEmpty::GetConsoleWindowImpl(_Out_ HWND* const pHwnd)
{
    *pHwnd = (HWND)INVALID_HANDLE_VALUE; // sure, why not.
    return 0;
}

DWORD ApiResponderEmpty::GetConsoleSelectionInfoImpl(_Out_ CONSOLE_SELECTION_INFO* const pConsoleSelectionInfo)
{
    pConsoleSelectionInfo->dwFlags = 0;
    pConsoleSelectionInfo->dwSelectionAnchor.X = 0;
    pConsoleSelectionInfo->dwSelectionAnchor.Y = 0;
    pConsoleSelectionInfo->srSelection.Top = 0;
    pConsoleSelectionInfo->srSelection.Left = 0;
    pConsoleSelectionInfo->srSelection.Bottom = 0;
    pConsoleSelectionInfo->srSelection.Right = 0;
    return 0;
}

DWORD ApiResponderEmpty::GetConsoleHistoryInfoImpl(_Out_ CONSOLE_HISTORY_INFO* const pConsoleHistoryInfo)
{
    pConsoleHistoryInfo->cbSize = sizeof(CONSOLE_HISTORY_INFO);
    pConsoleHistoryInfo->dwFlags = 0;
    pConsoleHistoryInfo->HistoryBufferSize = 20;
    pConsoleHistoryInfo->NumberOfHistoryBuffers = 5;
    return 0;
}

DWORD ApiResponderEmpty::SetConsoleHistoryInfoImpl(_In_ const CONSOLE_HISTORY_INFO* const pConsoleHistoryInfo)
{
    UNREFERENCED_PARAMETER(pConsoleHistoryInfo);
    return 0;
}

DWORD ApiResponderEmpty::SetCurrentConsoleFontExImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                     _In_ BOOLEAN const IsForMaximumWindowSize,
                                                     _In_ const CONSOLE_FONT_INFOEX* const pConsoleFontInfoEx)
{
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(IsForMaximumWindowSize);
    UNREFERENCED_PARAMETER(pConsoleFontInfoEx);
    return 0;
}
