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

namespace Microsoft::Console::Types
{
    class Viewport final
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

        static Viewport FromDimensions(_In_ const COORD origin,
                                    _In_ const COORD dimensions);

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
        COORD Dimensions() const;

        bool IsWithinViewport(_In_ const COORD* const pcoord) const;
        bool TrimToViewport(_Inout_ SMALL_RECT* const psr) const;
        void ConvertToOrigin(_Inout_ SMALL_RECT* const psr) const;
        void ConvertToOrigin(_Inout_ COORD* const pcoord) const;
        Viewport ConvertToOrigin(_In_ const Viewport& other) const;
        void ConvertFromOrigin(_Inout_ SMALL_RECT* const psr) const;
        void ConvertFromOrigin(_Inout_ COORD* const pcoord) const;

        SMALL_RECT ToExclusive() const;
        SMALL_RECT ToInclusive() const;

        Viewport ToOrigin() const;

        bool IsValid() const;

        [[nodiscard]]
        static HRESULT AddCoord(_In_ const Viewport& original,
                                _In_ const COORD delta,
                                _Out_ Viewport& modified);
        static Viewport OrViewports(_In_ const Viewport& lhs,
                                    _In_ const Viewport& rhs);

    private:
        // This is always stored as a Inclusive rect.
        SMALL_RECT _sr;

    };
}

inline bool operator==(const Microsoft::Console::Types::Viewport& a,
                       const Microsoft::Console::Types::Viewport& b) noexcept
{
    return a.ToInclusive() == b.ToInclusive();
}

inline bool operator!=(const Microsoft::Console::Types::Viewport& a,
                       const Microsoft::Console::Types::Viewport& b) noexcept
{
    return !(a == b);
}
