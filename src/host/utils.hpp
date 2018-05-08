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

#include "..\server\ObjectHandle.h"

#define RECT_WIDTH(x) ((x)->right - (x)->left)
#define RECT_HEIGHT(x) ((x)->bottom - (x)->top)

short CalcWindowSizeX(const SMALL_RECT * const pRect);
short CalcWindowSizeY(const SMALL_RECT * const pRect);
short CalcCursorYOffsetInPixels(const short sFontSizeY, const ULONG ulSize);
WORD ConvertStringToDec(_In_ PCWSTR pwchToConvert, _Out_opt_ PCWSTR * const ppwchEnd);

class Utils
{
public:
    static void s_IncrementCoordinate(const COORD bufferSize, COORD& coord);
    static void s_DecrementCoordinate(const COORD bufferSize, COORD& coord);
    static int s_CompareCoords(const COORD bufferSize, const COORD first, const COORD second);

    static bool s_DoDecrementScreenCoordinate(const SMALL_RECT srectEdges, _Inout_ COORD* const pcoordScreen);
    static bool s_DoIncrementScreenCoordinate(const SMALL_RECT srectEdges, _Inout_ COORD* const pcoordScreen);
    static bool s_AddToPosition(const SMALL_RECT srectEdges, const int iAdd, _Inout_ COORD* const pcoordPosition);
    static int s_CompareCoords(const COORD coordFirst, const COORD coordSecond);
    static void s_GetOppositeCorner(const SMALL_RECT srRectangle, const COORD coordCorner, _Out_ COORD* const pcoordOpposite);

    static void s_GetCurrentBufferEdges(_Out_ SMALL_RECT* const psrectEdges);
};
