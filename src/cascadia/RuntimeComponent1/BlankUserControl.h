//
// Declaration of the BlankUserControl class.
//

#pragma once

#include "winrt/Windows.UI.Xaml.h"
#include "winrt/Windows.UI.Xaml.Markup.h"
#include "winrt/Windows.UI.Xaml.Interop.h"
#include "BlankUserControl.g.h"

namespace winrt::RuntimeComponent1::implementation
{
    struct BlankUserControl : BlankUserControlT<BlankUserControl>
    {
        BlankUserControl();

        int32_t MyProperty();
        void MyProperty(int32_t value);

        void ClickHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
    };
}

namespace winrt::RuntimeComponent1::factory_implementation
{
    struct BlankUserControl : BlankUserControlT<BlankUserControl, implementation::BlankUserControl>
    {
    };
}
