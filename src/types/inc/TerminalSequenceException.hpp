
#include <exception>
#include <stdexcept>
#pragma once

namespace Microsoft::Console::Types
{
    class TerminalSequenceException final : public std::exception
    {
        virtual const char* what() const noexcept {
            return "This should be caught by the state machine";
        }

    };
}
