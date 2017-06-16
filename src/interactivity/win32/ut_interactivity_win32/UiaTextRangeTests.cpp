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

using namespace WEX::Common;
using namespace WEX::Logging;
using namespace WEX::TestExecution;

using namespace Microsoft::Console::Interactivity::Win32;


// UiaTextRange takes an object that implements
// IRawElementProviderSimple as a constructor argument. Making a real
// one would involve setting up the window which we don't want to do
// for unit tests so instead we'll use this one. We don't care about
// it not doing anything for its implementation because it is not used
// during the unit tests below.
class DummyElementProvider final : public IRawElementProviderSimple
{
public:
    // IUnknown methods
    IFACEMETHODIMP_(ULONG) AddRef() { return 1; }
    IFACEMETHODIMP_(ULONG) Release() { return 1; }
    IFACEMETHODIMP QueryInterface(_In_ REFIID /*riid*/,
                                  _COM_Outptr_result_maybenull_ void** /*ppInterface*/)
    {
        return E_NOTIMPL;
    };

    // IRawElementProviderSimple methods
    IFACEMETHODIMP get_ProviderOptions(_Out_ ProviderOptions* /*pOptions*/)
    {
        return E_NOTIMPL;
    }

    IFACEMETHODIMP GetPatternProvider(_In_ PATTERNID /*iid*/,
                                      _COM_Outptr_result_maybenull_ IUnknown** /*ppInterface*/)
    {
        return E_NOTIMPL;
    }

    IFACEMETHODIMP GetPropertyValue(_In_ PROPERTYID /*idProp*/,
                                    _Out_ VARIANT* /*pVariant*/)
    {
        return E_NOTIMPL;
    }

    IFACEMETHODIMP get_HostRawElementProvider(_COM_Outptr_result_maybenull_ IRawElementProviderSimple** /*ppProvider*/)
    {
        return E_NOTIMPL;
    }
};


class UiaTextRangeTests
{
    TEST_CLASS(UiaTextRangeTests);

    CommonState* _state;
    DummyElementProvider _dummyProvider;
    SCREEN_INFORMATION* _pScreenInfo;
    TEXT_BUFFER_INFO* _pTextBuffer;
    UiaTextRange* _range;

    TEST_METHOD_SETUP(MethodSetup)
    {
        // set up common state
        _state = new CommonState();
        _state->PrepareGlobalFont();
        _state->PrepareGlobalScreenBuffer();
        _state->PrepareNewTextBufferInfo();

        // set up pointers
        _pScreenInfo = ServiceLocator::LocateGlobals()->getConsoleInformation()->CurrentScreenBuffer;
        _pTextBuffer = _pScreenInfo->TextInfo;

        // set up default range
        _range = new UiaTextRange
        {
            &_dummyProvider,
            0,
            0
        };

        return true;
    }

    TEST_METHOD_CLEANUP(MethodCleanup)
    {
        _state->CleanupNewTextBufferInfo();
        _state->CleanupGlobalScreenBuffer();
        _state->CleanupGlobalFont();
        delete _state;
        delete _range;

        _pScreenInfo = nullptr;
        _pTextBuffer = nullptr;
        return true;
    }

    const unsigned int _getRowWidth() const
    {
        const CHAR_ROW charRow = _pTextBuffer->GetFirstRow()->CharRow;
        return charRow.Left - charRow.Right;
    }

    TEST_METHOD(DegenerateRangesDetected)
    {
        // make a degenerate range and verify that it reports degenerate
        UiaTextRange degenerate
        {
            &_dummyProvider,
            20,
            19
        };
        VERIFY_IS_TRUE(degenerate.IsDegenerate());
        VERIFY_ARE_EQUAL(0u, degenerate._rowCountInRange());

        // make a non-degenerate range and verify that it reports as such
        UiaTextRange notDegenerate1
        {
            &_dummyProvider,
            20,
            20
        };
        VERIFY_IS_FALSE(notDegenerate1.IsDegenerate());
        VERIFY_ARE_EQUAL(1u, notDegenerate1._rowCountInRange());

        UiaTextRange notDegenerate2
        {
            &_dummyProvider,
            20,
            35
        };
        VERIFY_IS_FALSE(notDegenerate2.IsDegenerate());
        VERIFY_ARE_EQUAL(1u, notDegenerate2._rowCountInRange());
    }

    TEST_METHOD(CanCheckIfScreenInfoRowIsInViewport)
    {
        // check a viewport that's one line tall
        Viewport viewport;
        viewport.Top = 0;
        viewport.Bottom = 0;

        VERIFY_IS_TRUE(_range->_isScreenInfoRowInViewport(0, viewport));
        VERIFY_IS_FALSE(_range->_isScreenInfoRowInViewport(1, viewport));

        // check a slightly larger viewport
        viewport.Bottom = 5;
        for (auto i = 0; i <= viewport.Bottom; ++i)
        {
            VERIFY_IS_TRUE(_range->_isScreenInfoRowInViewport(i, viewport),
                           NoThrowString().Format(L"%d should be in viewport", i));
        }
        VERIFY_IS_FALSE(_range->_isScreenInfoRowInViewport(viewport.Bottom + 1, viewport));
    }

    TEST_METHOD(CanTranslateScreenInfoRowToViewport)
    {
        const int totalRows = _pTextBuffer->TotalRowCount();

        Viewport viewport;
        viewport.Top = 0;
        viewport.Bottom = 10;


        std::vector<std::pair<int, int>> viewportSizes =
        {
            { 0, 10 }, // viewport at top
            { 2, 10 }, // shifted viewport
            { totalRows - 5, totalRows + 3 } // viewport with 0th row
        };

        for (auto it = viewportSizes.begin(); it != viewportSizes.end(); ++it)
        {
            viewport.Top = static_cast<SHORT>(it->first);
            viewport.Bottom = static_cast<SHORT>(it->second);
            for (int i = viewport.Top; _range->_isScreenInfoRowInViewport(i, viewport); ++i)
            {
                VERIFY_ARE_EQUAL(i - viewport.Top, _range->_screenInfoRowToViewportRow(i, viewport));
            }
        }

        // ScreenInfoRows that are above the viewport return a
        // negative value
        viewport.Top = 5;
        viewport.Bottom = 10;

        VERIFY_ARE_EQUAL(-1, _range->_screenInfoRowToViewportRow(4, viewport));
        VERIFY_ARE_EQUAL(-2, _range->_screenInfoRowToViewportRow(3, viewport));
    }

    TEST_METHOD(CanTranslateEndpointToTextBufferRow)
    {
        const auto rowWidth = _getRowWidth();
        for (auto i = 0; i < 300; ++i)
        {
            VERIFY_ARE_EQUAL(i / rowWidth, _range->_endpointToTextBufferRow(i));
        }
    }


    TEST_METHOD(CanTranslateTextBufferRowToEndpoint)
    {
        const auto rowWidth = _getRowWidth();
        for (unsigned int i = 0; i < 5; ++i)
        {
            VERIFY_ARE_EQUAL(i * rowWidth, _range->_textBufferRowToEndpoint(i));
            // make sure that the translation is reversible
            VERIFY_ARE_EQUAL(i , _range->_endpointToTextBufferRow(_range->_textBufferRowToEndpoint(i)));
        }
    }

    TEST_METHOD(CanTranslateTextBufferRowToScreenInfoRow)
    {
        const auto rowWidth = _getRowWidth();
        for (unsigned int i = 0; i < 5; ++i)
        {
            VERIFY_ARE_EQUAL(i , _range->_textBufferRowToScreenInfoRow(_range->_screenInfoRowToTextBufferRow(i)));
        }
    }

    TEST_METHOD(CanTranslateEndpointToColumn)
    {
        const auto rowWidth = _getRowWidth();
        for (auto i = 0; i < 300; ++i)
        {
            const auto column = i % rowWidth;
            VERIFY_ARE_EQUAL(column, _range->_endpointToColumn(i));
        }
    }

    TEST_METHOD(CanGetTotalRows)
    {
        const auto totalRows = _pTextBuffer->TotalRowCount();
        VERIFY_ARE_EQUAL(totalRows,
                         _range->_getTotalRows());
    }

    TEST_METHOD(CanGetRowWidth)
    {
        const auto rowWidth = _getRowWidth();
        VERIFY_ARE_EQUAL(rowWidth, _range->_getRowWidth());
    }

    TEST_METHOD(CanNormalizeRow)
    {
        const int totalRows = _pTextBuffer->TotalRowCount();
        std::vector<std::pair<unsigned int, unsigned int>> rowMappings =
        {
            { 0, 0 },
            { totalRows / 2, totalRows / 2 },
            { totalRows - 1, totalRows - 1 },
            { totalRows, 0 },
            { totalRows + 1, 1 },
            { -1, totalRows - 1}
        };

        for (auto it = rowMappings.begin(); it != rowMappings.end(); ++it)
        {
            VERIFY_ARE_EQUAL(static_cast<int>(it->second), _range->_normalizeRow(it->first));
        }
    }

    TEST_METHOD(CanGetViewportHeight)
    {
        Viewport viewport;
        viewport.Top = 0;
        viewport.Bottom = 0;

        // Viewports are inclusive, so Top == Bottom really means 1 row
        VERIFY_ARE_EQUAL(1u, _range->_getViewportHeight(viewport));

        // make the viewport 10 rows tall
        viewport.Top = 3;
        viewport.Bottom = 12;
        VERIFY_ARE_EQUAL(10u, _range->_getViewportHeight(viewport));
    }

    TEST_METHOD(CanGetViewportWidth)
    {
        Viewport viewport;
        viewport.Left = 0;
        viewport.Right = 0;

        // Viewports are inclusive, Left == Right is really 1 column
        VERIFY_ARE_EQUAL(1u, _range->_getViewportWidth(viewport));

        // test a more normal size
        viewport.Right = 300;
        VERIFY_ARE_EQUAL(viewport.Right + 1u, _range->_getViewportWidth(viewport));
    }
};