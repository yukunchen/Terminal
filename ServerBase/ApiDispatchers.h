#pragma once

#include "..\ServerBaseApi\IApiResponders.h"
// replace with interface to retrieve additional data
#include "DeviceProtocol.h"

namespace ApiDispatchers
{
#pragma region L1
    NTSTATUS ServeGetConsoleCP(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m);
    NTSTATUS ServeGetConsoleMode(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m);
    NTSTATUS ServeSetConsoleMode(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m);
    NTSTATUS ServeGetNumberOfInputEvents(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m);
    NTSTATUS ServeGetConsoleInput(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m);
    NTSTATUS ServeReadConsole(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m);
    NTSTATUS ServeWriteConsole(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m);
    NTSTATUS ServeGetConsoleLangId(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m);
#pragma endregion

#pragma region L2
    NTSTATUS ServeFillConsoleOutput(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m);
    NTSTATUS ServeGenerateConsoleCtrlEvent(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m);
    NTSTATUS ServeSetConsoleActiveScreenBuffer(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m);
    NTSTATUS ServeFlushConsoleInputBuffer(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m);
    NTSTATUS ServeSetConsoleCP(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m);
    NTSTATUS ServeGetConsoleCursorInfo(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m);
    NTSTATUS ServeSetConsoleCursorInfo(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m);
    NTSTATUS ServeGetConsoleScreenBufferInfo(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m);
    NTSTATUS ServeSetConsoleScreenBufferInfo(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m);
    NTSTATUS ServeSetConsoleScreenBufferSize(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m);
    NTSTATUS ServeSetConsoleCursorPosition(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m);
    NTSTATUS ServeGetLargestConsoleWindowSize(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m);
    NTSTATUS ServeScrollConsoleScreenBuffer(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m);
    NTSTATUS ServeSetConsoleTextAttribute(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m);
    NTSTATUS ServeSetConsoleWindowInfo(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m);
    NTSTATUS ServeReadConsoleOutputString(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m);
    NTSTATUS ServeWriteConsoleInput(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m);
    NTSTATUS ServeWriteConsoleOutput(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m);
    NTSTATUS ServeWriteConsoleOutputString(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m);
    NTSTATUS ServeReadConsoleOutput(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m);
    NTSTATUS ServeGetConsoleTitle(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m);
    NTSTATUS ServeSetConsoleTitle(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m);
#pragma endregion

#pragma region L3
    NTSTATUS ServeGetConsoleMouseInfo(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m);
    NTSTATUS ServeGetConsoleFontSize(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m);
    NTSTATUS ServeGetConsoleCurrentFont(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m);
    NTSTATUS ServeSetConsoleDisplayMode(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m);
    NTSTATUS ServeGetConsoleDisplayMode(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m);
    NTSTATUS ServeAddConsoleAlias(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m);
    NTSTATUS ServeGetConsoleAlias(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m);
    NTSTATUS ServeGetConsoleAliasesLength(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m);
    NTSTATUS ServeGetConsoleAliasExesLength(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m);
    NTSTATUS ServeGetConsoleAliases(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m);
    NTSTATUS ServeGetConsoleAliasExes(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m);
    NTSTATUS ServeGetConsoleWindow(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m);
    NTSTATUS ServeGetConsoleSelectionInfo(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m);
    NTSTATUS ServeGetConsoleProcessList(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m);
    NTSTATUS ServeGetConsoleHistory(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m);
    NTSTATUS ServeSetConsoleHistory(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m);
    NTSTATUS ServeSetConsoleCurrentFont(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m);
#pragma endregion
};

