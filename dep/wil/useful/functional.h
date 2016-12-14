// Windows Internal Libraries (wil)
// Functional.h:  Helpers and classes for dealing with lambdas and callbacks

#pragma once

#include "ResultMacros.h"
#include "wistd_functional.h"

namespace wil
{
    //! Wraps a non-copyable, but movable class for use in a lambda capture specification.
    //! This class derives from the desired class, exposing all public members. It overrides copy construction and transforms
    //! that into move construction of the desired derived class. This allows for a single copy to be made which moves
    //! the contents of the class, rather than actually copying.
    //!
    //! This is a simple stopgap measure that will be obsoleted once C++14's generalized lambda capture expressions
    //! are available to the Windows build.
    //!
    //! Note that some class operators may be hidden as a result of wrapping this class. To wrap a class only immediately
    //! prior to needing move_on_copy you can use make_move_on_copy.
    //! ~~~~
    //! wil::move_on_copy<wil::unique_event> cancel(wil::EventOptions::ManualReset);
    //! AsyncCall([cancel]
    //! {
    //!     // The event's ownership is now safely with the lambda. It cannot be used
    //!     // after the lambda declaration as it was 'moved' into the lambda capture.
    //! });
    //! ~~~~
    //! Note that this mechanism (copy performs a move) isn't intuitive outside of explicit use for this purpose and this mechanism
    //! should otherwise be avoided.
    template <typename T>
    class move_on_copy : public T
    {
    public:
        move_on_copy& operator=(const move_on_copy&) = delete;

        //! Forwarding constructor to allow a wrapped type to be constructed with parameters
        template <typename... args_t>
        explicit move_on_copy(args_t&&... args) :
            T(wistd::forward<args_t>(args)...)
        {
        }

        //! Move constructor from another type T
        move_on_copy(T&& other) :
            T(wistd::move(other))
        {
        }

        //! Specialized copy constructor that moves the underlying type rather than copying it; only one move is allowed
        move_on_copy(const move_on_copy<T>& other) :
            T(wistd::move(const_cast<T&>(static_cast<const T&>(other)))),
            m_copies(other.m_copies + 1)
        {
            WI_USAGE_ASSERT(!other.m_copied, "The variable being accessed has already been copied.");
            WI_USAGE_ASSERT(m_copies <= 2, "move_on_copy<T> being transferred more than twice (once for construction of a lambda, once for copying the lambda).  Possible usage error.");
            other.m_copied = true;
        }

        //! Standard move constructor
        move_on_copy(move_on_copy<T>&& other) :
            T(wistd::move(other)),
            m_copied(other.m_copied)
        {
            // not technically copied, but you shouldn't copy after moving away
            other.m_copied = true;
        }
    
    private:
        mutable bool m_copied = false;
        mutable unsigned char m_copies = 0;
    };

    //! Wraps an existing non-copyable, but movable class for use in a lambda capture specification.
    //! See @ref move_on_copy for full details.
    //! ~~~~
    //! wil::unique_event cancelEvent(wil::EventOptions::ManualReset);
    //! auto cancel = wil::make_move_on_copy(wistd::move(cancelEvent));
    //! AsyncCall([cancel]
    //! {
    //!     // The event's ownership is now safely with the lambda. It cannot be used
    //!     // after the lambda declaration as it was 'moved' into the lambda capture.
    //! });
    //! ~~~~
    template <typename T>
    move_on_copy<T> make_move_on_copy(T&& other)
    {
        return move_on_copy<T>(wistd::move(other));
    }
}