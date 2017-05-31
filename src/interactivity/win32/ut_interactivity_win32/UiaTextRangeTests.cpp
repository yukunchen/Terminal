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

        // set up pointers
        _pScreenInfo = ServiceLocator::LocateGlobals()->getConsoleInformation()->CurrentScreenBuffer;
        _pTextBuffer = _pScreenInfo->TextInfo;

        // set up default range
        _range = new UiaTextRange
        {
            &_dummyProvider,
            _pTextBuffer,
            _pScreenInfo,
            0,
            0
        };

        return true;
    }

    TEST_METHOD_CLEANUP(MethodCleanup)
    {
        _state->CleanupGlobalScreenBuffer();
        _state->CleanupGlobalFont();
        delete _state;
        delete _range;

        _pScreenInfo = nullptr;
        _pTextBuffer = nullptr;
        return true;
    }

    const int _getRowWidth() const
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
            _pTextBuffer,
            _pScreenInfo,
            20,
            20
        };
        VERIFY_IS_TRUE(degenerate._isDegenerate());

        // make a non-degenerate range and verify that it reports as such
        UiaTextRange notDegenerate
        {
            &_dummyProvider,
            _pTextBuffer,
            _pScreenInfo,
            20,
            35
        };
        VERIFY_IS_FALSE(notDegenerate._isDegenerate());
    }

    TEST_METHOD(CanCheckIfRowIsInViewport)
    {
        // check a viewport that's one line tall
        SMALL_RECT viewport;
        viewport.Top = 0;
        viewport.Bottom = 0;

        VERIFY_IS_TRUE(_range->_isRowInViewport(0, viewport));
        VERIFY_IS_FALSE(_range->_isRowInViewport(1, viewport));

        // check a slightly larger viewport
        viewport.Bottom = 5;
        for (auto i = 0; i <= viewport.Bottom; ++i)
        {
            VERIFY_IS_TRUE(_range->_isRowInViewport(i, viewport),
                           NoThrowString().Format(L"%d should be in viewport", i));
        }
        VERIFY_IS_FALSE(_range->_isRowInViewport(viewport.Bottom + 1, viewport));

        // check a viewport that contains the 0th row;
        const auto totalRows = _pTextBuffer->TotalRowCount();
        _pTextBuffer->SetFirstRowIndex(static_cast<SHORT>(totalRows - 5));
        viewport.Top = static_cast<SHORT>(totalRows - 3);

        VERIFY_IS_TRUE(_range->_isRowInViewport(0, viewport));
        VERIFY_IS_FALSE(_range->_isRowInViewport(viewport.Bottom + 2, viewport));
    }

    TEST_METHOD(CanTranslateRowToViewport)
    {
        const auto totalRows = _pTextBuffer->TotalRowCount();

        SMALL_RECT viewport;
        viewport.Top = 0;
        viewport.Bottom = 10;

        // invalid rows return -1
        VERIFY_ARE_EQUAL(-1, _range->_rowToViewport(-5, viewport));
        VERIFY_ARE_EQUAL(-1, _range->_rowToViewport(totalRows, viewport));

        std::vector<std::pair<size_t, size_t>> viewportSizes =
        {
            { 0, 10 }, // viewport at top
            { 2, 10 }, // shifted viewport
            { totalRows - 5, totalRows + 3 } // viewport with 0th row
        };

        for (auto it = viewportSizes.begin(); it != viewportSizes.end(); ++it)
        {
            viewport.Top = static_cast<SHORT>(it->first);
            viewport.Bottom = static_cast<SHORT>(it->second);
            for (auto i = viewport.Top; _range->_isRowInViewport(i, viewport); ++i)
            {
                VERIFY_ARE_EQUAL(i - viewport.Top, _range->_rowToViewport(i, viewport));
            }
        }
    }

    TEST_METHOD(CanTranslateEndpointToRow)
    {
        const auto rowWidth = _getRowWidth();
        for (auto i = 0; i < 300; ++i)
        {
            VERIFY_ARE_EQUAL(i / rowWidth, _range->_endpointToRow(i));
        }
    }


    TEST_METHOD(CanTranslateRowToEndpoint)
    {
        const auto rowWidth = _getRowWidth();
        for (auto i = 0; i < 5; ++i)
        {
            VERIFY_ARE_EQUAL(i * rowWidth, _range->_rowToEndpoint(i));
            // make sure that the translation is reversible
            VERIFY_ARE_EQUAL(i , _range->_endpointToRow(_range->_rowToEndpoint(i)));
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
        const int totalRows = _pTextBuffer->TotalRowCount();
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
        std::vector<std::pair<int, int>> rowMappings =
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
            VERIFY_ARE_EQUAL(it->second, _range->_normalizeRow(it->first));
        }
    }

    TEST_METHOD(CanGetViewportHeight)
    {
        SMALL_RECT viewport;
        viewport.Top = 0;
        viewport.Bottom = 0;

        // SMALL_RECTs are inclusive, so Top == Bottom really means 1 row
        VERIFY_ARE_EQUAL(1, _range->_getViewportHeight(viewport));

        // make the viewport 10 rows tall
        viewport.Top = 3;
        viewport.Bottom = 12;
        VERIFY_ARE_EQUAL(10, _range->_getViewportHeight(viewport));

        // make sure that viewport height is still properly calculated
        // when the 0th row lies within the viewport
        const auto totalRows = _pTextBuffer->TotalRowCount();
        _pTextBuffer->SetFirstRowIndex(static_cast<SHORT>(totalRows - 5));
        viewport.Top = static_cast<SHORT>(totalRows - 3);
        viewport.Bottom = 5;
        const size_t expectedHeight = totalRows - viewport.Top + viewport.Bottom + 1;
        VERIFY_ARE_EQUAL(expectedHeight, _range->_getViewportHeight(viewport));
    }

    TEST_METHOD(CanGetViewportWidth)
    {
        SMALL_RECT viewport;
        viewport.Left = 0;
        viewport.Right = 0;

        // SMALL_RECTs are inclusive, Left == Right is really 1 column
        VERIFY_ARE_EQUAL(1, _range->_getViewportWidth(viewport));

        // test a more normal size
        viewport.Right = 300;
        VERIFY_ARE_EQUAL(viewport.Right + 1, _range->_getViewportWidth(viewport));
    }
};