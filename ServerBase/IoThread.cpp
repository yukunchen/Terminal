#include "stdafx.h"
#include "IoThread.h"

#include "DeviceComm.h"
#include "DeviceProtocol.h"

#include "IApiResponders.h"
#include "ApiResponderEmpty.h"
#include "ApiSorter.h"
#include "Win32Control.h"

IoThread::IoThread(_In_ HANDLE Server) : 
	_Server(Server),
	_Thread(s_IoLoop, this)
{
	_Thread.detach();
}

IoThread::~IoThread()
{
	// detached threads free when they stop running.
}

void IoThread::s_IoLoop(_In_ IoThread* const pIoThread)
{
	pIoThread->IoLoop();
}

void IoThread::IoLoop()
{
	DeviceComm Comm(_Server);
	DeviceProtocol Prot(&Comm);
	ApiResponderEmpty Responder;

	bool fExiting = false;

	while (!fExiting)
	{
		// Attempt to read API call from wire
		CONSOLE_API_MSG ReceiveMsg;
		DWORD Result = Prot.GetApiCall(&ReceiveMsg);

		if (ERROR_PIPE_NOT_CONNECTED == Result)
		{
			fExiting = true;
			continue;
		}
		else if (S_OK != Result)
		{
			DebugBreak();
		}

		// Retrieve additional input/output buffer data if available
		// TODO: determine what to do with errors from here.
		Prot.GetInputBuffer(&ReceiveMsg);
		Prot.GetOutputBuffer(&ReceiveMsg);

		// Route function to handler
		switch (ReceiveMsg.Descriptor.Function)
		{
		case CONSOLE_IO_USER_DEFINED:
		{
			Result = LookupAndDoApiCall(&Responder, &ReceiveMsg);
			break;
		}
		case CONSOLE_IO_CONNECT:
		{
			Result = _IoConnect(&Prot, &ReceiveMsg);
			break;
		}
		case CONSOLE_IO_DISCONNECT:
		{
			// TODO.
			Result = _IoDefault();
			break;
		}
		case CONSOLE_IO_CREATE_OBJECT:
		{
			// TODO.
			Result = _IoDefault();
			break;
		}
		case CONSOLE_IO_CLOSE_OBJECT:
		{
			// TODO.
			Result = _IoDefault();
			break;
		}
		case CONSOLE_IO_RAW_WRITE:
		{
			// TODO.
			Result = _IoDefault();
			break;
		}
		case CONSOLE_IO_RAW_READ:
		{
			Result = DoRawReadCall(&Responder, &ReceiveMsg);
			break;
		}
		case CONSOLE_IO_RAW_FLUSH:
		{
			// TODO.
			Result = _IoDefault();
			break;
		}
		default:
		{
			Result = _IoDefault();
			break;
		}
		}

		// Return whatever status code we got back to the caller.
		ReceiveMsg.SetCompletionStatus(Result);

		// Write reply message signaling API call is complete.
		Prot.CompleteApiCall(&ReceiveMsg);
	}

	ExitProcess(STATUS_SUCCESS);
}

DWORD IoThread::_IoConnect(_In_ DeviceProtocol* Server, _In_ CONSOLE_API_MSG* const pMsg)
{
	HANDLE InputEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
	DWORD result = Server->SetInputAvailableEvent(InputEvent);

	// TODO: These are junk handles for now to various info that we want to get back when other calls come in.
	result = Server->SetConnectionInformation(pMsg, 0x14, 0x18, 0x1a);

	// Notify Win32k that this process is attached to a special console application type.
	// TODO: Don't do this for AttachConsole case (they already are a Win32 app type.)
	Win32Control::NotifyConsoleTypeApplication((HANDLE)pMsg->Descriptor.Process);

	return result;
}

DWORD IoThread::_IoDefault()
{
	return STATUS_UNSUCCESSFUL;
}
