//#pragma once
//
//using namespace std;
//
//extern "C"
//{
//#include <DDK\wdm.h>
//}
//
//#include "ObjectHandle.h"
//
//class ConsoleProcessHandle
//{
//public:
//
//	static DWORD CreateHandle(_In_ DWORD ProcessId,
//							  _In_ DWORD ThreadId,
//							  _In_ ConsoleHandleData* const pInputHandle,
//							  _In_ ConsoleHandleData* const pOutputHandle,
//							  _Out_ ConsoleProcessHandle** const ppProcessHandle);
//
//	~ConsoleProcessHandle();
//
//private:
//
//	ConsoleProcessHandle();
//
//	//LIST_ENTRY ListLink;
//	/*HANDLE ProcessHandle;
//	ULONG TerminateCount;
//	ULONG ProcessGroupId;
//	CLIENT_ID ClientId;*/
//
//	DWORD ProcessId;
//	DWORD ThreadId;
//
//	BOOL RootProcess;
//	//LIST_ENTRY WaitBlockQueue;
//	ConsoleHandleData* const InputHandle;
//	ConsoleHandleData* const OutputHandle;
//
//	static std::vector<ConsoleProcessHandle*> ConsoleProcessHandle::s_KnownProcesses;
//};
//
