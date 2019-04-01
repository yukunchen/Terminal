#pragma once

#include "stdafx.h"
#include "..\..\src\renderer\inc\IRenderTarget.hpp"

class NullRenderTarget : public Microsoft::Console::Render::IRenderTarget
{
public:
    virtual ~NullRenderTarget() {};

    virtual void TriggerRedraw(const Microsoft::Console::Types::Viewport& /*region*/) {};
    virtual void TriggerRedraw(const COORD* const /*pcoord*/) {};
    virtual void TriggerRedrawCursor(const COORD* const /*pcoord*/) {};

    virtual void TriggerRedrawAll() {};
    virtual void TriggerTeardown() {};

    virtual void TriggerSelection() {};
    virtual void TriggerScroll() {};
    virtual void TriggerScroll(const COORD* const /*pcoordDelta*/) {};
    virtual void TriggerCircling() {};
    virtual void TriggerTitleChange() {};
};