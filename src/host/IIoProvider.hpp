/*++
Copyright (c) Microsoft Corporation

Module Name:
- IIoProvider.hpp

Abstract:
- Provides an abstraction for aquiring the active input and output objects of 
    the console. 

Author(s): 
- Mike Griese (migrie) 11 Oct 2017
--*/
#pragma once

#include "screenInfo.hpp"
#include "inputBuffer.hpp"

namespace Microsoft
{
    namespace Console
    {
        class IIoProvider;
    }
}

class Microsoft::Console::IIoProvider
{
public:
    virtual SCREEN_INFORMATION* const GetActiveOutputBuffer() const = 0;
    virtual InputBuffer* const GetActiveInputBuffer() const = 0;
};