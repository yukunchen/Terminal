#pragma once
#include "stdafx.h"
#include "InputBuffer.h"
#include "OutputBuffer.h"
//#include "DriverResponder.h"

typedef long NTSTATUS;
class DriverResponder; // Forward decl

class ConsoleHost
{
public:
    ConsoleHost();
    ~ConsoleHost();
    NTSTATUS CreateIOBuffers(_Out_ IConsoleInputObject** const ppInputObject,
                             _Out_ IConsoleOutputObject** const ppOutputObject);

    void CreateWindowThread();

    DriverResponder* GetApiResponder();

	void HandleKeyEvent(_In_ const HWND hWnd,
		_In_ const UINT Message,
		_In_ const WPARAM wParam,
		_In_ const LPARAM lParam,
		_Inout_opt_ PBOOL pfUnlockConsole);
private:
    thread* _pWindowThread;

    InputBuffer* _pInputBuffer;
    OutputBuffer* _pOutputBuffer;
    
    DriverResponder* _pDriverResponder; // Needed so window proc can notify on input
    
    //int WINAPI _GoWindow();
	//LRESULT CALLBACK _WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);


};