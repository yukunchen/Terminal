#pragma once

HRESULT ConsoleCreateIoThreadLegacy(_In_ HANDLE Server);
void UseVtPipe(const wchar_t* const pwchVtPipeName);
// void UseVtPipe(const wchar_t* const pwchInVtPipeName, const wchar_t* const pwchOutVtPipeName);
