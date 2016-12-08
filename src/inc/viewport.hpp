/*++
Copyright (c) Microsoft Corporation

Module Name:
- viewport.hpp

Abstract:
- This method provides an interface for abstracting viewport operations

Author(s):
- Michael Niksa (miniksa) Nov 2015
--*/

#pragma once

class Viewport sealed
{
public:
    Viewport(_In_ const SMALL_RECT sr) : _sr(sr) {}
    ~Viewport() {}

    SHORT Left() const { return _sr.Left; }
    SHORT RightInclusive() const { return _sr.Right; }
    SHORT RightExclusive() const { return _sr.Right + 1; }
    SHORT Top() const { return _sr.Top; }
    SHORT BottomInclusive() const { return _sr.Bottom; }
    SHORT BottomExclusive() const { return _sr.Bottom + 1; }
    SHORT Height() const { return BottomExclusive() - Top(); }
    SHORT Width() const { return RightExclusive() - Left() ; }

    bool IsWithinViewport(_In_ const COORD* const pcoord) const
    {
        return pcoord->X >= Left() && pcoord->X < RightExclusive() &&
            pcoord->Y >= Top() && pcoord->Y < BottomExclusive();
    }

    bool TrimToViewport(_Inout_ SMALL_RECT* const psr) const
    {
        psr->Left = max(psr->Left, Left());
        psr->Right = min(psr->Right, RightExclusive());
        psr->Top = max(psr->Top, Top());
        psr->Bottom = min(psr->Bottom, BottomExclusive());

        return psr->Left < psr->Right && psr->Top < psr->Bottom;
    }

    void ConvertToOrigin(_Inout_ SMALL_RECT* const psr) const
    {
        psr->Left -= Left();
        psr->Right -= Left();
        psr->Top -= Top();
        psr->Bottom -= Top();
    }

    void ConvertToOrigin(_Inout_ COORD* const pcoord) const
    {
        pcoord->X -= Left();
        pcoord->Y -= Top();
    }

    void ConvertFromOrigin(_Inout_ SMALL_RECT* const psr) const
    {
        psr->Left += Left();
        psr->Right += Left();
        psr->Top += Top();
        psr->Bottom += Top();
    }

    void ConvertFromOrigin(_Inout_ COORD* const pcoord) const
    {
        pcoord->X += Left();
        pcoord->Y += Top();
    }

private:
    SMALL_RECT _sr;

};
