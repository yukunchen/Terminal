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
		CONSOLE_API_MSG ReceiveMsg;
		DWORD Error = Prot.GetReadIo(&ReceiveMsg);

		if (ERROR_PIPE_NOT_CONNECTED == Error)
		{
			fExiting = true;
			continue;
		}
		else if (S_OK != Error)
		{
			DebugBreak();
		}

		// Route function to handler
		switch (ReceiveMsg.Descriptor.Function)
		{
		case CONSOLE_IO_USER_DEFINED:
		{
			LookupAndDoApiCall(&Prot, &Responder, &ReceiveMsg);
			break;
		}
		case CONSOLE_IO_CONNECT:
		{
			_IoConnect(&Prot, &ReceiveMsg.Descriptor);
			break;
		}
		case CONSOLE_IO_DISCONNECT:
		{
			// TODO.
			_IoDefault(&Prot, &ReceiveMsg.Descriptor);
			break;
		}
		case CONSOLE_IO_CREATE_OBJECT:
		{
			// TODO.
			_IoDefault(&Prot, &ReceiveMsg.Descriptor);
			break;
		}
		case CONSOLE_IO_CLOSE_OBJECT:
		{
			// TODO.
			_IoDefault(&Prot, &ReceiveMsg.Descriptor);
			break;
		}
		case CONSOLE_IO_RAW_WRITE:
		{
			// TODO.
			_IoDefault(&Prot, &ReceiveMsg.Descriptor);
			break;
		}
		case CONSOLE_IO_RAW_READ:
		{
			DoRawReadCall(&Prot, &Responder, &ReceiveMsg);
			break;
		}
		case CONSOLE_IO_RAW_FLUSH:
		{
			// TODO.
			_IoDefault(&Prot, &ReceiveMsg.Descriptor);
			break;
		}
		default:
		{
			_IoDefault(&Prot, &ReceiveMsg.Descriptor);
			break;
		}
		}

	}

	ExitProcess(STATUS_SUCCESS);
}

DWORD IoThread::_IoConnect(_In_ DeviceProtocol* Server, _In_ CD_IO_DESCRIPTOR* const pMsg)
{
	HANDLE InputEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
	DWORD result = Server->SetInputAvailableEvent(InputEvent);

	// TODO: These are junk handles for now to various info that we want to get back when other calls come in.
	result = Server->SetConnectionInformation(pMsg, 0x14, 0x18, 0x1a);

	// Notify Win32k that this process is attached to a special console application type.
	// TODO: Don't do this for AttachConsole case (they already are a Win32 app type.)
	Win32Control::NotifyConsoleTypeApplication((HANDLE)pMsg->Process);

	return result;
}

DWORD IoThread::_IoDefault(_In_ DeviceProtocol* Server, _In_ CD_IO_DESCRIPTOR* const pMsg)
{
	return Server->SendCompletion(pMsg, STATUS_UNSUCCESSFUL, nullptr, 0);
}
