#pragma once

class IApiService
{
public:
    virtual void NotifyInputReadWait() = 0;
    virtual void NotifyOutputWriteWait() = 0;
};