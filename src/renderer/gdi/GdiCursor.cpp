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
    _coordPosition({-1, -1}),
    _pEngine(pEngine)
{

}

// GdiCursor::~GdiCursor()
// {

// }

// void GdiCursor::ShowCursor()
// {
//     _fIsVisible = true;
//     if(!_fIsBlinking)
//     {
//         _fIsOn = true;
//     }
// }

// void GdiCursor::HideCursor()
// {
//     _fIsVisible = false;

// }

// void GdiCursor::StartBlinking()
// {
//     _fIsBlinking = true;
// }

// void GdiCursor::StopBlinking()
// {

// }

void GdiCursor::Move(_In_ const COORD cPos)
{
    SMALL_RECT sr;

    // Invalidate both the old position and the new one

    sr.Left = _coordPosition.X;
    sr.Right = sr.Left + 1;
    sr.Top = _coordPosition.Y;
    sr.Bottom = sr.Top + 1;
    _pEngine->Invalidate(&sr);

    _coordPosition = cPos;
    
    sr.Left = _coordPosition.X;
    sr.Right = sr.Left + 1;
    sr.Top = _coordPosition.Y;
    sr.Bottom = sr.Top + 1;
    _pEngine->Invalidate(&sr);

    // _pEngine->TriggerRedraw(&_coordPosition);
}

COORD GdiCursor::GetPosition()
{
    return _coordPosition;
}
// void GdiCursor::SetDoubleWide(_In_ const bool fDoubleWide)
// {

// }

// void GdiCursor::SetSize(_In_ const unsigned long ulSize)
// {

// }
