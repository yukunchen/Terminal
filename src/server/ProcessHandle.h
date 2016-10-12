typedef struct _CONSOLE_PROCESS_HANDLE
{
    LIST_ENTRY ListLink;
    HANDLE ProcessHandle;
    ULONG TerminateCount;
    ULONG ProcessGroupId;
    CLIENT_ID ClientId;
    BOOL RootProcess;
    LIST_ENTRY WaitBlockQueue;
    HANDLE InputHandle;
    HANDLE OutputHandle;
} CONSOLE_PROCESS_HANDLE, *PCONSOLE_PROCESS_HANDLE;
