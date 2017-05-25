/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "WexTestClass.h"

#include "UiaTextRange.hpp"
#include "../../../host/textBuffer.hpp"

/*
#ifdef UNIT_TESTING
#error here
#endif
*/

using namespace WEX::Logging;
using namespace WEX::TestExecution;

using namespace Microsoft::Console::Interactivity::Win32;

class DummyElementProvider final : public IRawElementProviderSimple
{
public:
    // IUnknown methods
    IFACEMETHODIMP_(ULONG) AddRef() { return E_NOTIMPL; }
    IFACEMETHODIMP_(ULONG) Release() { return E_NOTIMPL; }
    IFACEMETHODIMP QueryInterface(_In_ REFIID riid,
                                  _COM_Outptr_result_maybenull_ void** ppInterface)
    {
        UNREFERENCED_PARAMETER(riid);
        UNREFERENCED_PARAMETER(ppInterface);
        return E_NOTIMPL;
    };

    // IRawElementProviderSimple methods
    IFACEMETHODIMP get_ProviderOptions(_Out_ ProviderOptions* pOptions)
    {
        UNREFERENCED_PARAMETER(pOptions);
        return E_NOTIMPL;
    }

    IFACEMETHODIMP GetPatternProvider(_In_ PATTERNID iid,
                                        _COM_Outptr_result_maybenull_ IUnknown** ppInterface)
    {
        UNREFERENCED_PARAMETER(iid);
        UNREFERENCED_PARAMETER(ppInterface);
        return E_NOTIMPL;
    }

    IFACEMETHODIMP GetPropertyValue(_In_ PROPERTYID idProp,
                                    _Out_ VARIANT* pVariant)
    {
        UNREFERENCED_PARAMETER(idProp);
        UNREFERENCED_PARAMETER(pVariant);
        return E_NOTIMPL;
    }

    IFACEMETHODIMP get_HostRawElementProvider(_COM_Outptr_result_maybenull_ IRawElementProviderSimple** ppProvider)
    {
        UNREFERENCED_PARAMETER(ppProvider);
        return E_NOTIMPL;
    }
};


class UiaTextRangeTests
{
    TEST_CLASS(UiaTextRangeTests);

    FontInfo* CreateFontInfo()
    {

        COORD coordFontSize;
        coordFontSize.X = 8;
        coordFontSize.Y = 12;
        FontInfo* pRet  = new FontInfo(L"Consolas", 0, 0, coordFontSize, 0);
        return pRet;
    }

    TEXT_BUFFER_INFO* CreateTextBuffer(FontInfo* pFontInfo)
    {
        TEXT_BUFFER_INFO* pTextBuffer;
        COORD screenBufferSize;
        screenBufferSize.X = 100;
        screenBufferSize.Y = 200;
        CHAR_INFO fill;
        fill.Attributes = FOREGROUND_BLUE | FOREGROUND_GREEN | BACKGROUND_RED | BACKGROUND_INTENSITY;
        UINT cursorSize = 12;
        TEXT_BUFFER_INFO::CreateInstance(pFontInfo,
                                         screenBufferSize,
                                         fill,
                                         cursorSize,
                                         &pTextBuffer);
        return pTextBuffer;
    }

    SCREEN_INFORMATION* CreateScreenInfo(FontInfo* pFontInfo)
    {
        COORD windowSize;
        windowSize.X = 200;
        windowSize.Y = 100;
        COORD screenBufferSize;
        screenBufferSize.X = 100;
        screenBufferSize.Y = 200;
        CHAR_INFO fill;
        fill.Attributes = FOREGROUND_BLUE | FOREGROUND_GREEN | BACKGROUND_RED | BACKGROUND_INTENSITY;
        UINT cursorSize = 12;
        SCREEN_INFORMATION* pScreenInfo;
        SCREEN_INFORMATION::CreateInstance(windowSize,
                                           pFontInfo,
                                           screenBufferSize,
                                           fill,
                                           fill,
                                           cursorSize,
                                           &pScreenInfo);
        return pScreenInfo;
    }

    TEST_METHOD(DegenerateRangesDetected)
    {
        // make required dependency objects
        DummyElementProvider dummyProvider;

        FontInfo* pFontInfo = CreateFontInfo();
        auto deleteFontInfo = wil::ScopeExit([&] { delete pFontInfo; });

        TEXT_BUFFER_INFO* pTextBuffer = CreateTextBuffer(pFontInfo);
        auto deleteTextBuffer = wil::ScopeExit([&] { delete pTextBuffer; });

        SCREEN_INFORMATION* pScreenInfo = CreateScreenInfo(pFontInfo);
        auto deleteScreenInfo = wil::ScopeExit([&] { delete pScreenInfo; });

        // make a degenerate range and verify that it reports degenerate
        UiaTextRange degenerate {
            &dummyProvider,
            pTextBuffer,
            pScreenInfo,
            20,
            20
        };
        VERIFY_IS_TRUE(degenerate._isDegenerate());

        // make a non-degenerate range and verify that it reports as such
        UiaTextRange notDegenerate {
            &dummyProvider,
            pTextBuffer,
            pScreenInfo,
            20,
            35
        };
        VERIFY_IS_FALSE(notDegenerate._isDegenerate());
    }

    TEST_METHOD(CanCheckIfLineIsInViewport)
    {
    }

    TEST_METHOD(CanTranslateRowToViewport)
    {
    }

    TEST_METHOD(CanEndpointToRow)
    {
    }

    TEST_METHOD(CanTranslateEndpointToColumn)
    {
    }

    TEST_METHOD(CanTranslateRowToEndpoint)
    {
    }

    TEST_METHOD(CanGetTotalRows)
    {
    }

    TEST_METHOD(CanGetRowWidth)
    {
    }

    TEST_METHOD(CanNormalizeRow)
    {
    }

    TEST_METHOD(CanGetViewportHeight)
    {
    }

    TEST_METHOD(CanGetViewportWidth)
    {
    }
};