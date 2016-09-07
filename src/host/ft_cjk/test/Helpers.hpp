/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

void DoFailure(PCWSTR pwszFunc, DWORD dwErrorCode);
void GlePattern(PCWSTR pwszFunc);
bool CheckLastErrorNegativeOneFail(DWORD dwReturn, PCWSTR pwszFunc);
bool CheckLastErrorZeroFail(int iValue, PCWSTR pwszFunc);
bool CheckLastErrorWait(DWORD dwReturn, PCWSTR pwszFunc);
bool CheckLastError(HRESULT hr, PCWSTR pwszFunc);
bool CheckLastError(BOOL fSuccess, PCWSTR pwszFunc);
bool CheckLastError(HANDLE handle, PCWSTR pwszFunc);

typedef void(*CJKInnerTest)(HANDLE hOut, HANDLE hIn);
void SetUpExternalCJKTestEnvironment(PCWSTR pwszPathToConsoleBinary, CJKInnerTest pfnTest);
