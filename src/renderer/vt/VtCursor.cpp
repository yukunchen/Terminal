/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "VtCursor.hpp"

#pragma hdrstop

using namespace Microsoft::Console::Render;

VtCursor::VtCursor(IRenderEngine* const pEngine) :
    _coordPosition({-1, -1}),
    _pEngine(pEngine)
{

}

// Method Description:
// - Moves the renderer's cursor.
//      For VT, does effectively nothing, but does store the "real" position of
//       the cursor.
// Arguments:
// - cPos: The new cursor position, in viewport origin character coordinates.
// Return Value:
// - <none>
void VtCursor::Move(_In_ const COORD cPos)
{
    _coordPosition = cPos;   
}

// Method Description:
// - Returns the position of the cursor in viewport origin, character coordinates.
// Arguments:
// - <none>
// Return Value:
// - The cursor position.
COORD VtCursor::GetPosition()
{
    return _coordPosition;
}
