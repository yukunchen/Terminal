
/*++
Copyright (c) Microsoft Corporation

Module Name:
- unicode.hpp

Abstract:
- This file contains macro definitions for some common wchar values.
- taken from input.h

Author(s):
- Austin Diviness (AustDi) Oct 2017
--*/

#define UNICODE_BACKSPACE ((WCHAR)0x08)
// NOTE: This isn't actually a backspace. It's a graphical block. But
// I believe it's emitted by one of our ANSI/OEM --> Unicode conversions.
// We should dig further into this in the future.
#define UNICODE_BACKSPACE2 ((WCHAR)0x25d8)
#define UNICODE_CARRIAGERETURN ((WCHAR)0x0d)
#define UNICODE_LINEFEED ((WCHAR)0x0a)
#define UNICODE_BELL ((WCHAR)0x07)
#define UNICODE_TAB ((WCHAR)0x09)
#define UNICODE_SPACE ((WCHAR)0x20)
#define UNICODE_LEFT_SMARTQUOTE ((WCHAR)0x201c)
#define UNICODE_RIGHT_SMARTQUOTE ((WCHAR)0x201d)
#define UNICODE_EM_DASH ((WCHAR)0x2014)
#define UNICODE_EN_DASH ((WCHAR)0x2013)
#define UNICODE_NBSP ((WCHAR)0xa0)
#define UNICODE_NARROW_NBSP ((WCHAR)0x202f)
#define UNICODE_QUOTE L'\"'
#define UNICODE_HYPHEN L'-'
