#include "stdafx.h"
#include "ApiDispatchers.h"

NTSTATUS ApiDispatchers::ServeGetConsoleCursorInfo(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_GETCURSORINFO_MSG* const a = &m->u.consoleMsgL2.GetConsoleCursorInfo;

    void* pHandle = nullptr; // look up handle again based on what's in message

    return pResponders->GetConsoleCursorInfoImpl(pHandle, &a->CursorSize, &a->Visible);
}
