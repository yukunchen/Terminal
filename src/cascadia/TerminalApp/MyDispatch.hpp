#include "../../terminal/adapter/termDispatch.hpp"

class MyDispatch : public Microsoft::Console::VirtualTerminal::TermDispatch
{
  public:
    virtual void Execute(const wchar_t /*wchControl*/) {};
    virtual void Print(const wchar_t /*wchPrintable*/) {};
    virtual void PrintString(const wchar_t *const /*rgwch*/, const size_t /*cch*/) {};
};
