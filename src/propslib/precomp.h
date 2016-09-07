//
//    Copyright (C) Microsoft.  All rights reserved.
//
#define DEFINE_CONSOLEV2_PROPERTIES
#define INC_OLE2

#define WIN32_NO_STATUS
#include <windows.h>
#undef WIN32_NO_STATUS

// From ntdef.h, but that can't be included or it'll fight over PROBE_ALIGNMENT and other such arch specific defs
typedef _Return_type_success_(return >= 0) LONG NTSTATUS;
/*lint -save -e624 */  // Don't complain about different typedefs.
typedef NTSTATUS *PNTSTATUS;
/*lint -restore */  // Resume checking for different typedefs.
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

#include <ntstatus.h>

#include <shlobj.h>
#include <strsafe.h>
#include <sal.h>

#include <winconp.h>
#include "..\host\settings.hpp"
#include <pathcch.h>

#include "conpropsp.hpp"

#include <wil\resource.h>
#include <wil\result.h>

#pragma region Definitions from DDK (wdm.h)
FORCEINLINE
PSINGLE_LIST_ENTRY
PopEntryList(
    _Inout_ PSINGLE_LIST_ENTRY ListHead
)
{

    PSINGLE_LIST_ENTRY FirstEntry;

    FirstEntry = ListHead->Next;
    if (FirstEntry != NULL) {
        ListHead->Next = FirstEntry->Next;
    }

    return FirstEntry;
}


FORCEINLINE
VOID
PushEntryList(
    _Inout_ PSINGLE_LIST_ENTRY ListHead,
    _Inout_ __drv_aliasesMem PSINGLE_LIST_ENTRY Entry
)

{

    Entry->Next = ListHead->Next;
    ListHead->Next = Entry;
    return;
}
#pragma endregion
