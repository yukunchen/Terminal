#pragma once

#include "ApiMessage.h"

class WaitQueue
{
public:
    WaitQueue();
    ~WaitQueue();

    void SetReason(_In_ DWORD reasons)
    {
        _WaitReasons |= reasons;
    }

    void ClearReason(_In_ DWORD reasons)
    {
        _WaitReasons &= ~reasons;

        if (_WaitReasons == 0)
        {
            _NotifyQueuedBlocks();
        }
    }

private:
    DWORD _WaitReasons;
    queue<CONSOLE_API_MSG*> _Blocks;

    void _NotifyQueuedBlocks()
    {
        while (!_Blocks.empty())
        {
            CONSOLE_API_MSG* pMsg = _Blocks.front();




            _Blocks.pop();
        }
    }
};

