/*++
Copyright (c) Microsoft Corporation

Module Name:
- MinimalCursor.hpp

Abstract:
- A minimal RenderCursor Implementation.
    Used to implement the IRenderCursor interface for the BGFX and WDDMCon 
    renderers.

Author(s):
- Mike Griese (migrie) 16-Oct-2017
--*/

#pragma once

#include "IRenderCursor.hpp"

namespace Microsoft
{
    namespace Console
    {
        namespace Render
        {
            class MinimalCursor;
        }
    }
}

class Microsoft::Console::Render::MinimalCursor : public IRenderCursor
{
public:
    MinimalCursor() : _coordPosition({0,0}) {};

    // Method Description:
    // - Moves the renderer's cursor.
    // Arguments:
    // - cPos: The new cursor position, in viewport origin character coordinates.
    // Return Value:
    // - <none>
    virtual void Move(_In_ const COORD cPos) override { _coordPosition = cPos; }

    // Method Description:
    // - Returns true if the cursor should always be painted, regardless if it's in
    //       the dirty portion of the screen or not.
    // Arguments:
    // - <none>
    // Return Value:
    // - true if we should manually paint the cursor
    virtual bool ForcePaint() const override { return false; }

    // Method Description:
    // - Returns the position of the cursor in viewport origin, character coordinates.
    // Arguments:
    // - <none>
    // Return Value:
    // - The cursor position.
    virtual COORD GetPosition() const { return _coordPosition; }

    virtual COLORREF GetColor() const override { return _color; };
    virtual void SetColor(_In_ const COLORREF color) override { _color = color; };

    virtual CursorType GetType() const { return _type; };
    virtual void SetType(_In_ const CursorType type) override { _type = type; };

protected:
    COORD _coordPosition;
    COLORREF _color;
    CursorType _type;
};
