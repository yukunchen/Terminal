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

namespace Microsoft::Console
{
    class IIoProvider
    {
    public:
        virtual SCREEN_INFORMATION& GetActiveOutputBuffer() = 0;
        virtual const SCREEN_INFORMATION& GetActiveOutputBuffer() const = 0;
        virtual InputBuffer* const GetActiveInputBuffer() const = 0;
    };
}
