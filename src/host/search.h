#pragma once

// This used to be in find.h.
#define SEARCH_STRING_LENGTH    (80)

USHORT SearchForString(const SCREEN_INFORMATION * const pScreenInfo,
	                   _In_reads_(cchSearch) PCWSTR pwszSearch,
	                   _In_range_(1, SEARCH_STRING_LENGTH) USHORT cchSearch,
	                   const BOOLEAN IgnoreCase,
	                   const BOOLEAN Reverse,
	                   const BOOLEAN SearchAndSetAttr,
	                   const ULONG ulAttr,
	                   _Out_opt_ PCOORD coordStringPosition);  // not touched for SearchAndSetAttr case.
