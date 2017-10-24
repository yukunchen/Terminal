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
    _fHasMoved = true;  
}

// Method Description:
// - Returns the position of the cursor in viewport origin, character coordinates.
// Arguments:
// - <none>
// Return Value:
// - The cursor position.
COORD VtCursor::GetPosition() const
{
    return _coordPosition;
}

// Method Description:
// - Returns true if the cursor should always be painted, regardless if it's in
//       the dirty portion of the screen or not.
// Arguments:
// - <none>
// Return Value:
// - true if we should manually paint the cursor
bool VtCursor::ForcePaint() const
{
    return HasMoved();
}

// Method Description:
// - Returns true if the cursor has moved since the last call to ClearMoved
// Arguments:
// - <none>
// Return Value:
// - true iff the cursor has moved since the last call to ClearMoved
bool VtCursor::HasMoved() const
{
    return _fHasMoved;
}

// Method Description:
// - Resets the move state of the cursor. This should be called during EndPaint.
// Arguments:
// - <none>
// Return Value:
// - <none>
void VtCursor::ClearMoved()
{
    _fHasMoved = false;
}
