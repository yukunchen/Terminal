#pragma once

// This used to be in find.h.
#define SEARCH_STRING_LENGTH    (80)

USHORT SearchForString(_In_ const SCREEN_INFORMATION * const pScreenInfo,
	                   _In_reads_(cchSearch) PCWSTR pwszSearch,
	                   _In_range_(1, SEARCH_STRING_LENGTH) USHORT cchSearch,
	                   _In_ const bool IgnoreCase,
	                   _In_ const bool Reverse,
	                   _In_ const bool SearchAndSetAttr,
	                   _In_ const ULONG ulAttr,
	                   _Out_opt_ PCOORD coordStringPosition);  // not touched for SearchAndSetAttr case.
