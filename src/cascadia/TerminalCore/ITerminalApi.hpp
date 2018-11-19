#pragma once
namespace Microsoft::Terminal::Core
{
    class ITerminalApi
    {
    public:
        virtual ~ITerminalApi() {}
        virtual bool PrintString(std::wstring_view stringView) = 0;
        virtual bool ExecuteChar(wchar_t wch) = 0;
    };
}
