#include "pch.h"
#include "Class.h"

namespace winrt::TerminalControl::implementation
{
    int32_t Class::MyProperty()
    {
        throw hresult_not_implemented();
    }

    void Class::MyProperty(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }

    int32_t Class::DoTheThing()
    {
        return 42;
    }
    void Class::StartTheThing()
    {
        if (_started) return;
        _connection = winrt::TerminalConnection::ConhostConnection(L"cmd.exe", 32, 80);
        _connection.Start();
        _started = true;
    }
    void Class::EndTheThing()
    {
        if (!_started) return;
        _connection.Close();
        _started = false;
    }
}
