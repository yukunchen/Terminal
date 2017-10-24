/*++
Copyright (c) Microsoft Corporation

Module Name:
- NoOpCursor.hpp

Abstract:
- A RenderCursor that does nothing.
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
            class NoOpCursor;
        }
    }
}

class Microsoft::Console::Render::NoOpCursor : public IRenderCursor
{
public:
    NoOpCursor(){};
    void Move(_In_ const COORD /*cPos*/) override {};
    bool ForcePaint() const override { return false; }
};
