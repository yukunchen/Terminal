/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "inc/Viewport.hpp"

using namespace Microsoft::Console::Types;

Viewport::Viewport(const SMALL_RECT sr) noexcept :
    _sr(sr)
{

}

Viewport::Viewport(const Viewport& other) noexcept :
    _sr(other._sr)
{

}

Viewport Viewport::Empty() noexcept
{
    return Viewport({ 0, 0, -1, -1 });
}

Viewport Viewport::FromInclusive(const SMALL_RECT sr) noexcept
{
    return Viewport(sr);
}

Viewport Viewport::FromExclusive(const SMALL_RECT sr) noexcept
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
Viewport Viewport::FromDimensions(const COORD origin,
                                  const short width,
                                  const short height) noexcept
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
Viewport Viewport::FromDimensions(const COORD origin,
                                  const COORD dimensions) noexcept
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
Viewport Viewport::FromCoord(const COORD origin) noexcept
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
// - Determines if the given viewport fits within this viewport.
// Arguments:
// - other - The viewport to fit inside this one
// Return Value:
// - True if it fits. False otherwise.
bool Viewport::IsInBounds(const Viewport& other) const noexcept
{
    return other.Left() >= Left() && other.Left() <= RightInclusive() &&
        other.RightInclusive() >= Left() && other.RightInclusive() <= RightInclusive() &&
        other.Top() >= Top() && other.Top() <= other.BottomInclusive() &&
        other.BottomInclusive() >= Top() && other.BottomInclusive() <= BottomInclusive();
}

// Method Description:
// - Determines if the given coordinate position lies within this viewport.
// Arguments:
// - pos - Coordinate position
// Return Value:
// - True if it lies inside the viewport. False otherwise.
bool Viewport::IsInBounds(const COORD& pos) const noexcept
{
    return pos.X >= Left() && pos.X < RightExclusive() &&
        pos.Y >= Top() && pos.Y < BottomExclusive();
}

// Method Description:
// - Clamps a coordinate position into the inside of this viewport.
// Arguments:
// - pos - coordinate to update/clamp
// Return Value:
// - <none>
void Viewport::Clamp(COORD& pos) const
{
    THROW_HR_IF(E_NOT_VALID_STATE, !IsValid()); // we can't clamp to an invalid viewport.

    pos.X = std::clamp(pos.X, Left(), RightInclusive());
    pos.Y = std::clamp(pos.Y, Top(), BottomInclusive());
}

// Method Description:
// - Moves the coordinate given by the number of positions and
//   in the direction given (repeated increment or decrement)
// Arguments:
// - move - Magnitude and direction of the move
// - pos - The coordinate position to adjust
// Return Value:
// - True if we successfully moved the requested distance. False if we had to stop early.
// - If False, we will restore the original position to the given coordinate.
bool Viewport::MoveInBounds(const ptrdiff_t move, COORD& pos) const noexcept
{
    const auto backup = pos;
    bool success = true; // If nothing happens, we're still successful (e.g. add = 0)

    for (int i = 0; i < move; i++)
    {
        success = IncrementInBounds(pos);

        // If an operation fails, break.
        if (!success)
        {
            break;
        }
    }

    for (int i = 0; i > move; i--)
    {
        success = DecrementInBounds(pos);

        // If an operation fails, break.
        if (!success)
        {
            break;
        }
    }

    // If any operation failed, revert to backed up state.
    if (!success)
    {
        pos = backup;
    }

    return success;
}

// Method Description:
// - Increments the given coordinate within the bounds of this viewport.
// Arguments:
// - pos - Coordinate position that will be incremented, if it can be.
// Return Value:
// - True if it could be incremented. False if it would move outside.
bool Viewport::IncrementInBounds(COORD& pos) const noexcept
{
    auto copy = pos;

    if (IncrementInBoundsCircular(copy))
    {
        pos = copy;
        return true;
    }
    else
    {
        return false;
    }
}

// Method Description:
// - Increments the given coordinate within the bounds of this viewport
//   rotating around to the top when reaching the bottom right corner.
// Arguments:
// - pos - Coordinate position that will be incremented.
// Return Value:
// - True if it could be incremented inside the viewport.
// - False if it rolled over from the bottom right corner back to the top.
bool Viewport::IncrementInBoundsCircular(COORD& pos) const noexcept
{
    // Assert that the position given fits inside this viewport.
    FAIL_FAST_IF(!IsInBounds(pos));

    if (pos.X == RightInclusive())
    {
        pos.Y++;
        pos.X = Left();

        if (pos.Y > BottomInclusive())
        {
            pos.Y = Top(); 
            return false;
        }
    }
    else
    {
        pos.X++;
    }

    return true;
}

// Method Description:
// - Decrements the given coordinate within the bounds of this viewport.
// Arguments:
// - pos - Coordinate position that will be incremented, if it can be.
// Return Value:
// - True if it could be incremented. False if it would move outside.
bool Viewport::DecrementInBounds(COORD& pos) const noexcept
{
    auto copy = pos;
    if (DecrementInBoundsCircular(copy))
    {
        pos = copy;
        return true;
    }
    else
    {
        return false;
    }
}

// Method Description:
// - Decrements the given coordinate within the bounds of this viewport
//   rotating around to the bottom right when reaching the top left corner.
// Arguments:
// - pos - Coordinate position that will be decremented.
// Return Value:
// - True if it could be decremented inside the viewport.
// - False if it rolled over from the top left corner back to the bottom right.
bool Viewport::DecrementInBoundsCircular(COORD& pos) const noexcept
{
    // Assert that the position given fits inside this viewport.
    FAIL_FAST_IF(!IsInBounds(pos));

    if (pos.X == Left())
    {
        pos.Y--;
        pos.X = RightInclusive();

        if (pos.Y < Top())
        {
            pos.Y = BottomInclusive();
            return false;
        }
    }
    else
    {
        pos.X--;
    }

    return true;
}

// Routine Description:
// - Compares two coordinate positions to determine whether they're the same, left, or right within the given buffer size
// Arguments:
// - first- The first coordinate position
// - second - The second coordinate position
// Return Value:
// -  Negative if First is to the left of the Second.
// -  0 if First and Second are the same coordinate.
// -  Positive if First is to the right of the Second.
// -  This is so you can do s_CompareCoords(first, second) <= 0 for "first is left or the same as second".
//    (the < looks like a left arrow :D)
// -  The magnitude of the result is the distance between the two coordinates when typing characters into the buffer (left to right, top to bottom)
int Viewport::CompareInBounds(const COORD& first, const COORD& second) const noexcept
{
    // Assert that our coordinates are within the expected boundaries
    FAIL_FAST_IF(!IsInBounds(first));
    FAIL_FAST_IF(!IsInBounds(second));
    
    // First set the distance vertically
    //   If first is on row 4 and second is on row 6, first will be -2 rows behind second * an 80 character row would be -160.
    //   For the same row, it'll be 0 rows * 80 character width = 0 difference.
    int retVal = (first.Y - second.Y) * Width();

    // Now adjust for horizontal differences
    //   If first is in position 15 and second is in position 30, first is -15 left in relation to 30.
    retVal += (first.X - second.X);

    // Further notes:
    //   If we already moved behind one row, this will help correct for when first is right of second.
    //     For example, with row 4, col 79 and row 5, col 0 as first and second respectively, the distance is -1.
    //     Assume the row width is 80.
    //     Step one will set the retVal as -80 as first is one row behind the second.
    //     Step two will then see that first is 79 - 0 = +79 right of second and add 79
    //     The total is -80 + 79 = -1.
    return retVal;
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
[[nodiscard]]
Viewport Viewport::ConvertToOrigin(const Viewport& other) const noexcept
{
    Viewport returnVal = other;
    ConvertToOrigin(&returnVal._sr);
    return returnVal;
}

// Method Description:
// - Translates another viewport out of this viewport's coordinate space.
// For example:
//  this = {5, 6, 7, 8} (w,h = 1, 1)
//  other = {0, 0, 5, 6} (w, h = 5, 6)
//  result = {5, 6, 10, 12} (w, h = 5, 6)
// Arguments:
// - other: the viewport to convert out of this coordinate space
// Return Value:
// - the input viewport in a the coordinate space with origin at (0, 0)
[[nodiscard]]
Viewport Viewport::ConvertFromOrigin(const Viewport& other) const noexcept
{
    Viewport returnVal = other;
    ConvertFromOrigin(&returnVal._sr);
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
HRESULT Viewport::AddCoord(const Viewport& original,
                           const COORD delta,
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
Viewport Viewport::OrViewports(const Viewport& lhs, const Viewport& rhs) noexcept
{
    const short Left = std::min(lhs._sr.Left, rhs._sr.Left);
    const short Top = std::min(lhs._sr.Top, rhs._sr.Top);
    const short Right = std::max(lhs._sr.Right, rhs._sr.Right);
    const short Bottom = std::max(lhs._sr.Bottom, rhs._sr.Bottom);

    return Viewport({ Left, Top, Right, Bottom });
}

// Method Description:
// - Returns true if the rectangle described by this Viewport has internal space 
// - i.e. it has a positive, non-zero height and width.
bool Viewport::IsValid() const noexcept
{
    return Height() > 0 && Width() > 0;
}
