#include <windows.h>
#include <vector>
#include <iostream>
#include <cassert>

// Xfer
#include "NetXferInterface.h"

#include "resource.h"

using namespace std;

//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
HINSTANCE               g_hInst = nullptr;
HWND                    g_hMainWnd = nullptr;
HWND                    g_hControlWnd = nullptr;

HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);

// For client
bool g_ServerConnected;
bool ConnectServer();
void DisconnectServer();
void GetServerAddress(wchar_t*, wchar_t*);

NetXferInterface* pHvrNetXfer = nullptr;

typedef struct _POLLING_THREAD_DATA
{
    HANDLE TerminateThreadsEvent;
} POLLING_THREAD_DATA;

DWORD WINAPI PollingThread(_In_ void* Param);
POLLING_THREAD_DATA g_PollingTData;
HANDLE g_PollingTHandle;

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    if (FAILED(InitWindow(hInstance, nCmdShow)))
    {
        return 0;
    }

    g_ServerConnected = false;
    g_PollingTHandle = nullptr;
    RtlZeroMemory(&g_PollingTData, sizeof(g_PollingTData));
    g_PollingTData.TerminateThreadsEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (!g_PollingTData.TerminateThreadsEvent)
    {
        wprintf(L"Fail call to CreateEvent\n");
        return 0;
    }
    ResetEvent(g_PollingTData.TerminateThreadsEvent);

    // Main message loop
    MSG msg = { 0 };
    while (WM_QUIT != msg.message)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            //
        }
    }

    CloseHandle(g_PollingTData.TerminateThreadsEvent);

    return (int)msg.wParam;
}

HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow)
{
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, (LPCTSTR)IDI_APPICON);
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = L"Dx11DemoRemoteWriteWindowClass";
    wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_APPICON);
    if (!RegisterClassEx(&wcex))

    // Create window
    g_hInst = hInstance;
    RECT rc = { 0, 0, 640, 480 };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    g_hMainWnd = CreateWindow(L"Dx11DemoRemoteWriteWindowClass", L"RemoteVR - Remote Renderer", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance,
        nullptr);
    if (!g_hMainWnd)
        return E_FAIL;

    // Create Control Window
    g_hControlWnd = CreateDialog(hInstance, MAKEINTRESOURCE(DLG_CONTROL), NULL, (DLGPROC)DlgProc);
    if (!g_hControlWnd)
    {
        return E_FAIL;
    }

    //ShowWindow(g_hMainWnd, nCmdShow);
    ShowWindow(g_hControlWnd, nCmdShow);
    UpdateWindow(g_hControlWnd);

    return S_OK;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message)
    {
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_SIZE:
        break;

    case WM_MOVE:
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

LRESULT CALLBACK DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_DESTROY:
        if (g_ServerConnected)
        {
            DisconnectServer();
        }
        PostQuitMessage(0);
        break;
    case WM_INITDIALOG:
        wchar_t buf[MAX_PATH];
        RtlZeroMemory(buf, MAX_PATH);
        swprintf_s(buf, MAX_PATH, L"%s:%s", L"127.0.0.1", HVR_NET_XFER_SERVER_PORT);
        SendMessage(GetDlgItem(hWnd, IDC_INPUT_SERVER_ADDR), WM_SETTEXT, (WPARAM)NULL, (LPARAM)buf);
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDCANCEL)
        {
            if (g_ServerConnected)
            {
                DisconnectServer();
            }
            PostQuitMessage(0);
            return (INT_PTR)TRUE;
        }
        else if (LOWORD(wParam) == IDC_BUTTON_CONNECT)
        {
            g_ServerConnected = !g_ServerConnected;
            wchar_t *btnCap = nullptr;
            if (g_ServerConnected)
            {
                SendMessage(GetDlgItem(hWnd, IDC_INPUT_SERVER_ADDR), EM_SETREADONLY, (WPARAM)true, (LPARAM)btnCap);
                btnCap = L"Disconnect";
                ConnectServer();
            }
            else
            {
                SendMessage(GetDlgItem(hWnd, IDC_INPUT_SERVER_ADDR), EM_SETREADONLY, (WPARAM)false, (LPARAM)btnCap);
                btnCap = L"Connect";
                DisconnectServer();
            }
            SendMessage(GetDlgItem(hWnd, IDC_BUTTON_CONNECT), WM_SETTEXT, (WPARAM)NULL, (LPARAM)btnCap);
        }
        break;
    }
    return (INT_PTR)FALSE;
}

bool ConnectServer()
{
    bool isSuccess = true;

    ResetEvent(g_PollingTData.TerminateThreadsEvent);

    pHvrNetXfer = new NetXferInterface();

    DWORD ThreadId;
    g_PollingTHandle = CreateThread(nullptr, 0, PollingThread, &g_PollingTData, 0, &ThreadId);
    if (nullptr == g_PollingTHandle)
    {
        isSuccess = false;
        wprintf(L"Fail to create polling thread");
    }

    return isSuccess;
}

void DisconnectServer()
{
    SetEvent(g_PollingTData.TerminateThreadsEvent);
    if (g_PollingTHandle)
    {
        WaitForMultipleObjectsEx(1, &g_PollingTHandle, TRUE, INFINITE, FALSE);
        CloseHandle(g_PollingTHandle);
    }
}

void GetServerAddress(wchar_t* pServerIP, wchar_t* pServerPort)
{
    if (nullptr != pServerIP && nullptr != pServerPort)
    {
        wchar_t buf[MAX_PATH];
        SendMessage(GetDlgItem(g_hControlWnd, IDC_INPUT_SERVER_ADDR), WM_GETTEXT, MAX_PATH, LPARAM(buf));

        wchar_t* colon = nullptr;
        wchar_t* nextToken = nullptr;
        colon = wcstok_s(buf, L":", &nextToken);
        if (0 == wcslen(colon) || 0 == wcslen(nextToken))
        {
            swprintf_s(pServerIP, MAX_PATH, L"%s", HVR_NET_XFER_SERVER_IP);
            swprintf_s(pServerPort, MAX_PATH, L"%s", HVR_NET_XFER_SERVER_PORT);
        }
        else
        {
            wcscpy_s(pServerIP, MAX_PATH, colon);
            wcscpy_s(pServerPort, MAX_PATH, nextToken);
        }
    }
}

DWORD WINAPI PollingThread(_In_ void* Param)
{
    bool isSuccess = true;
    POLLING_THREAD_DATA* TData = reinterpret_cast<POLLING_THREAD_DATA*>(Param);

    wchar_t bufIP[MAX_PATH];
    wchar_t bufPort[MAX_PATH];
    RtlZeroMemory(bufIP, MAX_PATH);
    RtlZeroMemory(bufPort, MAX_PATH);
    GetServerAddress(bufIP, bufPort);

    if (pHvrNetXfer)
    {
        pHvrNetXfer->InitNetXfer(L"Client", L"TCP", bufIP, bufPort);
    }

    FILE* pFile = nullptr;
    UINT32 frameSize = 0;
    INT64 latency = 0;

    PBYTE pReceiveBuf = (PBYTE)malloc(HVR_NET_XFER_RECV_ELEMENT_MAX_SIZE);
    uint32_t receiveSize = 0;

    bool netXferSuccess = false;
    UINT64 encodeDoneFrmIdx = 0;
    bool connected = false;
    bool infoReceived = false;
    bool fileCreated = false;
    bool streamStart = false;
    bool closeFile = false;

    HVR_NET_XFER_PACKET_HEADER pktHeader;
    while (isSuccess && (WaitForSingleObjectEx(TData->TerminateThreadsEvent, 0, FALSE) == WAIT_TIMEOUT))
    {
        netXferSuccess = pHvrNetXfer->GetPacket(
            &pktHeader,
            pReceiveBuf,
            HVR_NET_XFER_RECV_ELEMENT_MAX_SIZE);
        if (netXferSuccess)
        {
            switch (pktHeader.cmdInfo.s.CmdType)
            {
            case HVR_NET_XFER_PACKET_COMMAND_TYPE_GENERAL:
            {
                if (pktHeader.cmdInfo.s.u.CmdId == HVR_NET_XFER_PACKET_GENERAL_COMMAND_CONNECT)
                {
                    connected = true;
                }
                else if (pktHeader.cmdInfo.s.u.CmdId == HVR_NET_XFER_PACKET_GENERAL_COMMAND_DISCONNECT)
                {
                    connected = false;
                }
                break;
            }
            case HVR_NET_XFER_PACKET_COMMAND_TYPE_ENCODE:
            {
                if (pktHeader.cmdInfo.s.u.CmdId == HVR_NET_XFER_PACKET_ENCODE_COMMAND_H264_INFO)
                {
                    infoReceived = true;
                    if (infoReceived && !fileCreated)
                    {
                        PHVR_NET_XFER_ENCODE_H264_INFO pH264Info = (PHVR_NET_XFER_ENCODE_H264_INFO)pReceiveBuf;

                        SYSTEMTIME curTime = { 0 };
                        GetLocalTime(&curTime);

                        wchar_t buf[MAX_PATH * 2];
                        ZeroMemory(buf, MAX_PATH * 2 * sizeof(wchar_t));
                        GetCurrentDirectory(MAX_PATH * 2, buf);

                        swprintf_s(buf, MAX_PATH * 2, L"%s\\Encoded_%dx%d_%dfps_%04d%02d%02d_%02d%02d%02d.h264",
                            buf,
                            pH264Info->width, pH264Info->height, pH264Info->fps,
                            curTime.wYear, curTime.wMonth, curTime.wDay, curTime.wHour, curTime.wMinute, curTime.wSecond);
                        //pFile = _wfopen(buf, L"w+b");
                        _wfopen_s(&pFile, buf, L"w+b");
                        if (pFile)
                        {
                            fileCreated = true;
                        }
                        else
                        {
                            assert(0);
                        }
                    }
                }
                else if (pktHeader.cmdInfo.s.u.CmdId == HVR_NET_XFER_PACKET_ENCODE_COMMAND_H264_START)
                {
                    streamStart = true;
                }
                else if (pktHeader.cmdInfo.s.u.CmdId == HVR_NET_XFER_PACKET_ENCODE_COMMAND_H264_END)
                {
                    if (fileCreated && streamStart)
                    {
                        if (pFile)
                        {
                            fclose(pFile);
                        }
                        fileCreated = false;
                    }
                    streamStart = false;
                    infoReceived = false;
                }
                else if (pktHeader.cmdInfo.s.u.CmdId == HVR_NET_XFER_PACKET_ENCODE_COMMAND_H264_DATA)
                {
                    if (fileCreated && streamStart && pFile)
                    {
                        PHVR_NET_XFER_ENCODE_H264_DATA_HEADER pH264Header = (PHVR_NET_XFER_ENCODE_H264_DATA_HEADER)pReceiveBuf;
                        fwrite(pReceiveBuf + sizeof(HVR_NET_XFER_ENCODE_H264_DATA_HEADER), 1, pH264Header->frameSize, pFile);
                    }
                }
                break;
            }
            default:
                break;
            }
        }

        //wprintf(L"OK\n");// Debug using console
    }

    if (fileCreated && streamStart)
    {
        if (pFile)
        {
            fclose(pFile);
        }
        fileCreated = false;
    }
    streamStart = false;
    infoReceived = false;

    netXferSuccess = pHvrNetXfer->SendPacket(
        HVR_NET_XFER_PACKET_COMMAND_TYPE_GENERAL,
        HVR_NET_XFER_PACKET_GENERAL_COMMAND_DISCONNECT,
        nullptr,
        0);

    pHvrNetXfer->DestroyNetXfer();

    if (pReceiveBuf)
    {
        free(pReceiveBuf);
    }

    return 0;
}

