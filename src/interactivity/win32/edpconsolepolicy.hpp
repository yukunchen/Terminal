/*
Module Name:
- EdpConsolePolicy.hpp

Abstract:
- WIP helper in Clipboard::Paste()
- Enterprise Data Protection (EDP) was renamed Windows Data Protection (WIP).
  They refer to the same module.

++*/

#include "precomp.h"
#include "resource.h"

class EdpConsolePolicy final
{
public:
    void EdpAuditClipboard();

    EdpConsolePolicy();

private:
    wil::unique_hmodule _hEdpUtil;
};