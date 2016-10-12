typedef struct _CONSOLE_OBJECT_HEADER
{
    ULONG OpenCount;
    ULONG ReaderCount;
    ULONG WriterCount;
    ULONG ReadShareCount;
    ULONG WriteShareCount;
} CONSOLE_OBJECT_HEADER, *PCONSOLE_OBJECT_HEADER, * const PCCONSOLE_OBJECT_HEADER;
