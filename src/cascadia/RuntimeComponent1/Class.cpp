#include "pch.h"
#include "Class.h"

namespace winrt::RuntimeComponent1::implementation
{
    int32_t Class::MyProperty()
    {
        return 42;
    }

    void Class::MyProperty(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }
}
