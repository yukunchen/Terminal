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

        Viewport(_In_ const SMALL_RECT sr) noexcept;

        ~Viewport() {}
        Viewport(_In_ const Viewport& other) noexcept;
        Viewport(Viewport&&) = default;
        Viewport& operator=(const Viewport&)& = default;
        Viewport& operator=(Viewport&&)& = default;

        static Viewport FromInclusive(_In_ const SMALL_RECT sr) noexcept;

        static Viewport FromExclusive(_In_ const SMALL_RECT sr) noexcept;

        static Viewport FromDimensions(_In_ const COORD origin,
                                    _In_ const short width,
                                    _In_ const short height) noexcept;

        static Viewport FromDimensions(_In_ const COORD origin,
                                       _In_ const COORD dimensions) noexcept;

        static Viewport FromCoord(_In_ const COORD origin) noexcept;

        SHORT Left() const noexcept;
        SHORT RightInclusive() const noexcept;
        SHORT RightExclusive() const noexcept;
        SHORT Top() const noexcept;
        SHORT BottomInclusive() const noexcept;
        SHORT BottomExclusive() const noexcept;
        SHORT Height() const noexcept;
        SHORT Width() const noexcept;
        COORD Origin() const noexcept;
        COORD Dimensions() const noexcept;

        bool IsWithinViewport(_In_ const COORD* const pcoord) const noexcept;
        bool TrimToViewport(_Inout_ SMALL_RECT* const psr) const noexcept;
        void ConvertToOrigin(_Inout_ SMALL_RECT* const psr) const noexcept;
        void ConvertToOrigin(_Inout_ COORD* const pcoord) const noexcept;
        Viewport ConvertToOrigin(_In_ const Viewport& other) const noexcept;
        void ConvertFromOrigin(_Inout_ SMALL_RECT* const psr) const noexcept;
        void ConvertFromOrigin(_Inout_ COORD* const pcoord) const noexcept;

        SMALL_RECT ToExclusive() const noexcept;
        SMALL_RECT ToInclusive() const noexcept;

        Viewport ToOrigin() const noexcept;

        bool IsValid() const noexcept;

        [[nodiscard]]
        static HRESULT AddCoord(_In_ const Viewport& original,
                                _In_ const COORD delta,
                                _Out_ Viewport& modified);
        static Viewport OrViewports(_In_ const Viewport& lhs,
                                    _In_ const Viewport& rhs) noexcept;

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
