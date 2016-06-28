#pragma once

#include "ObjectHeader.h"

// INTERNAL: This stuff is from the private definitions in the internal conapi.h

#define OffsetToPointer(B,O)  ((PCHAR)( ((PCHAR)(B)) + ((ULONG_PTR)(O))  ))

typedef struct _CONSOLE_API_STATE
{
    ULONG WriteOffset;
    ULONG ReadOffset;
    ULONG InputBufferSize;
    ULONG OutputBufferSize;
    PVOID InputBuffer;
    PVOID OutputBuffer;

    PWCHAR TransBuffer;
    BOOLEAN StackBuffer;
    ULONG WriteFlags; // unused when WriteChars legacy is gone.

} CONSOLE_API_STATE, *PCONSOLE_API_STATE, *const PCCONSOLE_API_STATE;

class CONSOLE_API_MSG
{
public:
    // Contains the outgoing API call response
    CD_IO_COMPLETE Complete;

    // Contains state information used during the servicing of the API call
    CONSOLE_API_STATE State;

    // (probably) no longer used now that we're doing DeviceIoControl instead of NtDeviceIoControlFile
    IO_STATUS_BLOCK IoStatus;

    // Contains the incoming API call data
    CD_IO_DESCRIPTOR Descriptor;

    union
    {
        struct
        {
            CD_CREATE_OBJECT_INFORMATION CreateObject;
            CONSOLE_CREATESCREENBUFFER_MSG CreateScreenBuffer;
        };

        // Used for "user defined" IOCTL section (which is the majority of the console API surface)
        struct
        {
            CONSOLE_MSG_HEADER msgHeader;
            union
            {
                CONSOLE_MSG_BODY_L1 consoleMsgL1;
                CONSOLE_MSG_BODY_L2 consoleMsgL2;
                CONSOLE_MSG_BODY_L3 consoleMsgL3;
            } u;
        };
    };

    bool IsInputBufferAvailable() const;

    bool IsOutputBufferAvailable() const;

    void SetCompletionStatus(_In_ DWORD const Status);

    template <typename T> void GetInputBuffer(_Outptr_ T** const ppBuffer, _Out_ ULONG* const pBufferSize)
    {
        *ppBuffer = static_cast<T*>(State.InputBuffer);
        *pBufferSize = State.InputBufferSize / sizeof(T);
    }

    template <typename T> void GetOutputBuffer(_Outptr_ T** const ppBuffer, _Out_ ULONG* const pBufferSize)
    {
        *ppBuffer = static_cast<T*>(State.OutputBuffer);
        *pBufferSize = State.OutputBufferSize / sizeof(T);
    }

    // Routine Description:
    // - This routine validates a string buffer and returns the pointers of where the strings start within the buffer.
    // Arguments:
    // - Unicode - Supplies a boolean that is TRUE if the buffer contains Unicode strings, FALSE otherwise.
    // - Buffer - Supplies the buffer to be validated.
    // - Size - Supplies the size, in bytes, of the buffer to be validated.
    // - Count - Supplies the expected number of strings in the buffer.
    // ... - Supplies a pair of arguments per expected string. The first one is the expected size, in bytes, of the string
    //       and the second one receives a pointer to where the string starts.
    // Return Value:
    // - TRUE if the buffer is valid, FALSE otherwise.
    template <typename T> DWORD UnpackInputBuffer(_In_ ULONG Count, ...)
    {
        PVOID Buffer = State.InputBuffer;
        ULONG Size = State.InputBufferSize;

        va_list Marker;
        va_start(Marker, Count);

        while (Count > 0)
        {
            ULONG* StringSize = va_arg(Marker, ULONG*);
            T** StringStart = va_arg(Marker, T**);

            // Make sure the string fits in the supplied buffer and that it is properly aligned.
            if (*StringSize > Size)
            {
                break;
            }

            if ((*StringSize % sizeof(T)) != 0)
            {
                break;
            }

            *StringStart = static_cast<T*>(Buffer);

            // Go to the next string.

            Buffer = OffsetToPointer(Buffer, *StringSize);
            Size -= *StringSize;
            Count -= 1;

            // convert bytes to characters now that we're done extracting and walking to the next string by byte count.
            *StringSize /= sizeof(T);
        }

        va_end(Marker);

        return Count == 0 ? STATUS_SUCCESS : STATUS_INVALID_PARAMETER;
    }

    DWORD _GetConsoleObject(_In_ ConsoleObjectType const Type,
                            _In_ ACCESS_MASK AccessRequested,
                            _Out_ IConsoleObject** const ppObject);

    DWORD GetObjectType(_Out_ ConsoleObjectType* pType);

    DWORD GetOutputObject(_In_ ACCESS_MASK AccessRequested, _Out_ IConsoleOutputObject** const ppObject);

    DWORD GetInputObject(_In_ ACCESS_MASK AccessRequested, _Out_ IConsoleInputObject** const ppObject);

};