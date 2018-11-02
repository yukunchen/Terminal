/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "..\inc\conattrs.hpp"
#include "settings.hpp"

#include "..\interactivity\inc\ServiceLocator.hpp"

#pragma hdrstop

Settings::Settings() :
    _dwHotKey(0),
    _dwStartupFlags(0),
    _wFillAttribute(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE), // White (not bright) on black by default
    _wPopupFillAttribute(FOREGROUND_RED | FOREGROUND_BLUE | BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY), // Purple on white (bright) by default
    _wShowWindow(SW_SHOWNORMAL),
    _wReserved(0),
    // dwScreenBufferSize initialized below
    // dwWindowSize initialized below
    // dwWindowOrigin initialized below
    _nFont(0),
    // dwFontSize initialized below
    _uFontFamily(0),
    _uFontWeight(0),
    // FaceName initialized below
    _uCursorSize(CURSOR_SMALL_SIZE),
    _bFullScreen(false),
    _bQuickEdit(true),
    _bInsertMode(true),
    _bAutoPosition(true),
    _uHistoryBufferSize(DEFAULT_NUMBER_OF_COMMANDS),
    _uNumberOfHistoryBuffers(DEFAULT_NUMBER_OF_BUFFERS),
    _bHistoryNoDup(false),
    // ColorTable initialized below
    _uCodePage(ServiceLocator::LocateGlobals().uiOEMCP),
    _uScrollScale(1),
    _bLineSelection(true),
    _bWrapText(true),
    _fCtrlKeyShortcutsDisabled(false),
    _bWindowAlpha(BYTE_MAX), // 255 alpha = opaque. 0 = transparent.
    _fFilterOnPaste(false),
    _fTrimLeadingZeros(FALSE),
    _fEnableColorSelection(FALSE),
    _fAllowAltF4Close(true),
    _dwVirtTermLevel(0),
    _fUseWindowSizePixels(false),
    _fAutoReturnOnNewline(true), // the historic Windows behavior defaults this to on.
    _fRenderGridWorldwide(false), // historically grid lines were only rendered in DBCS codepages, so this is false by default unless otherwise specified.
    // window size pixels initialized below
    _fInterceptCopyPaste(0),
    _DefaultForeground(INVALID_COLOR),
    _DefaultBackground(INVALID_COLOR),
    _fUseDx(false)
{
    _dwScreenBufferSize.X = 80;
    _dwScreenBufferSize.Y = 25;

    _dwWindowSize.X = _dwScreenBufferSize.X;
    _dwWindowSize.Y = _dwScreenBufferSize.Y;

    _dwWindowSizePixels = { 0 };

    _dwWindowOrigin.X = 0;
    _dwWindowOrigin.Y = 0;

    _dwFontSize.X = 0;
    _dwFontSize.Y = 16;

    ZeroMemory((void*)&_FaceName, sizeof(_FaceName));
    wcscpy_s(_FaceName, DEFAULT_TT_FONT_FACENAME);

    ZeroMemory((void*)&_LaunchFaceName, sizeof(_LaunchFaceName));

    _CursorColor = Cursor::s_InvertCursorColor;
    _CursorType = CursorType::Legacy;

    _InitColorTable();

    _InitXtermTableValue(0,   0x00, 0x00, 0x00);
    _InitXtermTableValue(1,   0x80, 0x00, 0x00);
    _InitXtermTableValue(2,   0x00, 0x80, 0x00);
    _InitXtermTableValue(3,   0x80, 0x80, 0x00);
    _InitXtermTableValue(4,   0x00, 0x00, 0x80);
    _InitXtermTableValue(5,   0x80, 0x00, 0x80);
    _InitXtermTableValue(6,   0x00, 0x80, 0x80);
    _InitXtermTableValue(7,   0xc0, 0xc0, 0xc0);
    _InitXtermTableValue(8,   0x80, 0x80, 0x80);
    _InitXtermTableValue(9,   0xff, 0x00, 0x00);
    _InitXtermTableValue(10,  0x00, 0xff, 0x00);
    _InitXtermTableValue(11,  0xff, 0xff, 0x00);
    _InitXtermTableValue(12,  0x00, 0x00, 0xff);
    _InitXtermTableValue(13,  0xff, 0x00, 0xff);
    _InitXtermTableValue(14,  0x00, 0xff, 0xff);
    _InitXtermTableValue(15,  0xff, 0xff, 0xff);
    _InitXtermTableValue(16,  0x00, 0x00, 0x00);
    _InitXtermTableValue(17,  0x00, 0x00, 0x5f);
    _InitXtermTableValue(18,  0x00, 0x00, 0x87);
    _InitXtermTableValue(19,  0x00, 0x00, 0xaf);
    _InitXtermTableValue(20,  0x00, 0x00, 0xd7);
    _InitXtermTableValue(21,  0x00, 0x00, 0xff);
    _InitXtermTableValue(22,  0x00, 0x5f, 0x00);
    _InitXtermTableValue(23,  0x00, 0x5f, 0x5f);
    _InitXtermTableValue(24,  0x00, 0x5f, 0x87);
    _InitXtermTableValue(25,  0x00, 0x5f, 0xaf);
    _InitXtermTableValue(26,  0x00, 0x5f, 0xd7);
    _InitXtermTableValue(27,  0x00, 0x5f, 0xff);
    _InitXtermTableValue(28,  0x00, 0x87, 0x00);
    _InitXtermTableValue(29,  0x00, 0x87, 0x5f);
    _InitXtermTableValue(30,  0x00, 0x87, 0x87);
    _InitXtermTableValue(31,  0x00, 0x87, 0xaf);
    _InitXtermTableValue(32,  0x00, 0x87, 0xd7);
    _InitXtermTableValue(33,  0x00, 0x87, 0xff);
    _InitXtermTableValue(34,  0x00, 0xaf, 0x00);
    _InitXtermTableValue(35,  0x00, 0xaf, 0x5f);
    _InitXtermTableValue(36,  0x00, 0xaf, 0x87);
    _InitXtermTableValue(37,  0x00, 0xaf, 0xaf);
    _InitXtermTableValue(38,  0x00, 0xaf, 0xd7);
    _InitXtermTableValue(39,  0x00, 0xaf, 0xff);
    _InitXtermTableValue(40,  0x00, 0xd7, 0x00);
    _InitXtermTableValue(41,  0x00, 0xd7, 0x5f);
    _InitXtermTableValue(42,  0x00, 0xd7, 0x87);
    _InitXtermTableValue(43,  0x00, 0xd7, 0xaf);
    _InitXtermTableValue(44,  0x00, 0xd7, 0xd7);
    _InitXtermTableValue(45,  0x00, 0xd7, 0xff);
    _InitXtermTableValue(46,  0x00, 0xff, 0x00);
    _InitXtermTableValue(47,  0x00, 0xff, 0x5f);
    _InitXtermTableValue(48,  0x00, 0xff, 0x87);
    _InitXtermTableValue(49,  0x00, 0xff, 0xaf);
    _InitXtermTableValue(50,  0x00, 0xff, 0xd7);
    _InitXtermTableValue(51,  0x00, 0xff, 0xff);
    _InitXtermTableValue(52,  0x5f, 0x00, 0x00);
    _InitXtermTableValue(53,  0x5f, 0x00, 0x5f);
    _InitXtermTableValue(54,  0x5f, 0x00, 0x87);
    _InitXtermTableValue(55,  0x5f, 0x00, 0xaf);
    _InitXtermTableValue(56,  0x5f, 0x00, 0xd7);
    _InitXtermTableValue(57,  0x5f, 0x00, 0xff);
    _InitXtermTableValue(58,  0x5f, 0x5f, 0x00);
    _InitXtermTableValue(59,  0x5f, 0x5f, 0x5f);
    _InitXtermTableValue(60,  0x5f, 0x5f, 0x87);
    _InitXtermTableValue(61,  0x5f, 0x5f, 0xaf);
    _InitXtermTableValue(62,  0x5f, 0x5f, 0xd7);
    _InitXtermTableValue(63,  0x5f, 0x5f, 0xff);
    _InitXtermTableValue(64,  0x5f, 0x87, 0x00);
    _InitXtermTableValue(65,  0x5f, 0x87, 0x5f);
    _InitXtermTableValue(66,  0x5f, 0x87, 0x87);
    _InitXtermTableValue(67,  0x5f, 0x87, 0xaf);
    _InitXtermTableValue(68,  0x5f, 0x87, 0xd7);
    _InitXtermTableValue(69,  0x5f, 0x87, 0xff);
    _InitXtermTableValue(70,  0x5f, 0xaf, 0x00);
    _InitXtermTableValue(71,  0x5f, 0xaf, 0x5f);
    _InitXtermTableValue(72,  0x5f, 0xaf, 0x87);
    _InitXtermTableValue(73,  0x5f, 0xaf, 0xaf);
    _InitXtermTableValue(74,  0x5f, 0xaf, 0xd7);
    _InitXtermTableValue(75,  0x5f, 0xaf, 0xff);
    _InitXtermTableValue(76,  0x5f, 0xd7, 0x00);
    _InitXtermTableValue(77,  0x5f, 0xd7, 0x5f);
    _InitXtermTableValue(78,  0x5f, 0xd7, 0x87);
    _InitXtermTableValue(79,  0x5f, 0xd7, 0xaf);
    _InitXtermTableValue(80,  0x5f, 0xd7, 0xd7);
    _InitXtermTableValue(81,  0x5f, 0xd7, 0xff);
    _InitXtermTableValue(82,  0x5f, 0xff, 0x00);
    _InitXtermTableValue(83,  0x5f, 0xff, 0x5f);
    _InitXtermTableValue(84,  0x5f, 0xff, 0x87);
    _InitXtermTableValue(85,  0x5f, 0xff, 0xaf);
    _InitXtermTableValue(86,  0x5f, 0xff, 0xd7);
    _InitXtermTableValue(87,  0x5f, 0xff, 0xff);
    _InitXtermTableValue(88,  0x87, 0x00, 0x00);
    _InitXtermTableValue(89,  0x87, 0x00, 0x5f);
    _InitXtermTableValue(90,  0x87, 0x00, 0x87);
    _InitXtermTableValue(91,  0x87, 0x00, 0xaf);
    _InitXtermTableValue(92,  0x87, 0x00, 0xd7);
    _InitXtermTableValue(93,  0x87, 0x00, 0xff);
    _InitXtermTableValue(94,  0x87, 0x5f, 0x00);
    _InitXtermTableValue(95,  0x87, 0x5f, 0x5f);
    _InitXtermTableValue(96,  0x87, 0x5f, 0x87);
    _InitXtermTableValue(97,  0x87, 0x5f, 0xaf);
    _InitXtermTableValue(98,  0x87, 0x5f, 0xd7);
    _InitXtermTableValue(99,  0x87, 0x5f, 0xff);
    _InitXtermTableValue(100, 0x87, 0x87, 0x00);
    _InitXtermTableValue(101, 0x87, 0x87, 0x5f);
    _InitXtermTableValue(102, 0x87, 0x87, 0x87);
    _InitXtermTableValue(103, 0x87, 0x87, 0xaf);
    _InitXtermTableValue(104, 0x87, 0x87, 0xd7);
    _InitXtermTableValue(105, 0x87, 0x87, 0xff);
    _InitXtermTableValue(106, 0x87, 0xaf, 0x00);
    _InitXtermTableValue(107, 0x87, 0xaf, 0x5f);
    _InitXtermTableValue(108, 0x87, 0xaf, 0x87);
    _InitXtermTableValue(109, 0x87, 0xaf, 0xaf);
    _InitXtermTableValue(110, 0x87, 0xaf, 0xd7);
    _InitXtermTableValue(111, 0x87, 0xaf, 0xff);
    _InitXtermTableValue(112, 0x87, 0xd7, 0x00);
    _InitXtermTableValue(113, 0x87, 0xd7, 0x5f);
    _InitXtermTableValue(114, 0x87, 0xd7, 0x87);
    _InitXtermTableValue(115, 0x87, 0xd7, 0xaf);
    _InitXtermTableValue(116, 0x87, 0xd7, 0xd7);
    _InitXtermTableValue(117, 0x87, 0xd7, 0xff);
    _InitXtermTableValue(118, 0x87, 0xff, 0x00);
    _InitXtermTableValue(119, 0x87, 0xff, 0x5f);
    _InitXtermTableValue(120, 0x87, 0xff, 0x87);
    _InitXtermTableValue(121, 0x87, 0xff, 0xaf);
    _InitXtermTableValue(122, 0x87, 0xff, 0xd7);
    _InitXtermTableValue(123, 0x87, 0xff, 0xff);
    _InitXtermTableValue(124, 0xaf, 0x00, 0x00);
    _InitXtermTableValue(125, 0xaf, 0x00, 0x5f);
    _InitXtermTableValue(126, 0xaf, 0x00, 0x87);
    _InitXtermTableValue(127, 0xaf, 0x00, 0xaf);
    _InitXtermTableValue(128, 0xaf, 0x00, 0xd7);
    _InitXtermTableValue(129, 0xaf, 0x00, 0xff);
    _InitXtermTableValue(130, 0xaf, 0x5f, 0x00);
    _InitXtermTableValue(131, 0xaf, 0x5f, 0x5f);
    _InitXtermTableValue(132, 0xaf, 0x5f, 0x87);
    _InitXtermTableValue(133, 0xaf, 0x5f, 0xaf);
    _InitXtermTableValue(134, 0xaf, 0x5f, 0xd7);
    _InitXtermTableValue(135, 0xaf, 0x5f, 0xff);
    _InitXtermTableValue(136, 0xaf, 0x87, 0x00);
    _InitXtermTableValue(137, 0xaf, 0x87, 0x5f);
    _InitXtermTableValue(138, 0xaf, 0x87, 0x87);
    _InitXtermTableValue(139, 0xaf, 0x87, 0xaf);
    _InitXtermTableValue(140, 0xaf, 0x87, 0xd7);
    _InitXtermTableValue(141, 0xaf, 0x87, 0xff);
    _InitXtermTableValue(142, 0xaf, 0xaf, 0x00);
    _InitXtermTableValue(143, 0xaf, 0xaf, 0x5f);
    _InitXtermTableValue(144, 0xaf, 0xaf, 0x87);
    _InitXtermTableValue(145, 0xaf, 0xaf, 0xaf);
    _InitXtermTableValue(146, 0xaf, 0xaf, 0xd7);
    _InitXtermTableValue(147, 0xaf, 0xaf, 0xff);
    _InitXtermTableValue(148, 0xaf, 0xd7, 0x00);
    _InitXtermTableValue(149, 0xaf, 0xd7, 0x5f);
    _InitXtermTableValue(150, 0xaf, 0xd7, 0x87);
    _InitXtermTableValue(151, 0xaf, 0xd7, 0xaf);
    _InitXtermTableValue(152, 0xaf, 0xd7, 0xd7);
    _InitXtermTableValue(153, 0xaf, 0xd7, 0xff);
    _InitXtermTableValue(154, 0xaf, 0xff, 0x00);
    _InitXtermTableValue(155, 0xaf, 0xff, 0x5f);
    _InitXtermTableValue(156, 0xaf, 0xff, 0x87);
    _InitXtermTableValue(157, 0xaf, 0xff, 0xaf);
    _InitXtermTableValue(158, 0xaf, 0xff, 0xd7);
    _InitXtermTableValue(159, 0xaf, 0xff, 0xff);
    _InitXtermTableValue(160, 0xd7, 0x00, 0x00);
    _InitXtermTableValue(161, 0xd7, 0x00, 0x5f);
    _InitXtermTableValue(162, 0xd7, 0x00, 0x87);
    _InitXtermTableValue(163, 0xd7, 0x00, 0xaf);
    _InitXtermTableValue(164, 0xd7, 0x00, 0xd7);
    _InitXtermTableValue(165, 0xd7, 0x00, 0xff);
    _InitXtermTableValue(166, 0xd7, 0x5f, 0x00);
    _InitXtermTableValue(167, 0xd7, 0x5f, 0x5f);
    _InitXtermTableValue(168, 0xd7, 0x5f, 0x87);
    _InitXtermTableValue(169, 0xd7, 0x5f, 0xaf);
    _InitXtermTableValue(170, 0xd7, 0x5f, 0xd7);
    _InitXtermTableValue(171, 0xd7, 0x5f, 0xff);
    _InitXtermTableValue(172, 0xd7, 0x87, 0x00);
    _InitXtermTableValue(173, 0xd7, 0x87, 0x5f);
    _InitXtermTableValue(174, 0xd7, 0x87, 0x87);
    _InitXtermTableValue(175, 0xd7, 0x87, 0xaf);
    _InitXtermTableValue(176, 0xd7, 0x87, 0xd7);
    _InitXtermTableValue(177, 0xd7, 0x87, 0xff);
    _InitXtermTableValue(178, 0xdf, 0xaf, 0x00);
    _InitXtermTableValue(179, 0xdf, 0xaf, 0x5f);
    _InitXtermTableValue(180, 0xdf, 0xaf, 0x87);
    _InitXtermTableValue(181, 0xdf, 0xaf, 0xaf);
    _InitXtermTableValue(182, 0xdf, 0xaf, 0xd7);
    _InitXtermTableValue(183, 0xdf, 0xaf, 0xff);
    _InitXtermTableValue(184, 0xdf, 0xd7, 0x00);
    _InitXtermTableValue(185, 0xdf, 0xd7, 0x5f);
    _InitXtermTableValue(186, 0xdf, 0xd7, 0x87);
    _InitXtermTableValue(187, 0xdf, 0xd7, 0xaf);
    _InitXtermTableValue(188, 0xdf, 0xd7, 0xd7);
    _InitXtermTableValue(189, 0xdf, 0xd7, 0xff);
    _InitXtermTableValue(190, 0xdf, 0xff, 0x00);
    _InitXtermTableValue(191, 0xdf, 0xff, 0x5f);
    _InitXtermTableValue(192, 0xdf, 0xff, 0x87);
    _InitXtermTableValue(193, 0xdf, 0xff, 0xaf);
    _InitXtermTableValue(194, 0xdf, 0xff, 0xd7);
    _InitXtermTableValue(195, 0xdf, 0xff, 0xff);
    _InitXtermTableValue(196, 0xff, 0x00, 0x00);
    _InitXtermTableValue(197, 0xff, 0x00, 0x5f);
    _InitXtermTableValue(198, 0xff, 0x00, 0x87);
    _InitXtermTableValue(199, 0xff, 0x00, 0xaf);
    _InitXtermTableValue(200, 0xff, 0x00, 0xd7);
    _InitXtermTableValue(201, 0xff, 0x00, 0xff);
    _InitXtermTableValue(202, 0xff, 0x5f, 0x00);
    _InitXtermTableValue(203, 0xff, 0x5f, 0x5f);
    _InitXtermTableValue(204, 0xff, 0x5f, 0x87);
    _InitXtermTableValue(205, 0xff, 0x5f, 0xaf);
    _InitXtermTableValue(206, 0xff, 0x5f, 0xd7);
    _InitXtermTableValue(207, 0xff, 0x5f, 0xff);
    _InitXtermTableValue(208, 0xff, 0x87, 0x00);
    _InitXtermTableValue(209, 0xff, 0x87, 0x5f);
    _InitXtermTableValue(210, 0xff, 0x87, 0x87);
    _InitXtermTableValue(211, 0xff, 0x87, 0xaf);
    _InitXtermTableValue(212, 0xff, 0x87, 0xd7);
    _InitXtermTableValue(213, 0xff, 0x87, 0xff);
    _InitXtermTableValue(214, 0xff, 0xaf, 0x00);
    _InitXtermTableValue(215, 0xff, 0xaf, 0x5f);
    _InitXtermTableValue(216, 0xff, 0xaf, 0x87);
    _InitXtermTableValue(217, 0xff, 0xaf, 0xaf);
    _InitXtermTableValue(218, 0xff, 0xaf, 0xd7);
    _InitXtermTableValue(219, 0xff, 0xaf, 0xff);
    _InitXtermTableValue(220, 0xff, 0xd7, 0x00);
    _InitXtermTableValue(221, 0xff, 0xd7, 0x5f);
    _InitXtermTableValue(222, 0xff, 0xd7, 0x87);
    _InitXtermTableValue(223, 0xff, 0xd7, 0xaf);
    _InitXtermTableValue(224, 0xff, 0xd7, 0xd7);
    _InitXtermTableValue(225, 0xff, 0xd7, 0xff);
    _InitXtermTableValue(226, 0xff, 0xff, 0x00);
    _InitXtermTableValue(227, 0xff, 0xff, 0x5f);
    _InitXtermTableValue(228, 0xff, 0xff, 0x87);
    _InitXtermTableValue(229, 0xff, 0xff, 0xaf);
    _InitXtermTableValue(230, 0xff, 0xff, 0xd7);
    _InitXtermTableValue(231, 0xff, 0xff, 0xff);
    _InitXtermTableValue(232, 0x08, 0x08, 0x08);
    _InitXtermTableValue(233, 0x12, 0x12, 0x12);
    _InitXtermTableValue(234, 0x1c, 0x1c, 0x1c);
    _InitXtermTableValue(235, 0x26, 0x26, 0x26);
    _InitXtermTableValue(236, 0x30, 0x30, 0x30);
    _InitXtermTableValue(237, 0x3a, 0x3a, 0x3a);
    _InitXtermTableValue(238, 0x44, 0x44, 0x44);
    _InitXtermTableValue(239, 0x4e, 0x4e, 0x4e);
    _InitXtermTableValue(240, 0x58, 0x58, 0x58);
    _InitXtermTableValue(241, 0x62, 0x62, 0x62);
    _InitXtermTableValue(242, 0x6c, 0x6c, 0x6c);
    _InitXtermTableValue(243, 0x76, 0x76, 0x76);
    _InitXtermTableValue(244, 0x80, 0x80, 0x80);
    _InitXtermTableValue(245, 0x8a, 0x8a, 0x8a);
    _InitXtermTableValue(246, 0x94, 0x94, 0x94);
    _InitXtermTableValue(247, 0x9e, 0x9e, 0x9e);
    _InitXtermTableValue(248, 0xa8, 0xa8, 0xa8);
    _InitXtermTableValue(249, 0xb2, 0xb2, 0xb2);
    _InitXtermTableValue(250, 0xbc, 0xbc, 0xbc);
    _InitXtermTableValue(251, 0xc6, 0xc6, 0xc6);
    _InitXtermTableValue(252, 0xd0, 0xd0, 0xd0);
    _InitXtermTableValue(253, 0xda, 0xda, 0xda);
    _InitXtermTableValue(254, 0xe4, 0xe4, 0xe4);
    _InitXtermTableValue(255, 0xee, 0xee, 0xee);
}

void Settings::_InitXtermTableValue(const size_t iIndex, const byte bRed, const byte bGreen, const byte bBlue)
{
    _XtermColorTable[iIndex] = RGB(bRed, bGreen, bBlue);
}

// Routine Description:
// - Assigns default colors to the color table. These match the manifest file for the registry.
// Arguments:
// - <none> - Operates on internal state
// Return Value:
// - <none>
void Settings::_InitColorTable()
{
    // Default hardcoded colors for use in console. These are used when there are no overriding colors to load from the
    // registry or shortcut files.
    _ColorTable[0] = RGB(12, 12, 12); // Black
    _ColorTable[1] = RGB(0, 55, 218); // Dark Blue
    _ColorTable[2] = RGB(19, 161, 14); // Dark Green
    _ColorTable[3] = RGB(58, 150, 221); // Dark Cyan
    _ColorTable[4] = RGB(197, 15, 31); // Dark Red
    _ColorTable[5] = RGB(136, 23, 152); // Dark Magenta
    _ColorTable[6] = RGB(193, 156, 0); // Dark Yellow
    _ColorTable[7] = RGB(204, 204, 204); // Dark White
    _ColorTable[8] = RGB(118, 118, 118); // Bright Black
    _ColorTable[9] = RGB(59, 120, 255); // Bright Blue
    _ColorTable[10] = RGB(22, 198, 12); // Bright Green
    _ColorTable[11] = RGB(97, 214, 214); // Bright Cyan
    _ColorTable[12] = RGB(231, 72, 86); // Bright Red
    _ColorTable[13] = RGB(180, 0, 158); // Bright Magenta
    _ColorTable[14] = RGB(249, 241, 165); // Bright Yellow
    _ColorTable[15] = RGB(242, 242, 242); // White
}

// Routine Description:
// - Applies hardcoded default settings that are in line with what is defined
//   in our Windows edition manifest (living in win32k-settings.man).
// - NOTE: This exists in case we cannot access the registry on desktop platforms.
//   We will use this to provide better defaults than the constructor values which
//   are optimized for OneCore.
// Arguments:
// - <none>
// Return Value:
// - <none> - Adjusts internal state only.
void Settings::ApplyDesktopSpecificDefaults()
{
    _dwFontSize.X = 0;
    _dwFontSize.Y = 16;
    _uFontFamily = 0;
    _dwScreenBufferSize.X = 120;
    _dwScreenBufferSize.Y = 9001;
    _uCursorSize = 25;
    _dwWindowSize.X = 120;
    _dwWindowSize.Y = 30;
    _wFillAttribute = 0x7;
    _wPopupFillAttribute = 0xf5;
    wcscpy_s(_FaceName, L"__DefaultTTFont__");
    _uFontWeight = 0;
    _bInsertMode = TRUE;
    _bFullScreen = FALSE;
    _fCtrlKeyShortcutsDisabled = false;
    _bWrapText = true;
    _bLineSelection = TRUE;
    _bWindowAlpha = 255;
    _fFilterOnPaste = TRUE;
    _bQuickEdit = TRUE;
    _uHistoryBufferSize = 50;
    _uNumberOfHistoryBuffers = 4;
    _bHistoryNoDup = FALSE;

    _InitColorTable();

    _fTrimLeadingZeros = false;
    _fEnableColorSelection = false;
    _uScrollScale = 1;
}

void Settings::ApplyStartupInfo(const Settings* const pStartupSettings)
{
    const DWORD dwFlags = pStartupSettings->_dwStartupFlags;

    // See: http://msdn.microsoft.com/en-us/library/windows/desktop/ms686331(v=vs.85).aspx

    // Note: These attributes do not get sent to us if we started conhost
    //      directly.  See minkernel/console/client/dllinit for the
    //      initialization of these values for cmdline applications.

    if (WI_IsFlagSet(dwFlags, STARTF_USECOUNTCHARS))
    {
        _dwScreenBufferSize = pStartupSettings->_dwScreenBufferSize;
    }

    if (WI_IsFlagSet(dwFlags, STARTF_USESIZE))
    {
        // WARNING: This size is in pixels when passed in the create process call.
        // It will need to be divided by the font size before use.
        // All other Window Size values (from registry/shortcuts) are stored in characters.
        _dwWindowSizePixels = pStartupSettings->_dwWindowSize;
        _fUseWindowSizePixels = true;
    }

    if (WI_IsFlagSet(dwFlags, STARTF_USEPOSITION))
    {
        _dwWindowOrigin = pStartupSettings->_dwWindowOrigin;
        _bAutoPosition = FALSE;
    }

    if (WI_IsFlagSet(dwFlags, STARTF_USEFILLATTRIBUTE))
    {
        _wFillAttribute = pStartupSettings->_wFillAttribute;
    }

    if (WI_IsFlagSet(dwFlags, STARTF_USESHOWWINDOW))
    {
        _wShowWindow = pStartupSettings->_wShowWindow;
    }
}

// Method Description:
// - Applies settings passed on the commandline to this settings structure.
//      Currently, the only settings that can be passed on the commandline are
//      the initial width and height of the screenbuffer/viewport.
// Arguments:
// - consoleArgs: A reference to a parsed command-line args object.
// Return Value:
// - <none>
void Settings::ApplyCommandlineArguments(const ConsoleArguments& consoleArgs)
{
    const short width = consoleArgs.GetWidth();
    const short height = consoleArgs.GetHeight();

    if (width > 0 && height > 0)
    {
        _dwScreenBufferSize.X = width;
        _dwWindowSize.X = width;

        _dwScreenBufferSize.Y = height;
        _dwWindowSize.Y = height;
    }
    else if (ServiceLocator::LocateGlobals().getConsoleInformation().IsInVtIoMode())
    {
        // If we're a PTY but we weren't explicitly told a size, use the window size as the buffer size.
        _dwScreenBufferSize = _dwWindowSize;
    }
}

// WARNING: this function doesn't perform any validation or conversion
void Settings::InitFromStateInfo(_In_ PCONSOLE_STATE_INFO pStateInfo)
{
    _wFillAttribute = pStateInfo->ScreenAttributes;
    _wPopupFillAttribute = pStateInfo->PopupAttributes;
    _dwScreenBufferSize = pStateInfo->ScreenBufferSize;
    _dwWindowSize = pStateInfo->WindowSize;
    _dwWindowOrigin.X = (SHORT)pStateInfo->WindowPosX;
    _dwWindowOrigin.Y = (SHORT)pStateInfo->WindowPosY;
    _dwFontSize = pStateInfo->FontSize;
    _uFontFamily = pStateInfo->FontFamily;
    _uFontWeight = pStateInfo->FontWeight;
    StringCchCopyW(_FaceName, ARRAYSIZE(_FaceName), pStateInfo->FaceName);
    _uCursorSize = pStateInfo->CursorSize;
    _bFullScreen = pStateInfo->FullScreen;
    _bQuickEdit = pStateInfo->QuickEdit;
    _bAutoPosition = pStateInfo->AutoPosition;
    _bInsertMode = pStateInfo->InsertMode;
    _bHistoryNoDup = pStateInfo->HistoryNoDup;
    _uHistoryBufferSize = pStateInfo->HistoryBufferSize;
    _uNumberOfHistoryBuffers = pStateInfo->NumberOfHistoryBuffers;
    memmove(_ColorTable, pStateInfo->ColorTable, sizeof(_ColorTable));
    _uCodePage = pStateInfo->CodePage;
    _bWrapText = !!pStateInfo->fWrapText;
    _fFilterOnPaste = pStateInfo->fFilterOnPaste;
    _fCtrlKeyShortcutsDisabled = pStateInfo->fCtrlKeyShortcutsDisabled;
    _bLineSelection = pStateInfo->fLineSelection;
    _CursorColor = pStateInfo->CursorColor;
    _CursorType = static_cast<CursorType>(pStateInfo->CursorType);
    _fInterceptCopyPaste = pStateInfo->InterceptCopyPaste;
    _DefaultForeground = pStateInfo->DefaultForeground;
    _DefaultBackground = pStateInfo->DefaultBackground;
    _TerminalScrolling = pStateInfo->TerminalScrolling;
}

// Method Description:
// - Create a CONSOLE_STATE_INFO with the current state of this settings structure.
// Arguments:
// - <none>
// Return Value:
// - a CONSOLE_STATE_INFO with the current state of this settings structure.
CONSOLE_STATE_INFO Settings::CreateConsoleStateInfo() const
{
    CONSOLE_STATE_INFO csi = {0};
    csi.ScreenAttributes = _wFillAttribute;
    csi.PopupAttributes = _wPopupFillAttribute;
    csi.ScreenBufferSize = _dwScreenBufferSize;
    csi.WindowSize = _dwWindowSize;
    csi.WindowPosX = (SHORT)_dwWindowOrigin.X;
    csi.WindowPosY = (SHORT)_dwWindowOrigin.Y;
    csi.FontSize = _dwFontSize;
    csi.FontFamily = _uFontFamily;
    csi.FontWeight = _uFontWeight;
    StringCchCopyW(csi.FaceName, ARRAYSIZE(_FaceName), _FaceName);
    csi.CursorSize = _uCursorSize;
    csi.FullScreen = _bFullScreen;
    csi.QuickEdit = _bQuickEdit;
    csi.AutoPosition = _bAutoPosition;
    csi.InsertMode = _bInsertMode;
    csi.HistoryNoDup = _bHistoryNoDup;
    csi.HistoryBufferSize = _uHistoryBufferSize;
    csi.NumberOfHistoryBuffers = _uNumberOfHistoryBuffers;
    memmove(csi.ColorTable, _ColorTable, sizeof(_ColorTable));
    csi.CodePage = _uCodePage;
    csi.fWrapText = !!_bWrapText;
    csi.fFilterOnPaste = _fFilterOnPaste;
    csi.fCtrlKeyShortcutsDisabled = _fCtrlKeyShortcutsDisabled;
    csi.fLineSelection = _bLineSelection;
    csi.bWindowTransparency = _bWindowAlpha;
    csi.CursorColor = _CursorColor;
    csi.CursorType = static_cast<unsigned int>(_CursorType);
    csi.InterceptCopyPaste = _fInterceptCopyPaste;
    csi.DefaultForeground = _DefaultForeground;
    csi.DefaultBackground = _DefaultBackground;
    csi.TerminalScrolling = _TerminalScrolling;
    return csi;
}


void Settings::Validate()
{
    // If we were explicitly given a size in pixels from the startup info, divide by the font to turn it into characters.
    // See: https://msdn.microsoft.com/en-us/library/windows/desktop/ms686331%28v=vs.85%29.aspx
    if (WI_IsFlagSet(_dwStartupFlags, STARTF_USESIZE))
    {
        // TODO: FIX
        //// Get the font that we're going to use to convert pixels to characters.
        //DWORD const dwFontIndexWant = FindCreateFont(_uFontFamily,
        //                                             _FaceName,
        //                                             _dwFontSize,
        //                                             _uFontWeight,
        //                                             _uCodePage);

        //_dwWindowSize.X /= g_pfiFontInfo[dwFontIndexWant].Size.X;
        //_dwWindowSize.Y /= g_pfiFontInfo[dwFontIndexWant].Size.Y;
    }

    // minimum screen buffer size 1x1
    _dwScreenBufferSize.X = std::max(_dwScreenBufferSize.X, 1i16);
    _dwScreenBufferSize.Y = std::max(_dwScreenBufferSize.Y, 1i16);

    // minimum window size size 1x1
    _dwWindowSize.X = std::max(_dwWindowSize.X, 1i16);
    _dwWindowSize.Y = std::max(_dwWindowSize.Y, 1i16);

    // if buffer size is less than window size, increase buffer size to meet window size
    _dwScreenBufferSize.X = std::max(_dwWindowSize.X, _dwScreenBufferSize.X);
    _dwScreenBufferSize.Y = std::max(_dwWindowSize.Y, _dwScreenBufferSize.Y);

    // ensure that the window alpha value is not below the minimum. (no invisible windows)
    // if it's below minimum, just set it to the opaque value
    if (_bWindowAlpha < MIN_WINDOW_OPACITY)
    {
        _bWindowAlpha = BYTE_MAX;
    }

    // If text wrapping is on, ensure that the window width is the same as the buffer width.
    if (_bWrapText)
    {
        _dwWindowSize.X = _dwScreenBufferSize.X;
    }

    // Ensure that our fill attributes only contain colors and not any box drawing or invert attributes.
    WI_ClearAllFlags(_wFillAttribute, ~(FG_ATTRS | BG_ATTRS));
    WI_ClearAllFlags(_wPopupFillAttribute, ~(FG_ATTRS | BG_ATTRS));

    FAIL_FAST_IF(!(_dwWindowSize.X > 0));
    FAIL_FAST_IF(!(_dwWindowSize.Y > 0));
    FAIL_FAST_IF(!(_dwScreenBufferSize.X > 0));
    FAIL_FAST_IF(!(_dwScreenBufferSize.Y > 0));
}

DWORD Settings::GetVirtTermLevel() const
{
    return _dwVirtTermLevel;
}
void Settings::SetVirtTermLevel(const DWORD dwVirtTermLevel)
{
    _dwVirtTermLevel = dwVirtTermLevel;
}

bool Settings::IsAltF4CloseAllowed() const
{
    return _fAllowAltF4Close;
}
void Settings::SetAltF4CloseAllowed(const bool fAllowAltF4Close)
{
    _fAllowAltF4Close = fAllowAltF4Close;
}

bool Settings::IsReturnOnNewlineAutomatic() const
{
    return _fAutoReturnOnNewline;
}
void Settings::SetAutomaticReturnOnNewline(const bool fAutoReturnOnNewline)
{
    _fAutoReturnOnNewline = fAutoReturnOnNewline;
}

bool Settings::IsGridRenderingAllowedWorldwide() const
{
    return _fRenderGridWorldwide;
}
void Settings::SetGridRenderingAllowedWorldwide(const bool fGridRenderingAllowed)
{
    // Only trigger a notification and update the status if something has changed.
    if (_fRenderGridWorldwide != fGridRenderingAllowed)
    {
        _fRenderGridWorldwide = fGridRenderingAllowed;

        if (ServiceLocator::LocateGlobals().pRender != nullptr)
        {
            ServiceLocator::LocateGlobals().pRender->TriggerRedrawAll();
        }
    }
}

bool Settings::GetFilterOnPaste() const
{
    return _fFilterOnPaste;
}
void Settings::SetFilterOnPaste(const bool fFilterOnPaste)
{
    _fFilterOnPaste = fFilterOnPaste;
}

const WCHAR* const Settings::GetLaunchFaceName() const
{
    return _LaunchFaceName;
}
void Settings::SetLaunchFaceName(_In_ PCWSTR const LaunchFaceName, const size_t cchLength)
{
    StringCchCopyW(_LaunchFaceName, cchLength, LaunchFaceName);
}

UINT Settings::GetCodePage() const
{
    return _uCodePage;
}
void Settings::SetCodePage(const UINT uCodePage)
{
    _uCodePage = uCodePage;
}

UINT Settings::GetScrollScale() const
{
    return _uScrollScale;
}
void Settings::SetScrollScale(const UINT uScrollScale)
{
    _uScrollScale = uScrollScale;
}

bool Settings::GetTrimLeadingZeros() const
{
    return _fTrimLeadingZeros;
}
void Settings::SetTrimLeadingZeros(const bool fTrimLeadingZeros)
{
    _fTrimLeadingZeros = fTrimLeadingZeros;
}

bool Settings::GetEnableColorSelection() const
{
    return _fEnableColorSelection;
}
void Settings::SetEnableColorSelection(const bool fEnableColorSelection)
{
    _fEnableColorSelection = fEnableColorSelection;
}

bool Settings::GetLineSelection() const
{
    return _bLineSelection;
}
void Settings::SetLineSelection(const bool bLineSelection)
{
    _bLineSelection = bLineSelection;
}

bool Settings::GetWrapText () const
{
    return _bWrapText;
}
void Settings::SetWrapText (const bool bWrapText )
{
    _bWrapText = bWrapText;
}

bool Settings::GetCtrlKeyShortcutsDisabled () const
{
    return _fCtrlKeyShortcutsDisabled;
}
void Settings::SetCtrlKeyShortcutsDisabled (const bool fCtrlKeyShortcutsDisabled )
{
    _fCtrlKeyShortcutsDisabled = fCtrlKeyShortcutsDisabled;
}

BYTE Settings::GetWindowAlpha() const
{
    return _bWindowAlpha;
}
void Settings::SetWindowAlpha(const BYTE bWindowAlpha)
{
    // if we're out of bounds, set it to 100% opacity so it appears as if nothing happened.
    _bWindowAlpha = (bWindowAlpha < MIN_WINDOW_OPACITY)? BYTE_MAX : bWindowAlpha;
}

DWORD Settings::GetHotKey() const
{
    return _dwHotKey;
}
void Settings::SetHotKey(const DWORD dwHotKey)
{
    _dwHotKey = dwHotKey;
}

DWORD Settings::GetStartupFlags() const
{
    return _dwStartupFlags;
}
void Settings::SetStartupFlags(const DWORD dwStartupFlags)
{
    _dwStartupFlags = dwStartupFlags;
}

WORD Settings::GetFillAttribute() const
{
    return _wFillAttribute;
}
void Settings::SetFillAttribute(const WORD wFillAttribute)
{
    _wFillAttribute = wFillAttribute;

    // Do not allow the default fill attribute to use any attrs other than fg/bg colors.
    // This prevents us from accidentally inverting everything or suddenly drawing lines
    // everywhere by default.
    WI_ClearAllFlags(_wFillAttribute, ~(FG_ATTRS | BG_ATTRS));
}

WORD Settings::GetPopupFillAttribute() const
{
    return _wPopupFillAttribute;
}
void Settings::SetPopupFillAttribute(const WORD wPopupFillAttribute)
{
    _wPopupFillAttribute = wPopupFillAttribute;

    // Do not allow the default popup fill attribute to use any attrs other than fg/bg colors.
    // This prevents us from accidentally inverting everything or suddenly drawing lines
    // everywhere by defualt.
    WI_ClearAllFlags(_wPopupFillAttribute, ~(FG_ATTRS | BG_ATTRS));
}

WORD Settings::GetShowWindow() const
{
    return _wShowWindow;
}
void Settings::SetShowWindow(const WORD wShowWindow)
{
    _wShowWindow = wShowWindow;
}

WORD Settings::GetReserved() const
{
    return _wReserved;
}
void Settings::SetReserved(const WORD wReserved)
{
    _wReserved = wReserved;
}

COORD Settings::GetScreenBufferSize() const
{
    return _dwScreenBufferSize;
}
void Settings::SetScreenBufferSize(const COORD dwScreenBufferSize)
{
    _dwScreenBufferSize = dwScreenBufferSize;
}

COORD Settings::GetWindowSize() const
{
    return _dwWindowSize;
}
void Settings::SetWindowSize(const COORD dwWindowSize)
{
    _dwWindowSize = dwWindowSize;
}

bool Settings::IsWindowSizePixelsValid() const
{
    return _fUseWindowSizePixels;
}
COORD Settings::GetWindowSizePixels() const
{
    return _dwWindowSizePixels;
}
void Settings::SetWindowSizePixels(const COORD dwWindowSizePixels)
{
    _dwWindowSizePixels = dwWindowSizePixels;
}

COORD Settings::GetWindowOrigin() const
{
    return _dwWindowOrigin;
}
void Settings::SetWindowOrigin(const COORD dwWindowOrigin)
{
    _dwWindowOrigin = dwWindowOrigin;
}

DWORD Settings::GetFont() const
{
    return _nFont;
}
void Settings::SetFont(const DWORD nFont)
{
    _nFont = nFont;
}

COORD Settings::GetFontSize() const
{
    return _dwFontSize;
}
void Settings::SetFontSize(const COORD dwFontSize)
{
    _dwFontSize = dwFontSize;
}

UINT Settings::GetFontFamily() const
{
    return _uFontFamily;
}
void Settings::SetFontFamily(const UINT uFontFamily)
{
    _uFontFamily = uFontFamily;
}

UINT Settings::GetFontWeight() const
{
    return _uFontWeight;
}
void Settings::SetFontWeight(const UINT uFontWeight)
{
    _uFontWeight = uFontWeight;
}

const WCHAR* const Settings::GetFaceName() const
{
    return _FaceName;
}
void Settings::SetFaceName(_In_ PCWSTR const pcszFaceName, const size_t cchLength)
{
    StringCchCopyW(_FaceName, cchLength, pcszFaceName);
}

UINT Settings::GetCursorSize() const
{
    return _uCursorSize;
}
void Settings::SetCursorSize(const UINT uCursorSize)
{
    _uCursorSize = uCursorSize;
}

bool Settings::GetFullScreen() const
{
    return _bFullScreen;
}
void Settings::SetFullScreen(const bool bFullScreen)
{
    _bFullScreen = bFullScreen;
}

bool Settings::GetQuickEdit() const
{
    return _bQuickEdit;
}
void Settings::SetQuickEdit(const bool bQuickEdit)
{
    _bQuickEdit = bQuickEdit;
}

bool Settings::GetInsertMode() const
{
    return _bInsertMode;
}
void Settings::SetInsertMode(const bool bInsertMode)
{
    _bInsertMode = bInsertMode;
}

bool Settings::GetAutoPosition() const
{
    return _bAutoPosition;
}
void Settings::SetAutoPosition(const bool bAutoPosition)
{
    _bAutoPosition = bAutoPosition;
}

UINT Settings::GetHistoryBufferSize() const
{
    return _uHistoryBufferSize;
}
void Settings::SetHistoryBufferSize(const UINT uHistoryBufferSize)
{
    _uHistoryBufferSize = uHistoryBufferSize;
}

UINT Settings::GetNumberOfHistoryBuffers() const
{
    return _uNumberOfHistoryBuffers;
}
void Settings::SetNumberOfHistoryBuffers(const UINT uNumberOfHistoryBuffers)
{
    _uNumberOfHistoryBuffers = uNumberOfHistoryBuffers;
}

bool Settings::GetHistoryNoDup() const
{
    return _bHistoryNoDup;
}
void Settings::SetHistoryNoDup(const bool bHistoryNoDup)
{
    _bHistoryNoDup = bHistoryNoDup;
}

const COLORREF* const Settings::GetColorTable() const
{
    return _ColorTable;
}
void Settings::SetColorTable(_In_reads_(cSize) const COLORREF* const pColorTable, const size_t cSize)
{
    size_t cSizeWritten = std::min(cSize, static_cast<size_t>(COLOR_TABLE_SIZE));

    memmove(_ColorTable, pColorTable, cSizeWritten * sizeof(COLORREF));
}
void Settings::SetColorTableEntry(const size_t index, const COLORREF ColorValue)
{
    if (index < ARRAYSIZE(_ColorTable))
    {
        _ColorTable[index] = ColorValue;
    }
    else
    {
        _XtermColorTable[index] = ColorValue;
    }
}

bool Settings::IsStartupTitleIsLinkNameSet() const
{
    return WI_IsFlagSet(_dwStartupFlags, STARTF_TITLEISLINKNAME);
}

bool Settings::IsFaceNameSet() const
{
    return GetFaceName()[0] != '\0';
}

void Settings::UnsetStartupFlag(const DWORD dwFlagToUnset)
{
    _dwStartupFlags &= ~dwFlagToUnset;
}

const size_t Settings::GetColorTableSize() const
{
    return ARRAYSIZE(_ColorTable);
}

COLORREF Settings::GetColorTableEntry(const size_t index) const
{
    if (index < ARRAYSIZE(_ColorTable))
    {
        return _ColorTable[index];
    }
    else
    {
        return _XtermColorTable[index];
    }
}


// Routine Description:
// - Generates a legacy attribute from the given TextAttributes.
//     This needs to be a method on the Settings because the generated index
//     is dependent upon the particular values of the color table at the time of reading.
// Parameters:
// - attributes - The TextAttributes to generate a legacy attribute for.
// Return value:
// - A WORD representing the entry in the color table that most closely represents the given fullcolor attributes.
WORD Settings::GenerateLegacyAttributes(const TextAttribute attributes) const
{
    const WORD wLegacyOriginal = attributes.GetLegacyAttributes();
    if (attributes.IsLegacy())
    {
        return wLegacyOriginal;
    }
    // Get the Line drawing attributes and stash those, we'll need to preserve them.
    const WORD wNonColorAttributes = wLegacyOriginal & (~0xFF);
    const COLORREF rgbForeground = attributes.GetRgbForeground();
    const COLORREF rgbBackground = attributes.GetRgbBackground();
    const WORD wForegroundIndex = FindNearestTableIndex(rgbForeground);
    const WORD wBackgroundIndex = FindNearestTableIndex(rgbBackground);
    const WORD wCompleteAttr = (wNonColorAttributes) | (wBackgroundIndex << 4) | (wForegroundIndex);
    return wCompleteAttr;
}

//Routine Description:
// For a given RGB color Color, finds the nearest color from the array ColorTable, and returns the index of that match.
//Arguments:
// - Color - The RGB color to fine the nearest color to.
// - ColorTable - The array of colors to find a nearest color from.
// - cColorTable - The number of elements in ColorTable
// Return value:
// The index in ColorTable of the nearest match to Color.
WORD Settings::FindNearestTableIndex(const COLORREF Color) const
{
    return ::FindNearestTableIndex(Color, _ColorTable, ARRAYSIZE(_ColorTable));
}

COLORREF Settings::GetCursorColor() const noexcept
{
    return _CursorColor;
}

CursorType Settings::GetCursorType() const noexcept
{
    return _CursorType;
}

void Settings::SetCursorColor(const COLORREF CursorColor) noexcept
{
    _CursorColor = CursorColor;
}

void Settings::SetCursorType(const CursorType cursorType) noexcept
{
    _CursorType = cursorType;
}

bool Settings::GetInterceptCopyPaste() const noexcept
{
    return _fInterceptCopyPaste;
}

void Settings::SetInterceptCopyPaste(const bool interceptCopyPaste) noexcept
{
    _fInterceptCopyPaste = interceptCopyPaste;
}

COLORREF Settings::GetDefaultForegroundColor() const noexcept
{
    return _DefaultForeground;
}

void Settings::SetDefaultForegroundColor(const COLORREF defaultForeground) noexcept
{
    _DefaultForeground = defaultForeground;
}

COLORREF Settings::GetDefaultBackgroundColor() const noexcept
{
    return _DefaultBackground;
}

void Settings::SetDefaultBackgroundColor(const COLORREF defaultBackground) noexcept
{
    _DefaultBackground = defaultBackground;
}

TextAttribute Settings::GetDefaultAttributes() const noexcept
{
    TextAttribute attrs(_wFillAttribute);
    if (_DefaultForeground != INVALID_COLOR)
    {
        attrs.SetDefaultForeground(_DefaultForeground, _wFillAttribute);
    }
    if (_DefaultBackground != INVALID_COLOR)
    {
        attrs.SetDefaultBackground(_DefaultBackground, _wFillAttribute);
    }
    return attrs;
}

bool Settings::IsTerminalScrolling() const noexcept
{
    return _TerminalScrolling;
}

void Settings::SetTerminalScrolling(const bool terminalScrollingEnabled) noexcept
{
    _TerminalScrolling = terminalScrollingEnabled;
}

bool Settings::GetUseDx() const noexcept
{
    return _fUseDx;
}
