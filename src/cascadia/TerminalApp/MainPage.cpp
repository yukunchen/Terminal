#include "pch.h"
#include "MainPage.h"
#include "..\..\buffer\out\textBuffer.hpp"
#include "../../renderer/inc/DummyRenderTarget.hpp"

using namespace winrt;
using namespace Windows::UI::Xaml;

namespace winrt::TerminalApp::implementation
{
    MainPage::MainPage()
    {
        InitializeComponent();
        //CHAR_INFO fill;
        //fill.Attributes = 0x7f;
        COORD bufferSize;
        bufferSize.X = 80;
        bufferSize.Y = 9001;
        UINT cursorSize = 12;
        DummyRenderTarget _t;
        TextAttribute attr{ 0x7f };
        TextBuffer b(bufferSize, attr, cursorSize, _t);
        b.Write({ L"F" });
    }

    int32_t MainPage::MyProperty()
    {
        throw hresult_not_implemented();
    }

    void MainPage::MyProperty(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }

    void MainPage::ClickHandler(IInspectable const&, RoutedEventArgs const&)
    {
        myButton().Content(box_value(L"Clicked"));
    }
}
