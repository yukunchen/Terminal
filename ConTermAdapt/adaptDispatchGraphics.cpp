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


// Technically, this table can be changed by the sequence "OSC 4 ; <index> ; <color> BEL"
// However, since we aren't implementing that yet, we can leave it a constant here.
const COLORREF AdaptDispatch::s_XtermColorTable[XTERM_COLOR_TABLE_SIZE] = {
    RGB(0x00, 0x00, 0x00), // 0
    RGB(0x80, 0x00, 0x00), // 1
    RGB(0x00, 0x80, 0x00), // 2
    RGB(0x80, 0x80, 0x00), // 3
    RGB(0x00, 0x00, 0x80), // 4
    RGB(0x80, 0x00, 0x80), // 5
    RGB(0x00, 0x80, 0x80), // 6
    RGB(0xc0, 0xc0, 0xc0), // 7
    RGB(0x80, 0x80, 0x80), // 8
    RGB(0xff, 0x00, 0x00), // 9
    RGB(0x00, 0xff, 0x00), // 10
    RGB(0xff, 0xff, 0x00), // 11
    RGB(0x00, 0x00, 0xff), // 12
    RGB(0xff, 0x00, 0xff), // 13
    RGB(0x00, 0xff, 0xff), // 14
    RGB(0xff, 0xff, 0xff), // 15
    RGB(0x00, 0x00, 0x00), // 16
    RGB(0x00, 0x00, 0x5f), // 17
    RGB(0x00, 0x00, 0x87), // 18
    RGB(0x00, 0x00, 0xaf), // 19
    RGB(0x00, 0x00, 0xd7), // 20
    RGB(0x00, 0x00, 0xff), // 21
    RGB(0x00, 0x5f, 0x00), // 22
    RGB(0x00, 0x5f, 0x5f), // 23
    RGB(0x00, 0x5f, 0x87), // 24
    RGB(0x00, 0x5f, 0xaf), // 25
    RGB(0x00, 0x5f, 0xd7), // 26
    RGB(0x00, 0x5f, 0xff), // 27
    RGB(0x00, 0x87, 0x00), // 28
    RGB(0x00, 0x87, 0x5f), // 29
    RGB(0x00, 0x87, 0x87), // 30
    RGB(0x00, 0x87, 0xaf), // 31
    RGB(0x00, 0x87, 0xd7), // 32
    RGB(0x00, 0x87, 0xff), // 33
    RGB(0x00, 0xaf, 0x00), // 34
    RGB(0x00, 0xaf, 0x5f), // 35
    RGB(0x00, 0xaf, 0x87), // 36
    RGB(0x00, 0xaf, 0xaf), // 37
    RGB(0x00, 0xaf, 0xd7), // 38
    RGB(0x00, 0xaf, 0xff), // 39
    RGB(0x00, 0xd7, 0x00), // 40
    RGB(0x00, 0xd7, 0x5f), // 41
    RGB(0x00, 0xd7, 0x87), // 42
    RGB(0x00, 0xd7, 0xaf), // 43
    RGB(0x00, 0xd7, 0xd7), // 44
    RGB(0x00, 0xd7, 0xff), // 45
    RGB(0x00, 0xff, 0x00), // 46
    RGB(0x00, 0xff, 0x5f), // 47
    RGB(0x00, 0xff, 0x87), // 48
    RGB(0x00, 0xff, 0xaf), // 49
    RGB(0x00, 0xff, 0xd7), // 50
    RGB(0x00, 0xff, 0xff), // 51
    RGB(0x5f, 0x00, 0x00), // 52
    RGB(0x5f, 0x00, 0x5f), // 53
    RGB(0x5f, 0x00, 0x87), // 54
    RGB(0x5f, 0x00, 0xaf), // 55
    RGB(0x5f, 0x00, 0xd7), // 56
    RGB(0x5f, 0x00, 0xff), // 57
    RGB(0x5f, 0x5f, 0x00), // 58
    RGB(0x5f, 0x5f, 0x5f), // 59
    RGB(0x5f, 0x5f, 0x87), // 60
    RGB(0x5f, 0x5f, 0xaf), // 61
    RGB(0x5f, 0x5f, 0xd7), // 62
    RGB(0x5f, 0x5f, 0xff), // 63
    RGB(0x5f, 0x87, 0x00), // 64
    RGB(0x5f, 0x87, 0x5f), // 65
    RGB(0x5f, 0x87, 0x87), // 66
    RGB(0x5f, 0x87, 0xaf), // 67
    RGB(0x5f, 0x87, 0xd7), // 68
    RGB(0x5f, 0x87, 0xff), // 69
    RGB(0x5f, 0xaf, 0x00), // 70
    RGB(0x5f, 0xaf, 0x5f), // 71
    RGB(0x5f, 0xaf, 0x87), // 72
    RGB(0x5f, 0xaf, 0xaf), // 73
    RGB(0x5f, 0xaf, 0xd7), // 74
    RGB(0x5f, 0xaf, 0xff), // 75
    RGB(0x5f, 0xd7, 0x00), // 76
    RGB(0x5f, 0xd7, 0x5f), // 77
    RGB(0x5f, 0xd7, 0x87), // 78
    RGB(0x5f, 0xd7, 0xaf), // 79
    RGB(0x5f, 0xd7, 0xd7), // 80
    RGB(0x5f, 0xd7, 0xff), // 81
    RGB(0x5f, 0xff, 0x00), // 82
    RGB(0x5f, 0xff, 0x5f), // 83
    RGB(0x5f, 0xff, 0x87), // 84
    RGB(0x5f, 0xff, 0xaf), // 85
    RGB(0x5f, 0xff, 0xd7), // 86
    RGB(0x5f, 0xff, 0xff), // 87
    RGB(0x87, 0x00, 0x00), // 88
    RGB(0x87, 0x00, 0x5f), // 89
    RGB(0x87, 0x00, 0x87), // 90
    RGB(0x87, 0x00, 0xaf), // 91
    RGB(0x87, 0x00, 0xd7), // 92
    RGB(0x87, 0x00, 0xff), // 93
    RGB(0x87, 0x5f, 0x00), // 94
    RGB(0x87, 0x5f, 0x5f), // 95
    RGB(0x87, 0x5f, 0x87), // 96
    RGB(0x87, 0x5f, 0xaf), // 97
    RGB(0x87, 0x5f, 0xd7), // 98
    RGB(0x87, 0x5f, 0xff), // 99
    RGB(0x87, 0x87, 0x00), // 100
    RGB(0x87, 0x87, 0x5f), // 101
    RGB(0x87, 0x87, 0x87), // 102
    RGB(0x87, 0x87, 0xaf), // 103
    RGB(0x87, 0x87, 0xd7), // 104
    RGB(0x87, 0x87, 0xff), // 105
    RGB(0x87, 0xaf, 0x00), // 106
    RGB(0x87, 0xaf, 0x5f), // 107
    RGB(0x87, 0xaf, 0x87), // 108
    RGB(0x87, 0xaf, 0xaf), // 109
    RGB(0x87, 0xaf, 0xd7), // 110
    RGB(0x87, 0xaf, 0xff), // 111
    RGB(0x87, 0xd7, 0x00), // 112
    RGB(0x87, 0xd7, 0x5f), // 113
    RGB(0x87, 0xd7, 0x87), // 114
    RGB(0x87, 0xd7, 0xaf), // 115
    RGB(0x87, 0xd7, 0xd7), // 116
    RGB(0x87, 0xd7, 0xff), // 117
    RGB(0x87, 0xff, 0x00), // 118
    RGB(0x87, 0xff, 0x5f), // 119
    RGB(0x87, 0xff, 0x87), // 120
    RGB(0x87, 0xff, 0xaf), // 121
    RGB(0x87, 0xff, 0xd7), // 122
    RGB(0x87, 0xff, 0xff), // 123
    RGB(0xaf, 0x00, 0x00), // 124
    RGB(0xaf, 0x00, 0x5f), // 125
    RGB(0xaf, 0x00, 0x87), // 126
    RGB(0xaf, 0x00, 0xaf), // 127
    RGB(0xaf, 0x00, 0xd7), // 128
    RGB(0xaf, 0x00, 0xff), // 129
    RGB(0xaf, 0x5f, 0x00), // 130
    RGB(0xaf, 0x5f, 0x5f), // 131
    RGB(0xaf, 0x5f, 0x87), // 132
    RGB(0xaf, 0x5f, 0xaf), // 133
    RGB(0xaf, 0x5f, 0xd7), // 134
    RGB(0xaf, 0x5f, 0xff), // 135
    RGB(0xaf, 0x87, 0x00), // 136
    RGB(0xaf, 0x87, 0x5f), // 137
    RGB(0xaf, 0x87, 0x87), // 138
    RGB(0xaf, 0x87, 0xaf), // 139
    RGB(0xaf, 0x87, 0xd7), // 140
    RGB(0xaf, 0x87, 0xff), // 141
    RGB(0xaf, 0xaf, 0x00), // 142
    RGB(0xaf, 0xaf, 0x5f), // 143
    RGB(0xaf, 0xaf, 0x87), // 144
    RGB(0xaf, 0xaf, 0xaf), // 145
    RGB(0xaf, 0xaf, 0xd7), // 146
    RGB(0xaf, 0xaf, 0xff), // 147
    RGB(0xaf, 0xd7, 0x00), // 148
    RGB(0xaf, 0xd7, 0x5f), // 149
    RGB(0xaf, 0xd7, 0x87), // 150
    RGB(0xaf, 0xd7, 0xaf), // 151
    RGB(0xaf, 0xd7, 0xd7), // 152
    RGB(0xaf, 0xd7, 0xff), // 153
    RGB(0xaf, 0xff, 0x00), // 154
    RGB(0xaf, 0xff, 0x5f), // 155
    RGB(0xaf, 0xff, 0x87), // 156
    RGB(0xaf, 0xff, 0xaf), // 157
    RGB(0xaf, 0xff, 0xd7), // 158
    RGB(0xaf, 0xff, 0xff), // 159
    RGB(0xd7, 0x00, 0x00), // 160
    RGB(0xd7, 0x00, 0x5f), // 161
    RGB(0xd7, 0x00, 0x87), // 162
    RGB(0xd7, 0x00, 0xaf), // 163
    RGB(0xd7, 0x00, 0xd7), // 164
    RGB(0xd7, 0x00, 0xff), // 165
    RGB(0xd7, 0x5f, 0x00), // 166
    RGB(0xd7, 0x5f, 0x5f), // 167
    RGB(0xd7, 0x5f, 0x87), // 168
    RGB(0xd7, 0x5f, 0xaf), // 169
    RGB(0xd7, 0x5f, 0xd7), // 170
    RGB(0xd7, 0x5f, 0xff), // 171
    RGB(0xd7, 0x87, 0x00), // 172
    RGB(0xd7, 0x87, 0x5f), // 173
    RGB(0xd7, 0x87, 0x87), // 174
    RGB(0xd7, 0x87, 0xaf), // 175
    RGB(0xd7, 0x87, 0xd7), // 176
    RGB(0xd7, 0x87, 0xff), // 177
    RGB(0xdf, 0xaf, 0x00), // 178
    RGB(0xdf, 0xaf, 0x5f), // 179
    RGB(0xdf, 0xaf, 0x87), // 180
    RGB(0xdf, 0xaf, 0xaf), // 181
    RGB(0xdf, 0xaf, 0xd7), // 182
    RGB(0xdf, 0xaf, 0xff), // 183
    RGB(0xdf, 0xd7, 0x00), // 184
    RGB(0xdf, 0xd7, 0x5f), // 185
    RGB(0xdf, 0xd7, 0x87), // 186
    RGB(0xdf, 0xd7, 0xaf), // 187
    RGB(0xdf, 0xd7, 0xd7), // 188
    RGB(0xdf, 0xd7, 0xff), // 189
    RGB(0xdf, 0xff, 0x00), // 190
    RGB(0xdf, 0xff, 0x5f), // 191
    RGB(0xdf, 0xff, 0x87), // 192
    RGB(0xdf, 0xff, 0xaf), // 193
    RGB(0xdf, 0xff, 0xd7), // 194
    RGB(0xdf, 0xff, 0xff), // 195
    RGB(0xff, 0x00, 0x00), // 196
    RGB(0xff, 0x00, 0x5f), // 197
    RGB(0xff, 0x00, 0x87), // 198
    RGB(0xff, 0x00, 0xaf), // 199
    RGB(0xff, 0x00, 0xd7), // 200
    RGB(0xff, 0x00, 0xff), // 201
    RGB(0xff, 0x5f, 0x00), // 202
    RGB(0xff, 0x5f, 0x5f), // 203
    RGB(0xff, 0x5f, 0x87), // 204
    RGB(0xff, 0x5f, 0xaf), // 205
    RGB(0xff, 0x5f, 0xd7), // 206
    RGB(0xff, 0x5f, 0xff), // 207
    RGB(0xff, 0x87, 0x00), // 208
    RGB(0xff, 0x87, 0x5f), // 209
    RGB(0xff, 0x87, 0x87), // 210
    RGB(0xff, 0x87, 0xaf), // 211
    RGB(0xff, 0x87, 0xd7), // 212
    RGB(0xff, 0x87, 0xff), // 213
    RGB(0xff, 0xaf, 0x00), // 214
    RGB(0xff, 0xaf, 0x5f), // 215
    RGB(0xff, 0xaf, 0x87), // 216
    RGB(0xff, 0xaf, 0xaf), // 217
    RGB(0xff, 0xaf, 0xd7), // 218
    RGB(0xff, 0xaf, 0xff), // 219
    RGB(0xff, 0xd7, 0x00), // 220
    RGB(0xff, 0xd7, 0x5f), // 221
    RGB(0xff, 0xd7, 0x87), // 222
    RGB(0xff, 0xd7, 0xaf), // 223
    RGB(0xff, 0xd7, 0xd7), // 224
    RGB(0xff, 0xd7, 0xff), // 225
    RGB(0xff, 0xff, 0x00), // 226
    RGB(0xff, 0xff, 0x5f), // 227
    RGB(0xff, 0xff, 0x87), // 228
    RGB(0xff, 0xff, 0xaf), // 229
    RGB(0xff, 0xff, 0xd7), // 230
    RGB(0xff, 0xff, 0xff), // 231
    RGB(0x08, 0x08, 0x08), // 232
    RGB(0x12, 0x12, 0x12), // 233
    RGB(0x1c, 0x1c, 0x1c), // 234
    RGB(0x26, 0x26, 0x26), // 235
    RGB(0x30, 0x30, 0x30), // 236
    RGB(0x3a, 0x3a, 0x3a), // 237
    RGB(0x44, 0x44, 0x44), // 238
    RGB(0x4e, 0x4e, 0x4e), // 239
    RGB(0x58, 0x58, 0x58), // 240
    RGB(0x62, 0x62, 0x62), // 241
    RGB(0x6c, 0x6c, 0x6c), // 242
    RGB(0x76, 0x76, 0x76), // 243
    RGB(0x80, 0x80, 0x80), // 244
    RGB(0x8a, 0x8a, 0x8a), // 245
    RGB(0x94, 0x94, 0x94), // 246
    RGB(0x9e, 0x9e, 0x9e), // 247
    RGB(0xa8, 0xa8, 0xa8), // 248
    RGB(0xb2, 0xb2, 0xb2), // 249
    RGB(0xbc, 0xbc, 0xbc), // 250
    RGB(0xc6, 0xc6, 0xc6), // 251
    RGB(0xd0, 0xd0, 0xd0), // 252
    RGB(0xda, 0xda, 0xda), // 253
    RGB(0xe4, 0xe4, 0xe4), // 254
    RGB(0xee, 0xee, 0xee)  // 255
};

//Routine Description:
// Finds the "distance" between a given HSL color and an RGB color, using the HSL color space.
//   This function is designed such that the caller would convert one RGB color to HSL ahead of time, 
//   then compare many RGB colors to that first color. 
//Arguments:
// - phslColorA - a pointer to the first color, as a HSL color.
// - rgbColorB - The second color to compare, in RGB color.
// Return value:
// The "distance" between the two.
double AdaptDispatch::s_FindDifference(_In_ const HSL* const phslColorA, _In_ const COLORREF rgbColorB)
{
    HSL hslColorB = HSL(rgbColorB);
    return sqrt( pow((hslColorB.h - phslColorA->h), 2) + 
                 pow((hslColorB.s - phslColorA->s), 2) + 
                 pow((hslColorB.l - phslColorA->l), 2) );
}

//Routine Description:
// For a given RGB color Color, finds the nearest color from the array ColorTable, and returns the index of that match.
//Arguments:
// - Color - The RGB color to fine the nearest color to.
// - ColorTable - The array of colors to find a nearest color from.
// - cColorTable - The number of elements in ColorTable
// Return value:
// The index in ColorTable of the nearest match to Color.
size_t AdaptDispatch::s_FindNearestTableIndex(_In_ COLORREF const Color,
                                              _In_reads_(cColorTable) COLORREF* const ColorTable,
                                              _In_ size_t const cColorTable)
{
    HSL hslColor = HSL(Color);
    size_t closest = 0;
    if (cColorTable >= 1)
    {
        double minDiff = s_FindDifference(&hslColor, ColorTable[0]);
        for (size_t i = 1; i < cColorTable; i++)
        {
            double diff = s_FindDifference(&hslColor, ColorTable[i]);
            if (diff < minDiff)
            {
                minDiff = diff;
                closest = i;
            }
        }
    }
    return closest;
}

//Routine Description:
// Generates a GraphicsOption from the given Windows-style color table index. 
// This is necessary (despite not looking like it at first glance) because our 
//   color table is 4 = Red, 2 = Green, 1 = Blue and everyone else's is 1 = Red, 2 , Green, 4 = Blue (etc)
//Arguments:
// - ColorTableIndex - The Windows-style color table index
// - IsForeground - If the color table index should be used for the forground (true) or background (false).
// Return value:
// A GraphicsOption that corresponds to the ColorTableIndex
unsigned int AdaptDispatch::s_TranslateIndexToGraphicsOption(_In_ size_t const ColorTableIndex, _In_ bool const IsForeground)
{
    unsigned int opt = 0;

    switch (ColorTableIndex)
    {
        case 0:
            opt = GraphicsOptions::ForegroundBlack;
            break;
        case 1:
            opt = GraphicsOptions::ForegroundBlue;
            break;
        case 2:
            opt = GraphicsOptions::ForegroundGreen;
            break;
        case 3:
            opt = GraphicsOptions::ForegroundCyan;
            break;
        case 4:
            opt = GraphicsOptions::ForegroundRed;
            break;
        case 5:
            opt = GraphicsOptions::ForegroundMagenta;
            break;
        case 6:
            opt = GraphicsOptions::ForegroundYellow;
            break;
        case 7:
            opt = GraphicsOptions::ForegroundWhite;
            break;
        case 8:
            opt = GraphicsOptions::BrightForegroundBlack;
            break;
        case 9:
            opt = GraphicsOptions::BrightForegroundBlue;
            break;
        case 10:
            opt = GraphicsOptions::BrightForegroundGreen;
            break;
        case 11:
            opt = GraphicsOptions::BrightForegroundCyan;
            break;
        case 12:
            opt = GraphicsOptions::BrightForegroundRed;
            break;
        case 13:
            opt = GraphicsOptions::BrightForegroundMagenta;
            break;
        case 14:
            opt = GraphicsOptions::BrightForegroundYellow;
            break;
        case 15:
            opt = GraphicsOptions::BrightForegroundWhite;
            break;
    }
    if (!IsForeground)
    {
        opt += 10; // Background GraphicsOptions are offset from their foreground counterparts by and offest of 10 
    }
    return opt;
}

//Routine Description:
// Performs (roughly) the reverse of s_TranslateIndexToGraphicsOption. 
// Finds the Windows-style color table index that represents the same color as the given xterm color table index.
//Arguments:
// - XtermIndex - The xterm-style color table index
// Return value:
// The corresponding windows color table index.
unsigned int AdaptDispatch::s_TranslateXtermIndexToWindowsIndex(_In_ size_t const XtermIndex)
{
    unsigned int uiWindowsIndex = 0;
    if (XtermIndex < 16) // outside of the first 16, the extended (256) xterm color table doesn't nicely map to windows
    {
        bool isBright = XtermIndex > 8;
        switch(XtermIndex % 8) // both bright and normal translate in the same order
        {
            case 0:
                uiWindowsIndex = 0;
                break;
            case 1:
                uiWindowsIndex = 4;
                break;
            case 2:
                uiWindowsIndex = 2;
                break;
            case 3:
                uiWindowsIndex = 6;
                break;
            case 4:
                uiWindowsIndex = 1;
                break;
            case 5:
                uiWindowsIndex = 5;
                break;
            case 6:
                uiWindowsIndex = 3;
                break;
            case 7:
                uiWindowsIndex = 7;
                break;
        }
        if (isBright)
        {
            uiWindowsIndex += 8; // Bright colors are offset from their normal counterparts by 8
        }
    }
    return uiWindowsIndex;
}




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
        break;
    case GraphicsOptions::BoldBright:
        *pAttr |= FOREGROUND_INTENSITY;
        // Store that we should use brightness for any normal (non-bright) sequences
        // This is so that 9x series sequences, which are always bright. don't interfere with setting this state.
        // 3x sequences are ONLY bright if this flag is set, and without this the brightness of a 9x could bleed into a 3x.
        _wBrightnessState |= FOREGROUND_INTENSITY;
        break;
    case GraphicsOptions::Negative:
        *pAttr |= COMMON_LVB_REVERSE_VIDEO;
        break;
    case GraphicsOptions::Underline:
        *pAttr |= COMMON_LVB_UNDERSCORE;
        break;
    case GraphicsOptions::Positive:
        *pAttr &= ~COMMON_LVB_REVERSE_VIDEO;
        break;
    case GraphicsOptions::NoUnderline:
        *pAttr &= ~COMMON_LVB_UNDERSCORE;
        break;
    case GraphicsOptions::ForegroundBlack:
        s_DisableAllColors(pAttr, true); // turn off all color flags first.
        break;
    case GraphicsOptions::ForegroundBlue:
        s_DisableAllColors(pAttr, true); // turn off all color flags first.
        *pAttr |= FOREGROUND_BLUE;
        break;
    case GraphicsOptions::ForegroundGreen:
        s_DisableAllColors(pAttr, true); // turn off all color flags first.
        *pAttr |= FOREGROUND_GREEN;
        break;
    case GraphicsOptions::ForegroundCyan:
        s_DisableAllColors(pAttr, true); // turn off all color flags first.
        *pAttr |= FOREGROUND_BLUE | FOREGROUND_GREEN;
        break;
    case GraphicsOptions::ForegroundRed:
        s_DisableAllColors(pAttr, true); // turn off all color flags first.
        *pAttr |= FOREGROUND_RED;
        break;
    case GraphicsOptions::ForegroundMagenta:
        s_DisableAllColors(pAttr, true); // turn off all color flags first.
        *pAttr |= FOREGROUND_BLUE | FOREGROUND_RED;
        break;
    case GraphicsOptions::ForegroundYellow:
        s_DisableAllColors(pAttr, true); // turn off all color flags first.
        *pAttr |= FOREGROUND_GREEN | FOREGROUND_RED;
        break;
    case GraphicsOptions::ForegroundWhite:
        s_DisableAllColors(pAttr, true); // turn off all color flags first.
        *pAttr |= FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
        break;
    case GraphicsOptions::ForegroundDefault:
        s_ApplyColors(pAttr, _wDefaultTextAttributes, true);
        break;
    case GraphicsOptions::BackgroundBlack:
        s_DisableAllColors(pAttr, false); // turn off all color flags first.
        break;
    case GraphicsOptions::BackgroundBlue:
        s_DisableAllColors(pAttr, false); // turn off all color flags first.
        *pAttr |= BACKGROUND_BLUE;
        break;
    case GraphicsOptions::BackgroundGreen:
        s_DisableAllColors(pAttr, false); // turn off all color flags first.
        *pAttr |= BACKGROUND_GREEN;
        break;
    case GraphicsOptions::BackgroundCyan:
        s_DisableAllColors(pAttr, false); // turn off all color flags first.
        *pAttr |= BACKGROUND_BLUE | BACKGROUND_GREEN;
        break;
    case GraphicsOptions::BackgroundRed:
        s_DisableAllColors(pAttr, false); // turn off all color flags first.
        *pAttr |= BACKGROUND_RED;
        break;
    case GraphicsOptions::BackgroundMagenta:
        s_DisableAllColors(pAttr, false); // turn off all color flags first.
        *pAttr |= BACKGROUND_BLUE | BACKGROUND_RED;
        break;
    case GraphicsOptions::BackgroundYellow:
        s_DisableAllColors(pAttr, false); // turn off all color flags first.
        *pAttr |= BACKGROUND_GREEN | BACKGROUND_RED;
        break;
    case GraphicsOptions::BackgroundWhite:
        s_DisableAllColors(pAttr, false); // turn off all color flags first.
        *pAttr |= BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED;
        break;
    case GraphicsOptions::BackgroundDefault:
        s_ApplyColors(pAttr, _wDefaultTextAttributes, false);
        break;
    case GraphicsOptions::BrightForegroundBlack:
        _SetGraphicsOptionHelper(GraphicsOptions::ForegroundBlack, pAttr);
        *pAttr |= FOREGROUND_INTENSITY;
        break;
    case GraphicsOptions::BrightForegroundBlue:
        _SetGraphicsOptionHelper(GraphicsOptions::ForegroundBlue, pAttr);
        *pAttr |= FOREGROUND_INTENSITY;
        break;
    case GraphicsOptions::BrightForegroundGreen:
        _SetGraphicsOptionHelper(GraphicsOptions::ForegroundGreen, pAttr);
        *pAttr |= FOREGROUND_INTENSITY;
        break;
    case GraphicsOptions::BrightForegroundCyan:
        _SetGraphicsOptionHelper(GraphicsOptions::ForegroundCyan, pAttr);
        *pAttr |= FOREGROUND_INTENSITY;
        break;
    case GraphicsOptions::BrightForegroundRed:
        _SetGraphicsOptionHelper(GraphicsOptions::ForegroundRed, pAttr);
        *pAttr |= FOREGROUND_INTENSITY;
        break;
    case GraphicsOptions::BrightForegroundMagenta:
        _SetGraphicsOptionHelper(GraphicsOptions::ForegroundMagenta, pAttr);
        *pAttr |= FOREGROUND_INTENSITY;
        break;
    case GraphicsOptions::BrightForegroundYellow:
        _SetGraphicsOptionHelper(GraphicsOptions::ForegroundYellow, pAttr);
        *pAttr |= FOREGROUND_INTENSITY;
        break;
    case GraphicsOptions::BrightForegroundWhite:
        _SetGraphicsOptionHelper(GraphicsOptions::ForegroundWhite, pAttr);
        *pAttr |= FOREGROUND_INTENSITY;
        break;
    case GraphicsOptions::BrightBackgroundBlack:
        _SetGraphicsOptionHelper(GraphicsOptions::BackgroundBlack, pAttr);
        *pAttr |= BACKGROUND_INTENSITY;
        break;
    case GraphicsOptions::BrightBackgroundBlue:
        _SetGraphicsOptionHelper(GraphicsOptions::BackgroundBlue, pAttr);
        *pAttr |= BACKGROUND_INTENSITY;
        break;
    case GraphicsOptions::BrightBackgroundGreen:
        _SetGraphicsOptionHelper(GraphicsOptions::BackgroundGreen, pAttr);
        *pAttr |= BACKGROUND_INTENSITY;
        break;
    case GraphicsOptions::BrightBackgroundCyan:
        _SetGraphicsOptionHelper(GraphicsOptions::BackgroundCyan, pAttr);
        *pAttr |= BACKGROUND_INTENSITY;
        break;
    case GraphicsOptions::BrightBackgroundRed:
        _SetGraphicsOptionHelper(GraphicsOptions::BackgroundRed, pAttr);
        *pAttr |= BACKGROUND_INTENSITY;
        break;
    case GraphicsOptions::BrightBackgroundMagenta:
        _SetGraphicsOptionHelper(GraphicsOptions::BackgroundMagenta, pAttr);
        *pAttr |= BACKGROUND_INTENSITY;
        break;
    case GraphicsOptions::BrightBackgroundYellow:
        _SetGraphicsOptionHelper(GraphicsOptions::BackgroundYellow, pAttr);
        *pAttr |= BACKGROUND_INTENSITY;
        break;
    case GraphicsOptions::BrightBackgroundWhite:
        _SetGraphicsOptionHelper(GraphicsOptions::BackgroundWhite, pAttr);
        *pAttr |= BACKGROUND_INTENSITY;
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
                          _Out_ size_t* const pcOptionsConsumed, 
                          _In_reads_(cColorTable) COLORREF* const ColorTable,
                          _In_ size_t const cColorTable)
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
            fSuccess = true;
        } 
        else if (typeOpt == GraphicsOptions::Xterm256Index && cOptions >= 3)
        {
            *pcOptionsConsumed = 3;
            if (rgOptions[2] <= 255) // ensure that the provided index is on the table
            {
                unsigned int tableIndex = rgOptions[2];
                if (tableIndex >= cColorTable)
                {
                    *prgbColor = s_XtermColorTable[tableIndex];
                }
                else // otherwise, we need to use the windows color table color.
                {
                    *prgbColor = ColorTable[s_TranslateXtermIndexToWindowsIndex(tableIndex)];
                }
                fSuccess = true;
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
                fSuccess = _SetRgbColorsHelper(&(rgOptions[i]), cOptions-i, &rgbColor, &fIsForeground, &cOptionsConsumed, csbiex.ColorTable, 16);
                if (fSuccess)
                {
                    // find the nearest ColorTable entry to the RGB color we found 
                    size_t ColorTableIndex = s_FindNearestTableIndex(rgbColor, csbiex.ColorTable, 16);

                    // translate the color table index into an SGR option
                    GraphicsOptions ConvertedOpt = (GraphicsOptions) s_TranslateIndexToGraphicsOption(ColorTableIndex, fIsForeground);
                    
                    _SetGraphicsOptionHelper(ConvertedOpt, &attr);
                }
                i += (cOptionsConsumed - 1); // cOptionsConsumed includes the opt we're currently on.
            }
            else
            {
                _SetGraphicsOptionHelper(opt, &attr);
            }
        }

        // Apply back into console
        fSuccess = !!_pConApi->SetConsoleTextAttribute(attr);
    }

    return fSuccess;
}