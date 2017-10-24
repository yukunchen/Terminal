/*++
Copyright (c) Microsoft Corporation

Module Name:
- VtCursor.hpp

Abstract:
- A RenderCursor for the VtEngine.

Author(s):
- Mike Griese (migrie) 16-Oct-2017
--*/

#pragma once


#include "..\inc\IRenderCursor.hpp"
#include "..\inc\IRenderEngine.hpp"

namespace Microsoft
{
    namespace Console
    {
        namespace Render
        {
            class VtCursor;
        }
    }
}

class Microsoft::Console::Render::VtCursor : public IRenderCursor
{
public:
    VtCursor(IRenderEngine* const pEngine);
    void Move(_In_ const COORD cPos) override;
    bool ForcePaint() const override;
    
    COORD GetPosition() const;
    bool HasMoved() const;
    void ClearMoved();

private:
    bool _fHasMoved;
    COORD _coordPosition;
    IRenderEngine* const _pEngine;
};
