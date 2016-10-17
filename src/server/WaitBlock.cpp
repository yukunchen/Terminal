/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "WaitBlock.h"
#include "WaitQueue.h"

_CONSOLE_WAIT_BLOCK::_CONSOLE_WAIT_BLOCK(_In_ ConsoleWaitQueue* const pProcessQueue,
                                         _In_ ConsoleWaitQueue* const pObjectQueue) :
    _pProcessQueue(THROW_HR_IF_NULL(E_INVALIDARG, pProcessQueue)),
    _pObjectQueue(THROW_HR_IF_NULL(E_INVALIDARG, pObjectQueue))
{
    _pProcessQueue->_blocks.push_front(this);
    _itProcessQueue = _pProcessQueue->_blocks.cbegin();

    _pObjectQueue->_blocks.push_front(this);
    _itObjectQueue = _pObjectQueue->_blocks.cbegin();
}

_CONSOLE_WAIT_BLOCK::~_CONSOLE_WAIT_BLOCK()
{
    _pProcessQueue->_blocks.erase(_itProcessQueue);
    _pObjectQueue->_blocks.erase(_itObjectQueue);
}
