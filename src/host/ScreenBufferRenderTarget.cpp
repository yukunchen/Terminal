/********************************************************
 *                                                       *
 *   Copyright (C) Microsoft. All rights reserved.       *
 *                                                       *
 ********************************************************/

#include "precomp.h"
#include "ScreenBufferRenderTarget.hpp"
#include "../interactivity/inc/ServiceLocator.hpp"

ScreenBufferRenderTarget::ScreenBufferRenderTarget(SCREEN_INFORMATION& owner) :
    _owner{ owner }
{
}

void ScreenBufferRenderTarget::TriggerRedraw(const Microsoft::Console::Types::Viewport& region)
{
    auto* const pRenderer = ServiceLocator::LocateGlobals().pRender;
    const auto pActive = &ServiceLocator::LocateGlobals().getConsoleInformation().GetActiveOutputBuffer().GetActiveBuffer();
    if (pRenderer != nullptr && pActive == &_owner)
    {
        pRenderer->TriggerRedraw(region);
    }
}

void ScreenBufferRenderTarget::TriggerRedraw(const COORD* const pcoord)
{
    auto* const pRenderer = ServiceLocator::LocateGlobals().pRender;
    const auto pActive = &ServiceLocator::LocateGlobals().getConsoleInformation().GetActiveOutputBuffer().GetActiveBuffer();
    if (pRenderer != nullptr && pActive == &_owner)
    {
        pRenderer->TriggerRedraw(pcoord);
    }
}

void ScreenBufferRenderTarget::TriggerRedrawCursor(const COORD* const pcoord)
{
    auto* const pRenderer = ServiceLocator::LocateGlobals().pRender;
    const auto pActive = &ServiceLocator::LocateGlobals().getConsoleInformation().GetActiveOutputBuffer().GetActiveBuffer();
    if (pRenderer != nullptr && pActive == &_owner)
    {
        pRenderer->TriggerRedrawCursor(pcoord);
    }
}

void ScreenBufferRenderTarget::TriggerRedrawAll()
{
    auto* const pRenderer = ServiceLocator::LocateGlobals().pRender;
    const auto pActive = &ServiceLocator::LocateGlobals().getConsoleInformation().GetActiveOutputBuffer().GetActiveBuffer();
    if (pRenderer != nullptr && pActive == &_owner)
    {
        pRenderer->TriggerRedrawAll();
    }
}

void ScreenBufferRenderTarget::TriggerTeardown()
{
    auto* const pRenderer = ServiceLocator::LocateGlobals().pRender;
    const auto pActive = &ServiceLocator::LocateGlobals().getConsoleInformation().GetActiveOutputBuffer().GetActiveBuffer();
    if (pRenderer != nullptr && pActive == &_owner)
    {
        pRenderer->TriggerTeardown();
    }
}

void ScreenBufferRenderTarget::TriggerSelection()
{
    auto* const pRenderer = ServiceLocator::LocateGlobals().pRender;
    const auto pActive = &ServiceLocator::LocateGlobals().getConsoleInformation().GetActiveOutputBuffer().GetActiveBuffer();
    if (pRenderer != nullptr && pActive == &_owner)
    {
        pRenderer->TriggerSelection();
    }
}

void ScreenBufferRenderTarget::TriggerScroll()
{
    auto* const pRenderer = ServiceLocator::LocateGlobals().pRender;
    const auto pActive = &ServiceLocator::LocateGlobals().getConsoleInformation().GetActiveOutputBuffer().GetActiveBuffer();
    if (pRenderer != nullptr && pActive == &_owner)
    {
        pRenderer->TriggerScroll();
    }
}

void ScreenBufferRenderTarget::TriggerScroll(const COORD* const pcoordDelta)
{
    auto* const pRenderer = ServiceLocator::LocateGlobals().pRender;
    const auto pActive = &ServiceLocator::LocateGlobals().getConsoleInformation().GetActiveOutputBuffer().GetActiveBuffer();
    if (pRenderer != nullptr && pActive == &_owner)
    {
        pRenderer->TriggerScroll(pcoordDelta);
    }
}

void ScreenBufferRenderTarget::TriggerCircling()
{
    auto* const pRenderer = ServiceLocator::LocateGlobals().pRender;
    const auto pActive = &ServiceLocator::LocateGlobals().getConsoleInformation().GetActiveOutputBuffer().GetActiveBuffer();
    if (pRenderer != nullptr && pActive == &_owner)
    {
        pRenderer->TriggerCircling();
    }
}

void ScreenBufferRenderTarget::TriggerTitleChange()
{
    auto* const pRenderer = ServiceLocator::LocateGlobals().pRender;
    const auto pActive = &ServiceLocator::LocateGlobals().getConsoleInformation().GetActiveOutputBuffer().GetActiveBuffer();
    if (pRenderer != nullptr && pActive == &_owner)
    {
        pRenderer->TriggerTitleChange();
    }
}
