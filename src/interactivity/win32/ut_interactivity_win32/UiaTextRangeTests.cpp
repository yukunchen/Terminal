/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "WexTestClass.h"
#include "CommonState.hpp"

#include "UiaTextRange.hpp"
#include "../../../host/textBuffer.hpp"

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

    CommonState* _state;

    TEST_METHOD_SETUP(MethodSetup)
    {
        _state = new CommonState();
        _state->PrepareGlobalFont();
        _state->PrepareGlobalScreenBuffer();
        return true;
    }

    TEST_METHOD_CLEANUP(MethodCleanup)
    {
        _state->CleanupGlobalScreenBuffer();
        _state->CleanupGlobalFont();
        delete _state;
        return true;
    }

    TEST_METHOD(DegenerateRangesDetected)
    {
        // make required dependency objects
        DummyElementProvider dummyProvider;
        SCREEN_INFORMATION* pScreenInfo = ServiceLocator::LocateGlobals()->getConsoleInformation()->CurrentScreenBuffer;
        TEXT_BUFFER_INFO* pTextBuffer = pScreenInfo->TextInfo;

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