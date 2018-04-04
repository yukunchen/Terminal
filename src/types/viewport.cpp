/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "inc/Viewport.hpp"

using namespace Microsoft::Console::Types;
    
Viewport::Viewport(_In_ const SMALL_RECT sr) noexcept :
    _sr(sr) 
{

}

Viewport::Viewport(_In_ const Viewport& other) noexcept :
    _sr(other._sr)
{

}

Viewport Viewport::FromInclusive(_In_ const SMALL_RECT sr) noexcept
{
    return Viewport(sr);
}

Viewport Viewport::FromExclusive(_In_ const SMALL_RECT sr) noexcept
{ 
    SMALL_RECT _sr = sr;
    _sr.Bottom -= 1;
    _sr.Right -= 1;
    return Viewport::FromInclusive(_sr);
}

// Function Description:
// - Creates a new Viewport at the given origin, with the given dimensions.
// Arguments:
// - origin: The origin of the new Viewport. Becomes the Viewport's Left, Top
// - width: The width of the new viewport
// - height: The height of the new viewport
// Return Value:
// - a new Viewport at the given origin, with the given dimensions.
Viewport Viewport::FromDimensions(_In_ const COORD origin,
                                  _In_ const short width,
                                  _In_ const short height) noexcept
{ 
    return Viewport::FromExclusive({ origin.X, origin.Y, 
                                     origin.X + width, origin.Y + height });
}

// Function Description:
// - Creates a new Viewport at the given origin, with the given dimensions.
// Arguments:
// - origin: The origin of the new Viewport. Becomes the Viewport's Left, Top
// - dimensions: A coordinate containing the width and height of the new viewport
//      in the x and y coordinates respectively.
// Return Value:
// - a new Viewport at the given origin, with the given dimensions.
Viewport Viewport::FromDimensions(_In_ const COORD origin,
                                  _In_ const COORD dimensions) noexcept
{ 
    return Viewport::FromExclusive({ origin.X, origin.Y, 
                                     origin.X + dimensions.X, origin.Y + dimensions.Y });
}

// Method Description:
// - Creates a Viewport equivalent to a 1x1 rectangle at the given coordinate.
// Arguments:
// - origin: origin of the rectangle to create.
// Return Value:
// - a 1x1 Viewport at the given coordinate
Viewport Viewport::FromCoord(_In_ const COORD origin) noexcept
{ 
    return Viewport::FromInclusive({ origin.X, origin.Y, 
                                     origin.X, origin.Y });
}

SHORT Viewport::Left() const noexcept
{
    return _sr.Left;
}

SHORT Viewport::RightInclusive() const noexcept
{
    return _sr.Right;
}

SHORT Viewport::RightExclusive() const noexcept
{
    return _sr.Right + 1;
}

SHORT Viewport::Top() const noexcept
{
    return _sr.Top;
}

SHORT Viewport::BottomInclusive() const noexcept
{
    return _sr.Bottom;
}

SHORT Viewport::BottomExclusive() const noexcept
{
    return _sr.Bottom + 1;
}

SHORT Viewport::Height() const noexcept
{
    return BottomExclusive() - Top();
}

SHORT Viewport::Width() const noexcept
{
    return RightExclusive() - Left();
}

// Method Description:
// - Get a coord representing the origin of this viewport.
// Arguments:
// - <none>
// Return Value:
// - the coordinates of this viewport's origin.
COORD Viewport::Origin() const noexcept
{
    return { Left(), Top() };
}

// Method Description:
// - Get a coord representing the dimensions of this viewport.
// Arguments:
// - <none>
// Return Value:
// - the dimensions of this viewport.
COORD Viewport::Dimensions() const noexcept
{
    return { Width(), Height() };
}

// Method Description:
// - Returns true if the input coord is within the bounds of this viewport
// Arguments:
// - pcoord: a pointer to the coordinate to check
// Return Value:
// - true iff the coordinate is within the bounds of this viewport.
bool Viewport::IsWithinViewport(_In_ const COORD* const pcoord) const noexcept
{
    return pcoord->X >= Left() && pcoord->X < RightExclusive() &&
           pcoord->Y >= Top() && pcoord->Y < BottomExclusive();
}

// Method Description:
// - Clips the input rectangle to our bounds. Assumes that the input rectangle
//is an exclusive rectangle.
// Arguments:
// - psr: a pointer to an exclusive rect to clip.
// Return Value:
// - true iff the clipped rectangle is valid (with a width and height both >0)
bool Viewport::TrimToViewport(_Inout_ SMALL_RECT* const psr) const noexcept
{
    psr->Left = std::max(psr->Left, Left());
    psr->Right = std::min(psr->Right, RightExclusive());
    psr->Top = std::max(psr->Top, Top());
    psr->Bottom = std::min(psr->Bottom, BottomExclusive());

    return psr->Left < psr->Right && psr->Top < psr->Bottom;
}

// Method Description:
// - Translates the input SMALL_RECT out of our coordinate space, whose origin is
//      at (this.Left, this.Right)
// Arguments:
// - psr: a pointer to a SMALL_RECT the translate into our coordinate space.
// Return Value:
// - <none>
void Viewport::ConvertToOrigin(_Inout_ SMALL_RECT* const psr) const noexcept
{   
    const short dx = Left();
    const short dy = Top();
    psr->Left -= dx;
    psr->Right -= dx;
    psr->Top -= dy;
    psr->Bottom -= dy;
}

// Method Description:
// - Translates the input coordinate out of our coordinate space, whose origin is
//      at (this.Left, this.Right)
// Arguments:
// - pcoord: a pointer to a coordinate the translate into our coordinate space.
// Return Value:
// - <none>
void Viewport::ConvertToOrigin(_Inout_ COORD* const pcoord) const noexcept
{
    pcoord->X -= Left();
    pcoord->Y -= Top();
}

// Method Description:
// - Translates the input SMALL_RECT to our coordinate space, whose origin is
//      at (this.Left, this.Right)
// Arguments:
// - psr: a pointer to a SMALL_RECT the translate into our coordinate space.
// Return Value:
// - <none>
void Viewport::ConvertFromOrigin(_Inout_ SMALL_RECT* const psr) const noexcept
{    
    const short dx = Left();
    const short dy = Top();
    psr->Left += dx;
    psr->Right += dx;
    psr->Top += dy;
    psr->Bottom += dy;
}

// Method Description:
// - Translates the input coordinate to our coordinate space, whose origin is
//      at (this.Left, this.Right)
// Arguments:
// - pcoord: a pointer to a coordinate the translate into our coordinate space.
// Return Value:
// - <none>
void Viewport::ConvertFromOrigin(_Inout_ COORD* const pcoord) const noexcept
{
    pcoord->X += Left();
    pcoord->Y += Top();
}

// Method Description:
// - Returns an exclusive SMALL_RECT equivalent to this viewport.
// Arguments:
// - <none>
// Return Value:
// - an exclusive SMALL_RECT equivalent to this viewport.
SMALL_RECT Viewport::ToExclusive() const noexcept
{
    return { Left(), Top(), RightExclusive(), BottomExclusive() };
}

// Method Description:
// - Returns an inclusive SMALL_RECT equivalent to this viewport.
// Arguments:
// - <none>
// Return Value:
// - an inclusive SMALL_RECT equivalent to this viewport.
SMALL_RECT Viewport::ToInclusive() const noexcept
{
    return { Left(), Top(), RightInclusive(), BottomInclusive() };
}

// Method Description:
// - Returns a new viewport representing this viewport at the origin.
// For example:
//  this = {6, 5, 11, 11} (w, h = 5, 6)
//  result = {0, 0, 5, 6} (w, h = 5, 6)
// Arguments:
// - <none>
// Return Value:
// - a new viewport with the same dimensions as this viewport with top, left = 0, 0
Viewport Viewport::ToOrigin() const noexcept
{
    Viewport returnVal = *this;
    ConvertToOrigin(&returnVal._sr);
    return returnVal;
}

// Method Description:
// - Translates another viewport to this viewport's coordinate space.
// For example:
//  this = {5, 6, 7, 8} (w,h = 1, 1)
//  other = {6, 5, 11, 11} (w, h = 5, 6)
//  result = {1, -1, 6, 5} (w, h = 5, 6)
// Arguments:
// - other: the viewport to convert to this coordinate space
// Return Value:
// - the input viewport in a the coordinate space with origin at (this.Top, this.Left)
Viewport Viewport::ConvertToOrigin(_In_ const Viewport& other) const noexcept
{
    Viewport returnVal = other;
    ConvertToOrigin(&returnVal._sr);
    return returnVal;
}

// Function Description:
// - Translates a given Viewport by the specified coord amount. Does the
//      addition with safemath.
// Arguments:
// - original: The initial viewport to translate. Is unmodified by this operation.
// - delta: The amount to translate the original rect by, in both the x and y coordinates.
// - modified: is set to the result of the addition. Any initial value is discarded.
// Return Value:
// - S_OK if safemath succeeded, otherwise an HR representing the safemath error
[[nodiscard]]
HRESULT Viewport::AddCoord(_In_ const Viewport& original,
                           _In_ const COORD delta,
                           _Out_ Viewport& modified)
{
    SHORT newTop = original._sr.Top;
    SHORT newLeft = original._sr.Left;
    SHORT newRight = original._sr.Right;
    SHORT newBottom = original._sr.Bottom;

    RETURN_IF_FAILED(ShortAdd(newLeft, delta.X, &newLeft));
    RETURN_IF_FAILED(ShortAdd(newRight, delta.X, &newRight));
    RETURN_IF_FAILED(ShortAdd(newTop, delta.Y, &newTop));
    RETURN_IF_FAILED(ShortAdd(newBottom, delta.Y, &newBottom));

    modified._sr = { newLeft, newTop, newRight, newBottom };
    return S_OK;
}

// Function Description:
// - Returns a viewport created from the union of both the parameter viewports.
//      The result extends from the leftmost extent of either rect to the
//      rightmost extent of either rect, and from the lowest top value to the
//      highest bottom value, and everything in between.
// Arguments:
// - lhs: one of the viewports to or together
// - rhs: the other viewport to or together
// Return Value:
// - a Veiwport representing the union of the other two viewports.
Viewport Viewport::OrViewports(_In_ const Viewport& lhs, _In_ const Viewport& rhs) noexcept
{
    const short Left = std::min(lhs._sr.Left, rhs._sr.Left);
    const short Top = std::min(lhs._sr.Top, rhs._sr.Top);
    const short Right = std::max(lhs._sr.Right, rhs._sr.Right);
    const short Bottom = std::max(lhs._sr.Bottom, rhs._sr.Bottom);

    return Viewport({ Left, Top, Right, Bottom });
}

// Method Description:
// - Returns true if the rectangle described by this Viewport has positive dimensions.
// Arguments:
// - <none>
// Return Value:
// - true iff top < bottom && left < right
bool Viewport::IsValid() const noexcept
{
    return _sr.Top < _sr.Bottom && _sr.Left < _sr.Right;
}
