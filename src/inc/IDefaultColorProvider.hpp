/*++
Copyright (c) Microsoft Corporation

Module Name:
- IDefaultColorProvider.hpp

Abstract:
- Provides an abstraction for aquiring the default colors of a console object.

Author(s): 
- Mike Griese (migrie) 11 Oct 2017
--*/
#pragma once

namespace Microsoft
{
    namespace Console
    {
        class IDefaultColorProvider;
    }
}

class Microsoft::Console::IDefaultColorProvider
{
public:
    virtual ~IDefaultColorProvider() = 0;
    virtual COLORREF GetDefaultForeground() const = 0;
    virtual COLORREF GetDefaultBackground() const = 0;
};

inline Microsoft::Console::IDefaultColorProvider::~IDefaultColorProvider() { }
