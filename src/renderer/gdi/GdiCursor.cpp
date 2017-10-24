/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "gdicursor.hpp"

#pragma hdrstop

using namespace Microsoft::Console::Render;

GdiCursor::GdiCursor(IRenderEngine* const pEngine) :
    MinimalCursor(),
    _pEngine(pEngine)
{

}

// Method Description:
// - Moves the renderer's cursor.
//      For GDI, invalidates both the old position of the cursor (where the 
//      inverted cursor was) and the new position.
// Arguments:
// - cPos: The new cursor position, in viewport origin character coordinates.
// Return Value:
// - <none>
void GdiCursor::Move(_In_ const COORD cPos)
{
    SMALL_RECT sr;

    // Invalidate both the old position and the new one

    sr.Left = _coordPosition.X;
    sr.Right = sr.Left + 1;
    sr.Top = _coordPosition.Y;
    sr.Bottom = sr.Top + 1;
    _pEngine->Invalidate(&sr);

    MinimalCursor::Move(cPos);
    
    sr.Left = _coordPosition.X;
    sr.Right = sr.Left + 1;
    sr.Top = _coordPosition.Y;
    sr.Bottom = sr.Top + 1;
    _pEngine->Invalidate(&sr);

}
