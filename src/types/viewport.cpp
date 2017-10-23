/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "inc/Viewport.hpp"

using namespace Microsoft::Console::Types;
    
Viewport::Viewport(_In_ const SMALL_RECT sr) :
    _sr(sr) 
{

}

Viewport::Viewport(_In_ const Viewport& other) :
    _sr(other._sr)
{

}

Viewport::~Viewport()
{

}

Viewport Viewport::FromInclusive(_In_ const SMALL_RECT sr)
{
    return Viewport(sr);
}

Viewport Viewport::FromExclusive(_In_ const SMALL_RECT sr)
{ 
    SMALL_RECT _sr = sr;
    _sr.Bottom -= 1;
    _sr.Right -= 1;
    return Viewport::FromInclusive(_sr);
}

Viewport Viewport::FromDimensions(_In_ const COORD origin,
                                  _In_ const short width,
                                  _In_ const short height)
{ 
    return Viewport::FromExclusive({ origin.X, origin.Y, 
                                     origin.X+width, origin.Y+height });
}

SHORT Viewport::Left() const 
{
    return _sr.Left;
}

SHORT Viewport::RightInclusive() const 
{
    return _sr.Right;
}

SHORT Viewport::RightExclusive() const 
{
    return _sr.Right + 1;
}

SHORT Viewport::Top() const 
{
    return _sr.Top;
}

SHORT Viewport::BottomInclusive() const 
{
    return _sr.Bottom;
}

SHORT Viewport::BottomExclusive() const 
{
    return _sr.Bottom + 1;
}

SHORT Viewport::Height() const 
{
    return BottomExclusive() - Top();
}

SHORT Viewport::Width() const 
{
    return RightExclusive() - Left();
}

COORD Viewport::Origin() const 
{
    return { Left(), Top() };
}

bool Viewport::IsWithinViewport(_In_ const COORD* const pcoord) const
{
    return pcoord->X >= Left() && pcoord->X < RightExclusive() &&
        pcoord->Y >= Top() && pcoord->Y < BottomExclusive();
}

bool Viewport::TrimToViewport(_Inout_ SMALL_RECT* const psr) const
{
    psr->Left = max(psr->Left, Left());
    psr->Right = min(psr->Right, RightExclusive());
    psr->Top = max(psr->Top, Top());
    psr->Bottom = min(psr->Bottom, BottomExclusive());

    return psr->Left < psr->Right && psr->Top < psr->Bottom;
}

void Viewport::ConvertToOrigin(_Inout_ SMALL_RECT* const psr) const
{
    psr->Left -= Left();
    psr->Right -= Left();
    psr->Top -= Top();
    psr->Bottom -= Top();
}

void Viewport::ConvertToOrigin(_Inout_ COORD* const pcoord) const
{
    pcoord->X -= Left();
    pcoord->Y -= Top();
}

void Viewport::ConvertFromOrigin(_Inout_ SMALL_RECT* const psr) const
{
    psr->Left += Left();
    psr->Right += Left();
    psr->Top += Top();
    psr->Bottom += Top();
}

void Viewport::ConvertFromOrigin(_Inout_ COORD* const pcoord) const
{
    pcoord->X += Left();
    pcoord->Y += Top();
}

SMALL_RECT Viewport::ToExclusive() const
{
    return { Left(), Top(), RightExclusive(), BottomExclusive() };
}

SMALL_RECT Viewport::ToInclusive() const
{
    return { Left(), Top(), RightInclusive(), BottomInclusive() };
}

Viewport Viewport::ToOrigin(_In_ const Viewport& other) const
{
    Viewport returnVal = other;
    ConvertToOrigin(&returnVal._sr);
    return returnVal;
}

Viewport Viewport::ToOrigin() const
{
    Viewport returnVal = *this;
    ConvertToOrigin(&returnVal._sr);
    return returnVal;
}
