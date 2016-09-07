/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#include "precomp.h"

// NOTE THESE TESTS ARE CURRENTLY UNUSED
// THIS IS TO PRESERVE THE KNOWLEDGE ON HOW TO INVOKE THE IME FOR FUTURE USE

class ImeTests
{
    TEST_CLASS(ImeTests);

    TEST_METHOD(TestIme);

private:
    ITfLangBarMgr* _pTfLangBarMgr;
    IAICProxy* _pAICProxy;
};

void SendInputString(const CHAR* const sz)
{
    size_t cch = strlen(sz);

    for (size_t i = 0; i < cch; i++)
    {
        INPUT in;
        in.type = INPUT_KEYBOARD;
        in.ki.wVk = sz[i];
        in.ki.wScan = 0;
        in.ki.dwFlags = 0;
        in.ki.time = 0; // system will provide
        in.ki.dwExtraInfo = GetMessageExtraInfo();
        SendInput(1, &in, sizeof(INPUT));
        Sleep(200);
    }
}

void ImeTests::TestIme()
{
    HRESULT hr;
    hr = ::TF_CreateLangBarMgr(&_pTfLangBarMgr);

    if (CheckLastError(hr, L"TF_CreateLangBarMgr"))
    {
        hr = _pTfLangBarMgr->GetThreadMarshalInterface(::GetCurrentThreadId(), MITYPE_AICPROXY, IID_IAICProxy, (IUnknown**)&_pAICProxy);

        if (CheckLastError(hr, L"GetThreadMarshalInterface"))
        {
            LANGID langidOrig;
            hr = _pAICProxy->GetActiveLanguage(&langidOrig);

            hr = _pAICProxy->ActivateLanguage((LANGID)1041); // set to Japanese
            Sleep(TIMEOUT); // remember to slow it down, it won't change all at once.

            if (CheckLastError(hr, L"ActivateLanguage"))
            {
                BOOL fOpenStatus = TRUE;
                DWORD dwConversionMode = IME_CMODE_NATIVE | IME_CMODE_FULLSHAPE; // set to hiragana
                DWORD dwSentenceMode = 0;

                _pAICProxy->SetImmStatus(&fOpenStatus, &dwConversionMode, &dwSentenceMode);
                Sleep(TIMEOUT);
                SendInputString("GETSUYO");

                Sleep(TIMEOUT);

                dwConversionMode = 0; // set to alphanumeric
                _pAICProxy->SetImmStatus(&fOpenStatus, &dwConversionMode, &dwSentenceMode);
                Sleep(TIMEOUT);

                // 1033 is US langid. Or just save and restore.
                hr = _pAICProxy->ActivateLanguage(langidOrig);
                Sleep(TIMEOUT);
            }

            _pAICProxy->Release();
            _pAICProxy = nullptr;
        }

        _pTfLangBarMgr->Release();
        _pTfLangBarMgr = nullptr;
    }
}
