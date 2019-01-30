#include "pch.h"
#include "BlankUserControl.h"

using namespace winrt;
using namespace Windows::UI::Xaml;

namespace winrt::RuntimeComponent1::implementation
{
    BlankUserControl::BlankUserControl()
    {
        InitializeComponent();
    }

    int32_t BlankUserControl::MyProperty()
    {
        throw hresult_not_implemented();
    }

    void BlankUserControl::MyProperty(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }

    void BlankUserControl::ClickHandler(IInspectable const&, RoutedEventArgs const&)
    {
        Button().Content(box_value(L"Clicked"));
    }
}
