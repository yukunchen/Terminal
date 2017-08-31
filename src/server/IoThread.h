#pragma once

HRESULT ConsoleCreateIoThreadLegacy(_In_ HANDLE Server);

HRESULT UseVtPipe(const std::wstring& InPipeName, const std::wstring& OutPipeName, const std::wstring& VtMode);
