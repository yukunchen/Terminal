#pragma once

#include "DeviceProtocol.h"
#include "IApiResponders.h"

class IoThread
{
public:
	IoThread(_In_ HANDLE Server);
	~IoThread();

	static void s_IoLoop(_In_ IoThread* const pIoThread);
	void IoLoop();

private:
	DWORD _IoConnect(_In_ DeviceProtocol* Server, _In_ CD_IO_DESCRIPTOR* const pMsg);
	DWORD _IoDefault(_In_ DeviceProtocol* Server, _In_ CD_IO_DESCRIPTOR* const pMsg);

	HANDLE _Server;
	std::thread _Thread;
};

