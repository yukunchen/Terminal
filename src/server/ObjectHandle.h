class INPUT_READ_HANDLE_DATA;

typedef struct _CONSOLE_HANDLE_DATA
{
    ULONG HandleType;
    ACCESS_MASK Access;
    ULONG ShareAccess;
    PVOID ClientPointer; // This will be a pointer to a SCREEN_INFORMATION or INPUT_INFORMATION object.
    INPUT_READ_HANDLE_DATA * pClientInput;
} CONSOLE_HANDLE_DATA, *PCONSOLE_HANDLE_DATA;
