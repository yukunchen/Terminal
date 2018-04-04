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

        Viewport(const SMALL_RECT sr) noexcept;
        
        ~Viewport() {}
        Viewport(const Viewport& other) noexcept;
        Viewport(Viewport&&) = default;
        Viewport& operator=(const Viewport&)& = default;
        Viewport& operator=(Viewport&&)& = default;
                
        static Viewport FromInclusive(const SMALL_RECT sr) noexcept;

        static Viewport FromExclusive(const SMALL_RECT sr) noexcept;

        static Viewport FromDimensions(const COORD origin,
                                    const short width,
                                    const short height) noexcept;

        static Viewport FromDimensions(const COORD origin,
                                       const COORD dimensions) noexcept;
        
        static Viewport FromCoord(const COORD origin) noexcept;

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

        bool IsWithinViewport(const COORD* const pcoord) const noexcept;
        bool TrimToViewport(_Inout_ SMALL_RECT* const psr) const noexcept;
        void ConvertToOrigin(_Inout_ SMALL_RECT* const psr) const noexcept;
        void ConvertToOrigin(_Inout_ COORD* const pcoord) const noexcept;
        Viewport ConvertToOrigin(const Viewport& other) const noexcept;
        void ConvertFromOrigin(_Inout_ SMALL_RECT* const psr) const noexcept;
        void ConvertFromOrigin(_Inout_ COORD* const pcoord) const noexcept;

        SMALL_RECT ToExclusive() const noexcept;
        SMALL_RECT ToInclusive() const noexcept;

        Viewport ToOrigin() const noexcept;

        bool IsValid() const;

        [[nodiscard]]
        static HRESULT AddCoord(const Viewport& original,
                                const COORD delta,
                                _Out_ Viewport& modified);
        static Viewport OrViewports(const Viewport& lhs,
                                    const Viewport& rhs) noexcept;

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
