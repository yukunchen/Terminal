#include "pch.h"
#include "MainPage.h"

using namespace winrt;
using namespace Windows::UI::Xaml;

namespace winrt::TerminalApp::implementation
{
    MainPage::MainPage() :
        _renderTarget{}
    {
        InitializeComponent();
        COORD bufferSize { 80, 9001 };
        UINT cursorSize = 12;
        TextAttribute attr{ 0x7f };
        _buffer = std::make_unique<TextBuffer>(bufferSize, attr, cursorSize, _renderTarget);
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
        auto cursorX = _buffer->GetCursor().GetPosition().X;

        const auto burrito = L"🌯";
        OutputCellIterator burriter{ burrito };

        _buffer->Write({ L"F" });
        _buffer->IncrementCursor();

        // DebugBreak();
        // _buffer->Write(burriter);
        // _buffer->IncrementCursor();

        auto textIter = _buffer->GetTextDataAt({0, 0});

        //if (_buffer->GetCursor().GetPosition().X > 1) DebugBreak();

        std::wstring wstr = L"";
        //std::wstring wstr(*textIter);

         for (int x = 0; x < cursorX; x++)
         {
             auto view = *textIter;
             wstr += view;
             textIter++;
         }
        hstring hstr{ wstr };
        // myButton().Content(box_value(L"Clicked"));
        myButton().Content(box_value(hstr));
    }
}
