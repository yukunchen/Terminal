typedef BOOL(*CONSOLE_WAIT_ROUTINE) (_In_ PLIST_ENTRY pWaitQueue,
                                     _In_ PCONSOLE_API_MSG pWaitReplyMessage,
                                     _In_ PVOID pvWaitParameter,
                                     _In_ PVOID pvSatisfyParameter,
                                     _In_ BOOL fThreadDying);

typedef struct _CONSOLE_WAIT_BLOCK
{
    LIST_ENTRY Link;
    LIST_ENTRY ProcessLink;
    PVOID WaitParameter;
    CONSOLE_WAIT_ROUTINE WaitRoutine;
    CONSOLE_API_MSG WaitReplyMessage;
} CONSOLE_WAIT_BLOCK, *PCONSOLE_WAIT_BLOCK;
