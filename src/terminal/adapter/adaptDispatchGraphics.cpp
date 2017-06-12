/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include <precomp.h>

#include "adaptDispatch.hpp"
#include "conGetSet.hpp"

#define ENABLE_INTSAFE_SIGNED_FUNCTIONS
#include <intsafe.h>

using namespace Microsoft::Console::VirtualTerminal;

// Routine Description:
// - Small helper to disable all color flags within a given font attributes field
// Arguments:
// - pAttr - Pointer to font attributes field to adjust
// - fIsForeground - True if we're modifying the FOREGROUND colors. False if we're doing BACKGROUND.
// Return Value:
// - <none>
void AdaptDispatch::s_DisableAllColors(_Inout_ WORD* const pAttr, _In_ bool const fIsForeground)
{
    if (fIsForeground)
    {
        *pAttr &= ~(FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
    }
    else
    {
        *pAttr &= ~(BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED | BACKGROUND_INTENSITY); 
    }
}

// Routine Description:
// - Small helper to help mask off the appropriate foreground/background bits in the colors bitfield.
// Arguments:
// - pAttr - Pointer to font attributes field to adjust
// - wApplyThis - Color values to apply to the low or high word of the font attributes field.
// - fIsForeground - TRUE = foreground color. FALSE = background color. Specifies which half of the bit field to reset and then apply wApplyThis upon.
// Return Value:
// - <none>
void AdaptDispatch::s_ApplyColors(_Inout_ WORD* const pAttr, _In_ WORD const wApplyThis, _In_ bool const fIsForeground)
{
    // Copy the new attribute to apply
    WORD wNewColors = wApplyThis;

    // Mask off only the foreground or background
    if (fIsForeground)
    {
        *pAttr &= ~(FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
        wNewColors &= (FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
    }
    else
    {
        *pAttr &= ~(BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED | BACKGROUND_INTENSITY);
        wNewColors &= (BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED | BACKGROUND_INTENSITY);
    }

    // Apply appropriate flags.
    *pAttr |= wNewColors;
}

// Routine Description:
// - Helper to apply the actual flags to each text attributes field.
// - Placed as a helper so it can be recursive/re-entrant for some of the convenience flag methods that perform similar/multiple operations in one command.
// Arguments:
// - opt - Graphics option sent to us by the parser/requestor.
// - pAttr - Pointer to the font attribute field to adjust
// Return Value: 
// - <none>
void AdaptDispatch::_SetGraphicsOptionHelper(_In_ GraphicsOptions const opt, _Inout_ WORD* const pAttr)
{
    switch (opt)
    {
    case GraphicsOptions::Off:
        *pAttr = 0;
        *pAttr |= _wDefaultTextAttributes;
        // Clear out any stored brightness state.
        _wBrightnessState = 0;

        _fChangedForeground = true;
        _fChangedBackground = true;
        _fChangedMetaAttrs = true;
        break;
    case GraphicsOptions::BoldBright:
        *pAttr |= FOREGROUND_INTENSITY;
        // Store that we should use brightness for any normal (non-bright) sequences
        // This is so that 9x series sequences, which are always bright. don't interfere with setting this state.
        // 3x sequences are ONLY bright if this flag is set, and without this the brightness of a 9x could bleed into a 3x.
        _wBrightnessState |= FOREGROUND_INTENSITY;

        _fChangedForeground = true;
        break;
    case GraphicsOptions::Negative:
        *pAttr |= COMMON_LVB_REVERSE_VIDEO;
        _fChangedMetaAttrs = true;
        break;
    case GraphicsOptions::Underline:
        *pAttr |= COMMON_LVB_UNDERSCORE;
        _fChangedMetaAttrs = true;
        break;
    case GraphicsOptions::Positive:
        *pAttr &= ~COMMON_LVB_REVERSE_VIDEO;
        _fChangedMetaAttrs = true;
        break;
    case GraphicsOptions::NoUnderline:
        *pAttr &= ~COMMON_LVB_UNDERSCORE;
        _fChangedMetaAttrs = true;
        break;
    case GraphicsOptions::ForegroundBlack:
        s_DisableAllColors(pAttr, true); // turn off all color flags first.
        _fChangedForeground = true;
        break;
    case GraphicsOptions::ForegroundBlue:
        s_DisableAllColors(pAttr, true); // turn off all color flags first.
        *pAttr |= FOREGROUND_BLUE;
        _fChangedForeground = true;
        break;
    case GraphicsOptions::ForegroundGreen:
        s_DisableAllColors(pAttr, true); // turn off all color flags first.
        *pAttr |= FOREGROUND_GREEN;
        _fChangedForeground = true;
        break;
    case GraphicsOptions::ForegroundCyan:
        s_DisableAllColors(pAttr, true); // turn off all color flags first.
        *pAttr |= FOREGROUND_BLUE | FOREGROUND_GREEN;
        _fChangedForeground = true;
        break;
    case GraphicsOptions::ForegroundRed:
        s_DisableAllColors(pAttr, true); // turn off all color flags first.
        *pAttr |= FOREGROUND_RED;
        _fChangedForeground = true;
        break;
    case GraphicsOptions::ForegroundMagenta:
        s_DisableAllColors(pAttr, true); // turn off all color flags first.
        *pAttr |= FOREGROUND_BLUE | FOREGROUND_RED;
        _fChangedForeground = true;
        break;
    case GraphicsOptions::ForegroundYellow:
        s_DisableAllColors(pAttr, true); // turn off all color flags first.
        *pAttr |= FOREGROUND_GREEN | FOREGROUND_RED;
        _fChangedForeground = true;
        break;
    case GraphicsOptions::ForegroundWhite:
        s_DisableAllColors(pAttr, true); // turn off all color flags first.
        *pAttr |= FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
        _fChangedForeground = true;
        break;
    case GraphicsOptions::ForegroundDefault:
        s_ApplyColors(pAttr, _wDefaultTextAttributes, true);
        _fChangedForeground = true;
        break;
    case GraphicsOptions::BackgroundBlack:
        s_DisableAllColors(pAttr, false); // turn off all color flags first.
        _fChangedBackground = true;
        break;
    case GraphicsOptions::BackgroundBlue:
        s_DisableAllColors(pAttr, false); // turn off all color flags first.
        *pAttr |= BACKGROUND_BLUE;
        _fChangedBackground = true;
        break;
    case GraphicsOptions::BackgroundGreen:
        s_DisableAllColors(pAttr, false); // turn off all color flags first.
        *pAttr |= BACKGROUND_GREEN;
        _fChangedBackground = true;
        break;
    case GraphicsOptions::BackgroundCyan:
        s_DisableAllColors(pAttr, false); // turn off all color flags first.
        *pAttr |= BACKGROUND_BLUE | BACKGROUND_GREEN;
        _fChangedBackground = true;
        break;
    case GraphicsOptions::BackgroundRed:
        s_DisableAllColors(pAttr, false); // turn off all color flags first.
        *pAttr |= BACKGROUND_RED;
        _fChangedBackground = true;
        break;
    case GraphicsOptions::BackgroundMagenta:
        s_DisableAllColors(pAttr, false); // turn off all color flags first.
        *pAttr |= BACKGROUND_BLUE | BACKGROUND_RED;
        _fChangedBackground = true;
        break;
    case GraphicsOptions::BackgroundYellow:
        s_DisableAllColors(pAttr, false); // turn off all color flags first.
        *pAttr |= BACKGROUND_GREEN | BACKGROUND_RED;
        _fChangedBackground = true;
        break;
    case GraphicsOptions::BackgroundWhite:
        s_DisableAllColors(pAttr, false); // turn off all color flags first.
        *pAttr |= BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED;
        _fChangedBackground = true;
        break;
    case GraphicsOptions::BackgroundDefault:
        s_ApplyColors(pAttr, _wDefaultTextAttributes, false);
        _fChangedBackground = true;
        break;
    case GraphicsOptions::BrightForegroundBlack:
        _SetGraphicsOptionHelper(GraphicsOptions::ForegroundBlack, pAttr);
        *pAttr |= FOREGROUND_INTENSITY;
        _fChangedForeground = true;
        break;
    case GraphicsOptions::BrightForegroundBlue:
        _SetGraphicsOptionHelper(GraphicsOptions::ForegroundBlue, pAttr);
        *pAttr |= FOREGROUND_INTENSITY;
        _fChangedForeground = true;
        break;
    case GraphicsOptions::BrightForegroundGreen:
        _SetGraphicsOptionHelper(GraphicsOptions::ForegroundGreen, pAttr);
        *pAttr |= FOREGROUND_INTENSITY;
        _fChangedForeground = true;
        break;
    case GraphicsOptions::BrightForegroundCyan:
        _SetGraphicsOptionHelper(GraphicsOptions::ForegroundCyan, pAttr);
        *pAttr |= FOREGROUND_INTENSITY;
        _fChangedForeground = true;
        break;
    case GraphicsOptions::BrightForegroundRed:
        _SetGraphicsOptionHelper(GraphicsOptions::ForegroundRed, pAttr);
        *pAttr |= FOREGROUND_INTENSITY;
        _fChangedForeground = true;
        break;
    case GraphicsOptions::BrightForegroundMagenta:
        _SetGraphicsOptionHelper(GraphicsOptions::ForegroundMagenta, pAttr);
        *pAttr |= FOREGROUND_INTENSITY;
        _fChangedForeground = true;
        break;
    case GraphicsOptions::BrightForegroundYellow:
        _SetGraphicsOptionHelper(GraphicsOptions::ForegroundYellow, pAttr);
        *pAttr |= FOREGROUND_INTENSITY;
        _fChangedForeground = true;
        break;
    case GraphicsOptions::BrightForegroundWhite:
        _SetGraphicsOptionHelper(GraphicsOptions::ForegroundWhite, pAttr);
        *pAttr |= FOREGROUND_INTENSITY;
        _fChangedForeground = true;
        break;
    case GraphicsOptions::BrightBackgroundBlack:
        _SetGraphicsOptionHelper(GraphicsOptions::BackgroundBlack, pAttr);
        *pAttr |= BACKGROUND_INTENSITY;
        _fChangedBackground = true;
        break;
    case GraphicsOptions::BrightBackgroundBlue:
        _SetGraphicsOptionHelper(GraphicsOptions::BackgroundBlue, pAttr);
        *pAttr |= BACKGROUND_INTENSITY;
        _fChangedBackground = true;
        break;
    case GraphicsOptions::BrightBackgroundGreen:
        _SetGraphicsOptionHelper(GraphicsOptions::BackgroundGreen, pAttr);
        *pAttr |= BACKGROUND_INTENSITY;
        _fChangedBackground = true;
        break;
    case GraphicsOptions::BrightBackgroundCyan:
        _SetGraphicsOptionHelper(GraphicsOptions::BackgroundCyan, pAttr);
        *pAttr |= BACKGROUND_INTENSITY;
        _fChangedBackground = true;
        break;
    case GraphicsOptions::BrightBackgroundRed:
        _SetGraphicsOptionHelper(GraphicsOptions::BackgroundRed, pAttr);
        *pAttr |= BACKGROUND_INTENSITY;
        _fChangedBackground = true;
        break;
    case GraphicsOptions::BrightBackgroundMagenta:
        _SetGraphicsOptionHelper(GraphicsOptions::BackgroundMagenta, pAttr);
        *pAttr |= BACKGROUND_INTENSITY;
        _fChangedBackground = true;
        break;
    case GraphicsOptions::BrightBackgroundYellow:
        _SetGraphicsOptionHelper(GraphicsOptions::BackgroundYellow, pAttr);
        *pAttr |= BACKGROUND_INTENSITY;
        _fChangedBackground = true;
        break;
    case GraphicsOptions::BrightBackgroundWhite:
        _SetGraphicsOptionHelper(GraphicsOptions::BackgroundWhite, pAttr);
        *pAttr |= BACKGROUND_INTENSITY;
        _fChangedBackground = true;
        break;
    }
    // Apply the stored brightness state
    *pAttr |= _wBrightnessState;
}

// Routine Description:
// Returns true if the GraphicsOption represents an extended color option.
//   These are followed by up to 4 more values which compose the entire option.
// Return Value: 
// - true if the opt is the indicator for an extended color sequence, false otherwise.
bool AdaptDispatch::s_IsRgbColorOption(_In_ GraphicsOptions const opt)
{
    return opt == GraphicsOptions::ForegroundExtended || opt == GraphicsOptions::BackgroundExtended;
}

// Routine Description:
// - Helper to parse extended graphics options, which start with 38 (FG) or 48 (BG)
//     These options are followed by either a 2 (RGB) or 5 (xterm index)
//      RGB sequences then take 3 MORE params to designate the R, G, B parts of the color
//      Xterm index will use the param that follows to use a color from the preset 256 color xterm color table.
// Arguments:
// - rgOptions - An array of options that will be used to generate the RGB color
// - cOptions - The count of options 
// - prgbColor - A pointer to place the generated RGB color into.
// - pfIsForeground - a pointer to place whether or not the parsed color is for the foreground or not.
// - pcOptionsConsumed - a pointer to place the number of options we consumed parsing this option.
// - ColorTable - the windows color table, for xterm indices < 16 
// - cColorTable - The number of elements in the windows color table.
// Return Value: 
// Returns true if we successfully parsed an extended color option from the options array.
// - This corresponds to the following number of options consumed (pcOptionsConsumed):
//     1 - false, not enough options to parse.
//     2 - false, not enough options to parse.
//     3 - true, parsed an xterm index to a color
//     5 - true, parsed an RGB color.
bool AdaptDispatch::_SetRgbColorsHelper(_In_reads_(cOptions) const GraphicsOptions* const rgOptions, 
                          _In_ size_t const cOptions, 
                          _Out_ COLORREF* const prgbColor, 
                          _Out_ bool* const pfIsForeground, 
                          _Out_ size_t* const pcOptionsConsumed)
{
    bool fSuccess = false;
    *pcOptionsConsumed = 1;
    if (cOptions >= 2 && s_IsRgbColorOption(rgOptions[0]))
    {
        *pcOptionsConsumed = 2;
        GraphicsOptions extendedOpt = rgOptions[0];
        GraphicsOptions typeOpt = rgOptions[1];

        if (extendedOpt == GraphicsOptions::ForegroundExtended)
        {
            *pfIsForeground = true;
        }
        else if (extendedOpt == GraphicsOptions::BackgroundExtended)
        {
            *pfIsForeground = false;
        }

        if (typeOpt == GraphicsOptions::RGBColor && cOptions >= 5)
        {
            *pcOptionsConsumed = 5;
            // ensure that each value fits in a byte
            unsigned int red = rgOptions[2] > 255? 255 : rgOptions[2];
            unsigned int green = rgOptions[3] > 255? 255 : rgOptions[3];
            unsigned int blue = rgOptions[4] > 255? 255 : rgOptions[4];

            *prgbColor = RGB(red, green, blue);

            fSuccess = !!_pConApi->SetConsoleRGBTextAttribute(*prgbColor, *pfIsForeground);
        } 
        else if (typeOpt == GraphicsOptions::Xterm256Index && cOptions >= 3)
        {
            *pcOptionsConsumed = 3;
            if (rgOptions[2] <= 255) // ensure that the provided index is on the table
            {
                unsigned int tableIndex = rgOptions[2];

                fSuccess = !!_pConApi->SetConsoleXtermTextAttribute(tableIndex, *pfIsForeground);
            }
        }
    }
    return fSuccess;
}

// Routine Description:
// - SGR - Modifies the graphical rendering options applied to the next characters written into the buffer.
//       - Options include colors, invert, underlines, and other "font style" type options.
// Arguments:
// - rgOptions - An array of options that will be applied from 0 to N, in order, one at a time by setting or removing flags in the font style properties.
// - cOptions - The count of options (a.k.a. the N in the above line of comments)
// Return Value: 
// - True if handled successfully. False otherwise.
bool AdaptDispatch::SetGraphicsRendition(_In_reads_(cOptions) const GraphicsOptions* const rgOptions, _In_ size_t const cOptions)
{
    CONSOLE_SCREEN_BUFFER_INFOEX csbiex = { 0 };
    csbiex.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);
    bool fSuccess = !!_pConApi->GetConsoleScreenBufferInfoEx(&csbiex);

    if (fSuccess)
    {
        WORD attr = csbiex.wAttributes; // existing color attributes

        // Run through the graphics options and apply them
        for (size_t i = 0; i < cOptions; i++)
        {
            GraphicsOptions opt = rgOptions[i];
            if (s_IsRgbColorOption(opt))
            {
                COLORREF rgbColor;
                bool fIsForeground = true;

                size_t cOptionsConsumed = 0;

                // _SetRgbColorsHelper will call the appropriate ConApi function
                fSuccess = _SetRgbColorsHelper(&(rgOptions[i]), cOptions-i, &rgbColor, &fIsForeground, &cOptionsConsumed);

                i += (cOptionsConsumed - 1); // cOptionsConsumed includes the opt we're currently on.
            }
            else
            {
                _SetGraphicsOptionHelper(opt, &attr);

                fSuccess = !!_pConApi->PrivateSetLegacyAttributes(attr, _fChangedForeground, _fChangedBackground, _fChangedMetaAttrs);

                _fChangedForeground = false;
                _fChangedBackground = false;
                _fChangedMetaAttrs = false;
            }
        }

    }

    return fSuccess;
}