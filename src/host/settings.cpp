/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "settings.hpp"

#include "cursor.h"
#include "misc.h"

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
    _nInputBufferSize(0), // TODO: reconcile
    // dwFontSize initialized below
    _uFontFamily(0),
    _uFontWeight(0),
    // FaceName initialized below
    _uCursorSize(CURSOR_SMALL_SIZE),
    _bFullScreen(false),
    _bQuickEdit(false),
    _bInsertMode(false),
    _bAutoPosition(true),
    _uHistoryBufferSize(DEFAULT_NUMBER_OF_COMMANDS),
    _uNumberOfHistoryBuffers(DEFAULT_NUMBER_OF_BUFFERS),
    _bHistoryNoDup(false),
    // ColorTable initialized below
    _uCodePage(g_uiOEMCP),
    _uScrollScale(1),
    _bLineSelection(false),
    _bWrapText(false),
    _fCtrlKeyShortcutsDisabled(false),
    _bWindowAlpha(BYTE_MAX), // 255 alpha = opaque. 0 = transparent.
    _fFilterOnPaste(false),
    _fTrimLeadingZeros(FALSE),
    _fEnableColorSelection(FALSE),
    _fExtendedEditKey(false),
    _fAllowAltF4Close(true),
    _dwVirtTermLevel(0),
    _fUseWindowSizePixels(false),
    _fAutoReturnOnNewline(true), // the historic Windows behavior defaults this to on.
    _fRenderGridWorldwide(false) // historically grid lines were only rendered in DBCS codepages, so this is false by default unless otherwise specified.
    // window size pixels initialized below
{
    _dwScreenBufferSize.X = 80;
    _dwScreenBufferSize.Y = 25;

    _dwWindowSize.X = _dwScreenBufferSize.X;
    _dwWindowSize.Y = _dwScreenBufferSize.Y;

    _dwWindowSizePixels = { 0 };

    _dwWindowOrigin.X = 0;
    _dwWindowOrigin.Y = 0;

    ZeroMemory((void*)&_dwFontSize, sizeof(_dwFontSize));
    ZeroMemory((void*)&_FaceName, sizeof(_FaceName));
    ZeroMemory((void*)&_LaunchFaceName, sizeof(_LaunchFaceName));

    _ColorTable[0] = RGB(0x0000, 0x0000, 0x0000);
    _ColorTable[1] = RGB(0x0000, 0x0000, 0x0080);
    _ColorTable[2] = RGB(0x0000, 0x0080, 0x0000);
    _ColorTable[3] = RGB(0x0000, 0x0080, 0x0080);
    _ColorTable[4] = RGB(0x0080, 0x0000, 0x0000);
    _ColorTable[5] = RGB(0x0080, 0x0000, 0x0080);
    _ColorTable[6] = RGB(0x0080, 0x0080, 0x0000);
    _ColorTable[7] = RGB(0x00C0, 0x00C0, 0x00C0);
    _ColorTable[8] = RGB(0x0080, 0x0080, 0x0080);
    _ColorTable[9] = RGB(0x0000, 0x0000, 0x00FF);
    _ColorTable[10] = RGB(0x0000, 0x00FF, 0x0000);
    _ColorTable[11] = RGB(0x0000, 0x00FF, 0x00FF);
    _ColorTable[12] = RGB(0x00FF, 0x0000, 0x0000);
    _ColorTable[13] = RGB(0x00FF, 0x0000, 0x00FF);
    _ColorTable[14] = RGB(0x00FF, 0x00FF, 0x0000);
    _ColorTable[15] = RGB(0x00FF, 0x00FF, 0x00FF);

    _InitXtermTableValue(0, 0x00, 0x00, 0x00);
    _InitXtermTableValue(1, 0x80, 0x00, 0x00);
    _InitXtermTableValue(2, 0x00, 0x80, 0x00);
    _InitXtermTableValue(3, 0x80, 0x80, 0x00);
    _InitXtermTableValue(4, 0x00, 0x00, 0x80);
    _InitXtermTableValue(5, 0x80, 0x00, 0x80);
    _InitXtermTableValue(6, 0x00, 0x80, 0x80);
    _InitXtermTableValue(7, 0xc0, 0xc0, 0xc0);
    _InitXtermTableValue(8, 0x80, 0x80, 0x80);
    _InitXtermTableValue(9, 0xff, 0x00, 0x00);
    _InitXtermTableValue(10, 0x00, 0xff, 0x00);
    _InitXtermTableValue(11, 0xff, 0xff, 0x00);
    _InitXtermTableValue(12, 0x00, 0x00, 0xff);
    _InitXtermTableValue(13, 0xff, 0x00, 0xff);
    _InitXtermTableValue(14, 0x00, 0xff, 0xff);
    _InitXtermTableValue(15, 0xff, 0xff, 0xff);
    _InitXtermTableValue(16, 0x00, 0x00, 0x00);
    _InitXtermTableValue(17, 0x00, 0x00, 0x5f);
    _InitXtermTableValue(18, 0x00, 0x00, 0x87);
    _InitXtermTableValue(19, 0x00, 0x00, 0xaf);
    _InitXtermTableValue(20, 0x00, 0x00, 0xd7);
    _InitXtermTableValue(21, 0x00, 0x00, 0xff);
    _InitXtermTableValue(22, 0x00, 0x5f, 0x00);
    _InitXtermTableValue(23, 0x00, 0x5f, 0x5f);
    _InitXtermTableValue(24, 0x00, 0x5f, 0x87);
    _InitXtermTableValue(25, 0x00, 0x5f, 0xaf);
    _InitXtermTableValue(26, 0x00, 0x5f, 0xd7);
    _InitXtermTableValue(27, 0x00, 0x5f, 0xff);
    _InitXtermTableValue(28, 0x00, 0x87, 0x00);
    _InitXtermTableValue(29, 0x00, 0x87, 0x5f);
    _InitXtermTableValue(30, 0x00, 0x87, 0x87);
    _InitXtermTableValue(31, 0x00, 0x87, 0xaf);
    _InitXtermTableValue(32, 0x00, 0x87, 0xd7);
    _InitXtermTableValue(33, 0x00, 0x87, 0xff);
    _InitXtermTableValue(34, 0x00, 0xaf, 0x00);
    _InitXtermTableValue(35, 0x00, 0xaf, 0x5f);
    _InitXtermTableValue(36, 0x00, 0xaf, 0x87);
    _InitXtermTableValue(37, 0x00, 0xaf, 0xaf);
    _InitXtermTableValue(38, 0x00, 0xaf, 0xd7);
    _InitXtermTableValue(39, 0x00, 0xaf, 0xff);
    _InitXtermTableValue(40, 0x00, 0xd7, 0x00);
    _InitXtermTableValue(41, 0x00, 0xd7, 0x5f);
    _InitXtermTableValue(42, 0x00, 0xd7, 0x87);
    _InitXtermTableValue(43, 0x00, 0xd7, 0xaf);
    _InitXtermTableValue(44, 0x00, 0xd7, 0xd7);
    _InitXtermTableValue(45, 0x00, 0xd7, 0xff);
    _InitXtermTableValue(46, 0x00, 0xff, 0x00);
    _InitXtermTableValue(47, 0x00, 0xff, 0x5f);
    _InitXtermTableValue(48, 0x00, 0xff, 0x87);
    _InitXtermTableValue(49, 0x00, 0xff, 0xaf);
    _InitXtermTableValue(50, 0x00, 0xff, 0xd7);
    _InitXtermTableValue(51, 0x00, 0xff, 0xff);
    _InitXtermTableValue(52, 0x5f, 0x00, 0x00);
    _InitXtermTableValue(53, 0x5f, 0x00, 0x5f);
    _InitXtermTableValue(54, 0x5f, 0x00, 0x87);
    _InitXtermTableValue(55, 0x5f, 0x00, 0xaf);
    _InitXtermTableValue(56, 0x5f, 0x00, 0xd7);
    _InitXtermTableValue(57, 0x5f, 0x00, 0xff);
    _InitXtermTableValue(58, 0x5f, 0x5f, 0x00);
    _InitXtermTableValue(59, 0x5f, 0x5f, 0x5f);
    _InitXtermTableValue(60, 0x5f, 0x5f, 0x87);
    _InitXtermTableValue(61, 0x5f, 0x5f, 0xaf);
    _InitXtermTableValue(62, 0x5f, 0x5f, 0xd7);
    _InitXtermTableValue(63, 0x5f, 0x5f, 0xff);
    _InitXtermTableValue(64, 0x5f, 0x87, 0x00);
    _InitXtermTableValue(65, 0x5f, 0x87, 0x5f);
    _InitXtermTableValue(66, 0x5f, 0x87, 0x87);
    _InitXtermTableValue(67, 0x5f, 0x87, 0xaf);
    _InitXtermTableValue(68, 0x5f, 0x87, 0xd7);
    _InitXtermTableValue(69, 0x5f, 0x87, 0xff);
    _InitXtermTableValue(70, 0x5f, 0xaf, 0x00);
    _InitXtermTableValue(71, 0x5f, 0xaf, 0x5f);
    _InitXtermTableValue(72, 0x5f, 0xaf, 0x87);
    _InitXtermTableValue(73, 0x5f, 0xaf, 0xaf);
    _InitXtermTableValue(74, 0x5f, 0xaf, 0xd7);
    _InitXtermTableValue(75, 0x5f, 0xaf, 0xff);
    _InitXtermTableValue(76, 0x5f, 0xd7, 0x00);
    _InitXtermTableValue(77, 0x5f, 0xd7, 0x5f);
    _InitXtermTableValue(78, 0x5f, 0xd7, 0x87);
    _InitXtermTableValue(79, 0x5f, 0xd7, 0xaf);
    _InitXtermTableValue(80, 0x5f, 0xd7, 0xd7);
    _InitXtermTableValue(81, 0x5f, 0xd7, 0xff);
    _InitXtermTableValue(82, 0x5f, 0xff, 0x00);
    _InitXtermTableValue(83, 0x5f, 0xff, 0x5f);
    _InitXtermTableValue(84, 0x5f, 0xff, 0x87);
    _InitXtermTableValue(85, 0x5f, 0xff, 0xaf);
    _InitXtermTableValue(86, 0x5f, 0xff, 0xd7);
    _InitXtermTableValue(87, 0x5f, 0xff, 0xff);
    _InitXtermTableValue(88, 0x87, 0x00, 0x00);
    _InitXtermTableValue(89, 0x87, 0x00, 0x5f);
    _InitXtermTableValue(90, 0x87, 0x00, 0x87);
    _InitXtermTableValue(91, 0x87, 0x00, 0xaf);
    _InitXtermTableValue(92, 0x87, 0x00, 0xd7);
    _InitXtermTableValue(93, 0x87, 0x00, 0xff);
    _InitXtermTableValue(94, 0x87, 0x5f, 0x00);
    _InitXtermTableValue(95, 0x87, 0x5f, 0x5f);
    _InitXtermTableValue(96, 0x87, 0x5f, 0x87);
    _InitXtermTableValue(97, 0x87, 0x5f, 0xaf);
    _InitXtermTableValue(98, 0x87, 0x5f, 0xd7);
    _InitXtermTableValue(99, 0x87, 0x5f, 0xff);
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



void Settings::_InitXtermTableValue(_In_ const size_t iIndex, _In_ const byte bRed, _In_ const byte bGreen, _In_ const byte bBlue)
{
    _XtermColorTable[iIndex] = RGB(bRed, bGreen, bBlue);
}

void Settings::ApplyStartupInfo(_In_ const Settings* const pStartupSettings)
{
    const DWORD dwFlags = pStartupSettings->_dwStartupFlags;

    // See: http://msdn.microsoft.com/en-us/library/windows/desktop/ms686331(v=vs.85).aspx

    if (IsFlagSet(dwFlags, STARTF_USECOUNTCHARS))
    {
        _dwScreenBufferSize = pStartupSettings->_dwScreenBufferSize;
    }

    if (IsFlagSet(dwFlags, STARTF_USESIZE))
    {
        // WARNING: This size is in pixels when passed in the create process call.
        // It will need to be divided by the font size before use.
        // All other Window Size values (from registry/shortcuts) are stored in characters.
        _dwWindowSizePixels = pStartupSettings->_dwWindowSize;
        _fUseWindowSizePixels = true;
    }

    if (IsFlagSet(dwFlags, STARTF_USEPOSITION))
    {
        _dwWindowOrigin = pStartupSettings->_dwWindowOrigin;
        _bAutoPosition = FALSE;
    }

    if (IsFlagSet(dwFlags, STARTF_USEFILLATTRIBUTE))
    {
        _wFillAttribute = pStartupSettings->_wFillAttribute;
    }

    if (IsFlagSet(dwFlags, STARTF_USESHOWWINDOW))
    {
        _wShowWindow = pStartupSettings->_wShowWindow;
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
    _bWindowAlpha = pStateInfo->bWindowTransparency;
}

void Settings::Validate()
{
    // If we were explicitly given a size in pixels from the startup info, divide by the font to turn it into characters.
    // See: https://msdn.microsoft.com/en-us/library/windows/desktop/ms686331%28v=vs.85%29.aspx
    if (IsFlagSet(_dwStartupFlags, STARTF_USESIZE))
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
    _dwScreenBufferSize.X = max(_dwScreenBufferSize.X, 1);
    _dwScreenBufferSize.Y = max(_dwScreenBufferSize.Y, 1);

    // minimum window size size 1x1
    _dwWindowSize.X = max(_dwWindowSize.X, 1);
    _dwWindowSize.Y = max(_dwWindowSize.Y, 1);

    // if buffer size is less than window size, increase buffer size to meet window size
    _dwScreenBufferSize.X = max(_dwWindowSize.X, _dwScreenBufferSize.X);
    _dwScreenBufferSize.Y = max(_dwWindowSize.Y, _dwScreenBufferSize.Y);

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

    ASSERT(_dwWindowSize.X > 0);
    ASSERT(_dwWindowSize.Y > 0);
    ASSERT(_dwScreenBufferSize.X > 0);
    ASSERT(_dwScreenBufferSize.Y > 0);
}

DWORD Settings::GetVirtTermLevel() const
{
    return this->_dwVirtTermLevel;
}
void Settings::SetVirtTermLevel(_In_ DWORD const dwVirtTermLevel)
{
    this->_dwVirtTermLevel = dwVirtTermLevel;
}

bool Settings::IsAltF4CloseAllowed() const
{
    return this->_fAllowAltF4Close;
}
void Settings::SetAltF4CloseAllowed(_In_ bool const fAllowAltF4Close)
{
    this->_fAllowAltF4Close = fAllowAltF4Close;
}

bool Settings::IsReturnOnNewlineAutomatic() const
{
    return this->_fAutoReturnOnNewline;
}
void Settings::SetAutomaticReturnOnNewline(_In_ bool const fAutoReturnOnNewline)
{
    this->_fAutoReturnOnNewline = fAutoReturnOnNewline;
}

bool Settings::IsGridRenderingAllowedWorldwide() const
{
    return this->_fRenderGridWorldwide;
}
void Settings::SetGridRenderingAllowedWorldwide(_In_ bool const fGridRenderingAllowed)
{
    // Only trigger a notification and update the status if something has changed.
    if (this->_fRenderGridWorldwide != fGridRenderingAllowed)
    {
        this->_fRenderGridWorldwide = fGridRenderingAllowed;

        if (g_pRender != nullptr)
        {
            g_pRender->TriggerRedrawAll();
        }
    }
}

bool Settings::GetExtendedEditKey() const
{
    return this->_fExtendedEditKey;
}
void Settings::SetExtendedEditKey(_In_ bool const fExtendedEditKey)
{
    this->_fExtendedEditKey = fExtendedEditKey;
}

BOOL Settings::GetFilterOnPaste() const
{
    return this->_fFilterOnPaste;
}
void Settings::SetFilterOnPaste(_In_ BOOL const fFilterOnPaste)
{
    this->_fFilterOnPaste = fFilterOnPaste;
}

const WCHAR* const Settings::GetLaunchFaceName() const
{
    return this->_LaunchFaceName;
}
void Settings::SetLaunchFaceName(_In_ PCWSTR const LaunchFaceName, _In_ size_t const cchLength)
{
    StringCchCopyW(this->_LaunchFaceName, cchLength, LaunchFaceName);
}

UINT Settings::GetCodePage() const
{
    return this->_uCodePage;
}
void Settings::SetCodePage(_In_ const UINT uCodePage)
{
    this->_uCodePage = uCodePage;
}

UINT Settings::GetScrollScale() const
{
    return this->_uScrollScale;
}
void Settings::SetScrollScale(_In_ const UINT uScrollScale)
{
    this->_uScrollScale = uScrollScale;
}

BOOL Settings::GetTrimLeadingZeros() const
{
    return this->_fTrimLeadingZeros;
}
void Settings::SetTrimLeadingZeros(_In_ const BOOL fTrimLeadingZeros)
{
    this->_fTrimLeadingZeros = fTrimLeadingZeros;
}

BOOL Settings::GetEnableColorSelection() const
{
    return this->_fEnableColorSelection;
}
void Settings::SetEnableColorSelection(_In_ const BOOL fEnableColorSelection)
{
    this->_fEnableColorSelection = fEnableColorSelection;
}

BOOL Settings::GetLineSelection() const
{
    return this->_bLineSelection;
}
void Settings::SetLineSelection(_In_ const BOOL bLineSelection)
{
    this->_bLineSelection = bLineSelection;
}

bool Settings::GetWrapText () const
{
    return this->_bWrapText;
}
void Settings::SetWrapText (_In_ const bool bWrapText )
{
    this->_bWrapText = bWrapText;
}

BOOL Settings::GetCtrlKeyShortcutsDisabled () const
{
    return this->_fCtrlKeyShortcutsDisabled;
}
void Settings::SetCtrlKeyShortcutsDisabled (_In_ const BOOL fCtrlKeyShortcutsDisabled )
{
    this->_fCtrlKeyShortcutsDisabled = fCtrlKeyShortcutsDisabled;
}

BYTE Settings::GetWindowAlpha() const
{
    return this->_bWindowAlpha;
}
void Settings::SetWindowAlpha(_In_ const BYTE bWindowAlpha)
{
    // if we're out of bounds, set it to 100% opacity so it appears as if nothing happened.
    this->_bWindowAlpha = (bWindowAlpha < MIN_WINDOW_OPACITY)? BYTE_MAX : bWindowAlpha;
}

DWORD Settings::GetHotKey() const
{
    return this->_dwHotKey;
}
void Settings::SetHotKey(_In_ const DWORD dwHotKey)
{
    this->_dwHotKey = dwHotKey;
}

DWORD Settings::GetStartupFlags() const
{
    return this->_dwStartupFlags;
}
void Settings::SetStartupFlags(_In_ const DWORD dwStartupFlags)
{
    this->_dwStartupFlags = dwStartupFlags;
}

WORD Settings::GetFillAttribute() const
{
    return this->_wFillAttribute;
}
void Settings::SetFillAttribute(_In_ const WORD wFillAttribute)
{
    this->_wFillAttribute = wFillAttribute;
}

WORD Settings::GetPopupFillAttribute() const
{
    return this->_wPopupFillAttribute;
}
void Settings::SetPopupFillAttribute(_In_ const WORD wPopupFillAttribute)
{
    this->_wPopupFillAttribute = wPopupFillAttribute;
}

WORD Settings::GetShowWindow() const
{
    return this->_wShowWindow;
}
void Settings::SetShowWindow(_In_ const WORD wShowWindow)
{
    this->_wShowWindow = wShowWindow;
}

WORD Settings::GetReserved() const
{
    return this->_wReserved;
}
void Settings::SetReserved(_In_ const WORD wReserved)
{
    this->_wReserved = wReserved;
}

COORD Settings::GetScreenBufferSize() const
{
    return this->_dwScreenBufferSize;
}
void Settings::SetScreenBufferSize(_In_ const COORD dwScreenBufferSize)
{
    this->_dwScreenBufferSize = dwScreenBufferSize;
}

COORD Settings::GetWindowSize() const
{
    return this->_dwWindowSize;
}
void Settings::SetWindowSize(_In_ const COORD dwWindowSize)
{
    this->_dwWindowSize = dwWindowSize;
}

bool Settings::IsWindowSizePixelsValid() const
{
    return _fUseWindowSizePixels;
}
COORD Settings::GetWindowSizePixels() const
{
    return _dwWindowSizePixels;
}
void Settings::SetWindowSizePixels(_In_ const COORD dwWindowSizePixels)
{
    _dwWindowSizePixels = dwWindowSizePixels;
}

COORD Settings::GetWindowOrigin() const
{
    return this->_dwWindowOrigin;
}
void Settings::SetWindowOrigin(_In_ const COORD dwWindowOrigin)
{
    this->_dwWindowOrigin = dwWindowOrigin;
}

DWORD Settings::GetFont() const
{
    return this->_nFont;
}
void Settings::SetFont(_In_ const DWORD nFont)
{
    this->_nFont = nFont;
}

DWORD Settings::GetInputBufferSize() const
{
    return this->_nInputBufferSize;
}
void Settings::SetInputBufferSize(_In_ const DWORD nInputBufferSize)
{
    this->_nInputBufferSize = nInputBufferSize;
}

COORD Settings::GetFontSize() const
{
    return this->_dwFontSize;
}
void Settings::SetFontSize(_In_ const COORD dwFontSize)
{
    this->_dwFontSize = dwFontSize;
}

UINT Settings::GetFontFamily() const
{
    return this->_uFontFamily;
}
void Settings::SetFontFamily(_In_ const UINT uFontFamily)
{
    this->_uFontFamily = uFontFamily;
}

UINT Settings::GetFontWeight() const
{
    return this->_uFontWeight;
}
void Settings::SetFontWeight(_In_ const UINT uFontWeight)
{
    this->_uFontWeight = uFontWeight;
}

const WCHAR* const Settings::GetFaceName() const
{
    return this->_FaceName;
}
void Settings::SetFaceName(_In_ PCWSTR const pcszFaceName, _In_ size_t const cchLength)
{
    StringCchCopyW(this->_FaceName, cchLength, pcszFaceName);
}

UINT Settings::GetCursorSize() const
{
    return this->_uCursorSize;
}
void Settings::SetCursorSize(_In_ const UINT uCursorSize)
{
    this->_uCursorSize = uCursorSize;
}

BOOL Settings::GetFullScreen() const
{
    return this->_bFullScreen;
}
void Settings::SetFullScreen(_In_ const BOOL bFullScreen)
{
    this->_bFullScreen = bFullScreen;
}

BOOL Settings::GetQuickEdit() const
{
    return this->_bQuickEdit;
}
void Settings::SetQuickEdit(_In_ const BOOL bQuickEdit)
{
    this->_bQuickEdit = bQuickEdit;
}

BOOL Settings::GetInsertMode() const
{
    return this->_bInsertMode;
}
void Settings::SetInsertMode(_In_ const BOOL bInsertMode)
{
    this->_bInsertMode = bInsertMode;
}

BOOL Settings::GetAutoPosition() const
{
    return this->_bAutoPosition;
}
void Settings::SetAutoPosition(_In_ const BOOL bAutoPosition)
{
    this->_bAutoPosition = bAutoPosition;
}

UINT Settings::GetHistoryBufferSize() const
{
    return this->_uHistoryBufferSize;
}
void Settings::SetHistoryBufferSize(_In_ const UINT uHistoryBufferSize)
{
    this->_uHistoryBufferSize = uHistoryBufferSize;
}

UINT Settings::GetNumberOfHistoryBuffers() const
{
    return this->_uNumberOfHistoryBuffers;
}
void Settings::SetNumberOfHistoryBuffers(_In_ const UINT uNumberOfHistoryBuffers)
{
    this->_uNumberOfHistoryBuffers = uNumberOfHistoryBuffers;
}

BOOL Settings::GetHistoryNoDup() const
{
    return this->_bHistoryNoDup;
}
void Settings::SetHistoryNoDup(_In_ const BOOL bHistoryNoDup)
{
    this->_bHistoryNoDup = bHistoryNoDup;
}

const COLORREF* const Settings::GetColorTable() const
{
    return this->_ColorTable;
}
void Settings::SetColorTable(_In_reads_(cSize) const COLORREF* const pColorTable, _In_ size_t const cSize)
{
    size_t cSizeWritten = min(cSize, COLOR_TABLE_SIZE);

    memmove(this->_ColorTable, pColorTable, cSizeWritten * sizeof(COLORREF));
}
void Settings::SetColorTableEntry(_In_ size_t const index, _In_ COLORREF const ColorValue)
{
    if (index < ARRAYSIZE(_ColorTable))
    {
        this->_ColorTable[index] = ColorValue;
    }
    else
    {
        this->_XtermColorTable[index] = ColorValue;
    }
}

bool Settings::IsStartupTitleIsLinkNameSet() const
{
    return IsFlagSet(_dwStartupFlags, STARTF_TITLEISLINKNAME);
}

BOOL Settings::IsFaceNameSet() const
{
    return this->GetFaceName()[0] != '\0';
}

void Settings::UnsetStartupFlag(_In_ DWORD const dwFlagToUnset)
{
    this->_dwStartupFlags &= ~dwFlagToUnset;
}

const size_t Settings::GetColorTableSize() const
{
    return ARRAYSIZE(this->_ColorTable);
}

COLORREF Settings::GetColorTableEntry(_In_ size_t const index) const
{
    if (index < ARRAYSIZE(_ColorTable))
    {
        return this->_ColorTable[index];
    }
    else
    {
        return this->_XtermColorTable[index];
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
WORD Settings::GenerateLegacyAttributes(_In_ const TextAttribute attributes) const
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
// Finds the "distance" between a given HSL color and an RGB color, using the HSL color space.
//   This function is designed such that the caller would convert one RGB color to HSL ahead of time,
//   then compare many RGB colors to that first color.
//Arguments:
// - phslColorA - a pointer to the first color, as a HSL color.
// - rgbColorB - The second color to compare, in RGB color.
// Return value:
// The "distance" between the two.
double Settings::s_FindDifference(_In_ const _HSL* const phslColorA, _In_ const COLORREF rgbColorB)
{
    const _HSL hslColorB = _HSL(rgbColorB);
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
WORD Settings::FindNearestTableIndex(_In_ COLORREF const Color) const
{
    const _HSL hslColor = _HSL(Color);
    WORD closest = 0;
    double minDiff = s_FindDifference(&hslColor, _ColorTable[0]);
    for (WORD i = 1; i < ARRAYSIZE(_ColorTable); i++)
    {
        double diff = s_FindDifference(&hslColor, _ColorTable[i]);
        if (diff < minDiff)
        {
            minDiff = diff;
            closest = i;
        }
    }
    return closest;
}
