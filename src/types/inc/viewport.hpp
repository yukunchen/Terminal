/*++
Copyright (c) Microsoft Corporation

Module Name:
- viewport.hpp

Abstract:
- This method provides an interface for abstracting viewport operations

Author(s):
- Michael Niksa (miniksa) Nov 2015
--*/

#pragma once
#include "../../inc/operators.hpp"

namespace Microsoft
{
    namespace Console
    {
        namespace Types
        {
            class Viewport;
        };
    };
};

class Microsoft::Console::Types::Viewport final
{
public:
    
    Viewport(_In_ const SMALL_RECT sr);
    Viewport(_In_ const Viewport& other);
    ~Viewport();

    static Viewport FromInclusive(_In_ const SMALL_RECT sr);
    
    static Viewport FromExclusive(_In_ const SMALL_RECT sr);

    static Viewport FromDimensions(_In_ const COORD origin,
                                   _In_ const short width,
                                   _In_ const short height);
    static Viewport FromCoord(_In_ const COORD origin);
    
    SHORT Left() const;
    SHORT RightInclusive() const;
    SHORT RightExclusive() const;
    SHORT Top() const;
    SHORT BottomInclusive() const;
    SHORT BottomExclusive() const;
    SHORT Height() const;
    SHORT Width() const;
    COORD Origin() const;

    bool IsWithinViewport(_In_ const COORD* const pcoord) const;
    bool TrimToViewport(_Inout_ SMALL_RECT* const psr) const;
    void ConvertToOrigin(_Inout_ SMALL_RECT* const psr) const;
    void ConvertToOrigin(_Inout_ COORD* const pcoord) const;
    void ConvertFromOrigin(_Inout_ SMALL_RECT* const psr) const;
    void ConvertFromOrigin(_Inout_ COORD* const pcoord) const;

    SMALL_RECT ToExclusive() const;
    SMALL_RECT ToInclusive() const;

    Viewport ToOrigin(_In_ const Viewport& other) const;
    Viewport ToOrigin() const;

private:
    // This is always stored as a Inclusive rect.
    SMALL_RECT _sr;

};
