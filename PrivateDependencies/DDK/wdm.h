#pragma once

    //
    // ClientId
    //

    typedef struct _CLIENT_ID {
        HANDLE UniqueProcess;
        HANDLE UniqueThread;
    } CLIENT_ID;
    typedef CLIENT_ID *PCLIENT_ID;


#pragma region LIST_ENTRY manipulation
    FORCEINLINE
        VOID
        InitializeListHead(
            _Out_ PLIST_ENTRY ListHead
        )

    {

        ListHead->Flink = ListHead->Blink = ListHead;
        return;
    }

    _Must_inspect_result_
        BOOLEAN
        CFORCEINLINE
        IsListEmpty(
            _In_ const LIST_ENTRY * ListHead
        )

    {

        return (BOOLEAN)(ListHead->Flink == ListHead);
    }

    FORCEINLINE
        VOID
        InsertHeadList(
            _Inout_ PLIST_ENTRY ListHead,
            _Out_ PLIST_ENTRY Entry
        )

    {

        PLIST_ENTRY NextEntry;

        NextEntry = ListHead->Flink;
        if (NextEntry->Blink != ListHead) {
            assert(false);
        }

        Entry->Flink = NextEntry;
        Entry->Blink = ListHead;
        NextEntry->Blink = Entry;
        ListHead->Flink = Entry;
        return;
    }

    FORCEINLINE
        VOID
        InsertTailList(
            _Inout_ PLIST_ENTRY ListHead,
            _Out_ __drv_aliasesMem PLIST_ENTRY Entry
        )
    {

        PLIST_ENTRY PrevEntry;


        PrevEntry = ListHead->Blink;
        if (PrevEntry->Flink != ListHead) {
            assert(false);
        }

        Entry->Flink = ListHead;
        Entry->Blink = PrevEntry;
        PrevEntry->Flink = Entry;
        ListHead->Blink = Entry;
        return;
    }

    FORCEINLINE
        BOOLEAN
        RemoveEntryList(
            _In_ PLIST_ENTRY Entry
        )

    {

        PLIST_ENTRY PrevEntry;
        PLIST_ENTRY NextEntry;

        NextEntry = Entry->Flink;
        PrevEntry = Entry->Blink;
        if ((NextEntry->Blink != Entry) || (PrevEntry->Flink != Entry)) {
            assert(false);
        }

        PrevEntry->Flink = NextEntry;
        NextEntry->Blink = PrevEntry;
        return (BOOLEAN)(PrevEntry == NextEntry);
    }
