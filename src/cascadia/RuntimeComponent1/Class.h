#pragma once

#include "Class.g.h"
#include <winrt/TerminalConnection.h>

namespace winrt::RuntimeComponent1::implementation
{
    struct Class : ClassT<Class>
    {
        Class() = default;

        int32_t MyProperty();
        void MyProperty(int32_t value);

        int32_t DoTheThing();
        void StartTheThing();
        void EndTheThing();
    private:
        winrt::TerminalConnection::ITerminalConnection _connection;
        bool _started = false;
    };
}

namespace winrt::RuntimeComponent1::factory_implementation
{
    struct Class : ClassT<Class, implementation::Class>
    {
    };
}
