#include "../../terminal/adapter/termDispatch.hpp"
#include "ITerminalApi.hpp"

class TerminalDispatch : public Microsoft::Console::VirtualTerminal::TermDispatch
{
public:
    TerminalDispatch(::Microsoft::Terminal::Core::ITerminalApi& terminalApi);
    virtual ~TerminalDispatch(){};
    virtual void Execute(const wchar_t /*wchControl*/) override;
    virtual void Print(const wchar_t /*wchPrintable*/) override;
    virtual void PrintString(const wchar_t *const /*rgwch*/, const size_t /*cch*/) override;

private:
    ::Microsoft::Terminal::Core::ITerminalApi& _terminalApi;
};
