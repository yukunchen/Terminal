/*++
Copyright (c) Microsoft Corporation

Module Name:
- utils.hpp

Abstract:
- This moduile contains utility math functions that help perform calculations elsewhere in the console

Author(s):
- Paul Campbell (PaulCam)     2014
- Michael Niksa (MiNiksa)     2014
--*/
#pragma once

#include "conapi.h"
#include "server.h"

#define RECT_WIDTH(x) ((x)->right - (x)->left)
#define RECT_HEIGHT(x) ((x)->bottom - (x)->top)

PCONSOLE_PROCESS_HANDLE GetMessageProcess(_In_ PCCONSOLE_API_MSG pMessage);
HANDLE GetMessageObject(_In_ PCCONSOLE_API_MSG pMessage);
void SetReplyStatus(_Inout_ PCCONSOLE_API_MSG pMessage, _In_ const NTSTATUS Status);
void SetReplyInformation(_Inout_ PCCONSOLE_API_MSG pMessage, _In_ ULONG_PTR pInformation);
short CalcWindowSizeX(_In_ const SMALL_RECT * const pRect);
short CalcWindowSizeY(_In_ const SMALL_RECT * const pRect);
short CalcCursorYOffsetInPixels(_In_ short const sFontSizeY, _In_ ULONG const ulSize);
bool IsFlagSet(_In_ DWORD const dwAllFlags, _In_ DWORD const dwFlagToTest);
void SetFlag(_Inout_ DWORD* const pdwAllFlags, _In_ DWORD const dwFlagToSet);
void UnsetFlag(_Inout_ DWORD* const pdwAllFlags, _In_ DWORD const dwFlagToUnset);
void SetFlagBasedOnBool(_In_ bool const fCondition, _Inout_ DWORD* const pdwAllFlags, _In_ DWORD const dwFlag);
WORD ConvertStringToDec(_In_ PCWSTR pwchToConvert, _Out_opt_ PCWSTR * const ppwchEnd);

class Utils
{
public:
    static bool s_DoDecrementScreenCoordinate(_In_ const SMALL_RECT srectEdges, _Inout_ COORD* const pcoordScreen);
    static bool s_DoIncrementScreenCoordinate(_In_ const SMALL_RECT srectEdges, _Inout_ COORD* const pcoordScreen);
    static bool s_AddToPosition(_In_ const SMALL_RECT srectEdges, _In_ const int iAdd, _Inout_ COORD* const pcoordPosition);
    static int s_CompareCoords(_In_ const COORD coordFirst, _In_ const COORD coordSecond);
    static void s_GetOppositeCorner(_In_ const SMALL_RECT srRectangle, _In_ const COORD coordCorner, _Out_ COORD* const pcoordOpposite);

    static void s_GetCurrentBufferEdges(_Out_ SMALL_RECT* const psrectEdges);
};
