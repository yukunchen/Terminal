#pragma once

#include "../../renderer/inc/RenderEngineBase.hpp"

#include <dxgi.h>
#include <dxgi1_2.h>

#include <d3d11.h>
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <dwrite_1.h>
#include <dwrite_2.h>
#include <dwrite_3.h>

namespace Microsoft::Console::Render
{
    class DxEngine final : public RenderEngineBase
    {
    public:
        DxEngine();
        virtual ~DxEngine() override;

        // Used to release device resources so that another instance of
        // conhost can render to the screen (i.e. only one DirectX
        // application may control the screen at a time.)
        [[nodiscard]]
        HRESULT Enable() noexcept;
        [[nodiscard]]
        HRESULT Disable() noexcept;

        [[nodiscard]]
        HRESULT SetHwnd(const HWND hwnd) noexcept;

        // IRenderEngine Members
        [[nodiscard]]
        HRESULT Invalidate(const SMALL_RECT* const psrRegion) noexcept override;
        [[nodiscard]]
        HRESULT InvalidateCursor(const COORD* const pcoordCursor) noexcept override;
        [[nodiscard]]
        HRESULT InvalidateSystem(const RECT* const prcDirtyClient) noexcept override;
        [[nodiscard]]
        HRESULT InvalidateSelection(const std::vector<SMALL_RECT>& rectangles) noexcept override;
        [[nodiscard]]
        HRESULT InvalidateScroll(const COORD* const pcoordDelta) noexcept override;
        [[nodiscard]]
        HRESULT InvalidateAll() noexcept override;
        [[nodiscard]]
        HRESULT InvalidateCircling(_Out_ bool* const pForcePaint) noexcept override;
        [[nodiscard]]
        HRESULT PrepareForTeardown(_Out_ bool* const pForcePaint) noexcept override;

        [[nodiscard]]
        HRESULT StartPaint() noexcept override;
        [[nodiscard]]
        HRESULT EndPaint() noexcept override;
        [[nodiscard]]
        HRESULT Present() noexcept override;

        [[nodiscard]]
        HRESULT ScrollFrame() noexcept override;

        [[nodiscard]]
        HRESULT PaintBackground() noexcept override;
        [[nodiscard]]
        HRESULT PaintBufferLine(PCWCHAR const pwsLine,
                                const unsigned char* const rgWidths,
                                size_t const cchLine,
                                COORD const coord,
                                bool const fTrimLeft,
                                const bool lineWrapped) noexcept override;
        [[nodiscard]]
        HRESULT PaintBufferGridLines(GridLines const lines, COLORREF const color, size_t const cchLine, COORD const coordTarget) noexcept override;
        [[nodiscard]]
        HRESULT PaintSelection(const std::vector<SMALL_RECT>& rectangles) noexcept override;

        [[nodiscard]]
        HRESULT PaintCursor(const COORD coordCursor,
                            const ULONG ulCursorHeightPercent,
                            const bool fIsDoubleWidth,
                            const CursorType cursorType,
                            const bool fUseColor,
                            const COLORREF cursorColor) noexcept override;

        [[nodiscard]]
        HRESULT ClearCursor() noexcept override;

        [[nodiscard]]
        HRESULT UpdateDrawingBrushes(COLORREF const colorForeground,
                                     COLORREF const colorBackground,
                                     const WORD legacyColorAttribute,
                                     const bool isBold,
                                     bool const fIncludeBackgrounds) noexcept override;
        [[nodiscard]]
        HRESULT UpdateFont(const FontInfoDesired& fiFontInfoDesired, FontInfo& fiFontInfo) noexcept override;
        [[nodiscard]]
        HRESULT UpdateDpi(int const iDpi) noexcept override;
        [[nodiscard]]
        HRESULT UpdateViewport(const SMALL_RECT srNewViewport) noexcept override;

        [[nodiscard]]
        HRESULT GetProposedFont(const FontInfoDesired& fiFontInfoDesired, FontInfo& fiFontInfo, int const iDpi) noexcept override;

        [[nodiscard]]
        SMALL_RECT GetDirtyRectInChars() noexcept override;

        [[nodiscard]]
        HRESULT GetFontSize(_Out_ COORD* const pFontSize) noexcept override;
        [[nodiscard]]
        HRESULT IsGlyphWideByFont(const std::wstring_view glyph, _Out_ bool* const pResult) noexcept override;

    protected:
        [[nodiscard]]
        HRESULT _DoUpdateTitle(_In_ const std::wstring& newTitle) noexcept override;

    private:
        HWND _hwndTarget;

        bool _isEnabled;
        bool _isPainting;
        
        SIZE _displaySizePixels;
        SIZE _glyphCell;
        float _fontSize;
        float _baseline;

        // Persistent heap memory to optimize line drawing performance.
        // It is very costly to keep allocating/deallocating this for every line drawn.
        // Instead, we're going to hold onto approximately one line worth of heap memory here
        // for the duration of the renderer as scratch space.
        std::vector<uint16_t> _glyphIds;
        std::vector<float> _glyphAdvances;

        [[nodiscard]]
        RECT _GetDisplayRect() const noexcept;

        bool _isInvalidUsed;
        RECT _invalidRect;
        SIZE _invalidScroll;

        void _InvalidOr(SMALL_RECT sr) noexcept;
        void _InvalidOr(RECT rc) noexcept;

        void _InvalidOffset(POINT pt) noexcept;

        bool _presentReady;
        RECT _presentDirty;
        RECT _presentScroll;
        POINT _presentOffset;
        DXGI_PRESENT_PARAMETERS _presentParams;
        
        static const ULONG s_ulMinCursorHeightPercent = 25;
        static const ULONG s_ulMaxCursorHeightPercent = 100;

        // Device-Independent Resources
        Microsoft::WRL::ComPtr<ID2D1Factory> _d2dFactory;
        Microsoft::WRL::ComPtr<IDWriteFactory2> _dwriteFactory;
        Microsoft::WRL::ComPtr<IDWriteTextFormat2> _dwriteTextFormat;
        Microsoft::WRL::ComPtr<IDWriteFontFace5> _dwriteFontFace;
        Microsoft::WRL::ComPtr<IDWriteTextAnalyzer1> _dwriteTextAnalyzer;

        // Device-Dependent Resources
        bool _haveDeviceResources;
        Microsoft::WRL::ComPtr<ID3D11Device> _d3dDevice;
        Microsoft::WRL::ComPtr<ID3D11DeviceContext> _d3dDeviceContext;
        Microsoft::WRL::ComPtr<IDXGIAdapter1> _dxgiAdapter1;
        Microsoft::WRL::ComPtr<IDXGIFactory2> _dxgiFactory2;
        Microsoft::WRL::ComPtr<IDXGIOutput> _dxgiOutput;
        Microsoft::WRL::ComPtr<IDXGISurface> _dxgiSurface;
        Microsoft::WRL::ComPtr<ID2D1RenderTarget> _d2dRenderTarget;
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> _d2dBrushForeground;
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> _d2dBrushBackground;
        Microsoft::WRL::ComPtr<IDXGISwapChain1> _dxgiSwapChain;

        [[nodiscard]]
        HRESULT _CreateDeviceResources(const bool createSwapChain) noexcept;

        void _ReleaseDeviceResources() noexcept;
        
        [[nodiscard]]
        HRESULT _CreateTextLayout(
            _In_reads_(StringLength) PCWCHAR String,
            _In_ size_t StringLength,
            _Out_ IDWriteTextLayout **ppTextLayout) noexcept;

        [[nodiscard]]
        HRESULT _CopyFrontToBack() noexcept;

        [[nodiscard]]
        HRESULT _EnableDisplayAccess(const bool outputEnabled) noexcept;

        [[nodiscard]]
        Microsoft::WRL::ComPtr<IDWriteFontFace5> _FindFontFace(const std::wstring& familyName,
                                                               DWRITE_FONT_WEIGHT weight,
                                                               DWRITE_FONT_STRETCH stretch,
                                                               DWRITE_FONT_STYLE style);
        
        [[nodiscard]]
        COORD _GetFontSize() const noexcept;

        [[nodiscard]]
        SIZE _GetClientSize() const noexcept;

        [[nodiscard]]
        static DWRITE_LINE_SPACING s_DetermineLineSpacing(IDWriteFontFace5* const fontFace, 
                                                          const float fontSize, 
                                                          const float cellHeight) noexcept;

        [[nodiscard]]
        static D2D1_COLOR_F s_ColorFFromColorRef(const COLORREF color) noexcept;
    };
}
