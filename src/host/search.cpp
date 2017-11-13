#include "precomp.h"

#include "search.h"

#include "dbcs.h"

USHORT SearchForString(_In_ const SCREEN_INFORMATION * const pScreenInfo,
	                   _In_reads_(cchSearch) PCWSTR pwszSearch,
	                   _In_range_(1, SEARCH_STRING_LENGTH) USHORT cchSearch,
	                   _In_ const BOOLEAN IgnoreCase,
	                   _In_ const BOOLEAN Reverse,
	                   _In_ const BOOLEAN SearchAndSetAttr,
	                   _In_ const ULONG ulAttr,
	                   _Out_opt_ PCOORD coordStringPosition)  // not touched for SearchAndSetAttr case.
{
	if (coordStringPosition != nullptr)
	{
		coordStringPosition->X = 0;
		coordStringPosition->Y = 0;
	}

	COORD MaxPosition = pScreenInfo->GetScreenBufferSize();
	MaxPosition.X -= cchSearch;
	MaxPosition.Y -= 1;

	Selection* const pSelection = &Selection::Instance();

	// calculate starting position
	COORD Position;
	if (pSelection->IsInSelectingState())
	{
		COORD coordSelAnchor;
		pSelection->GetSelectionAnchor(&coordSelAnchor);

		Position.X = min(coordSelAnchor.X, MaxPosition.X);
		Position.Y = coordSelAnchor.Y;
	}
	else if (Reverse)
	{
		Position.X = 0;
		Position.Y = 0;
	}
	else
	{
		Position.X = MaxPosition.X;
		Position.Y = MaxPosition.Y;
	}

	// Prepare search string.
	WCHAR SearchString2[SEARCH_STRING_LENGTH * 2 + 1];  // search string buffer
	UINT const cchSearchString2 = ARRAYSIZE(SearchString2);
	ASSERT(cchSearch == wcslen(pwszSearch) && cchSearch < cchSearchString2);

	PWSTR pStr = SearchString2;

	// boundary to prevent loop overflow. -1 to leave space for the null terminator at the end of the string.
	const PCWCHAR pwchSearchString2End = SearchString2 + cchSearchString2 - 1;

	while (*pwszSearch && pStr < pwchSearchString2End)
	{
		*pStr++ = *pwszSearch;
#if defined(CON_TB_MARK)
		// On the screen, one East Asian "FullWidth" character occupies two columns (double width),
		// so we have to share two screen buffer elements for one DBCS character.
		// For example, if the screen shows "AB[DBC]CD", the screen buffer will be,
		//   [L'A'] [L'B'] [DBC(Unicode)] [CON_TB_MARK] [L'C'] [L'D']
		//   (DBC:: Double Byte Character)
		// CON_TB_MARK is used to indicate that the column is the trainling byte.
		//
		// Before comparing the string with the screen buffer, we need to modify the search
		// string to match the format of the screen buffer.
		// If we find a FullWidth character in the search string, put CON_TB_MARK
		// right after it so that we're able to use NLS functions.
#else
		// If KAttribute is used, the above example will look like:
		// CharRow.Chars: [L'A'] [L'B'] [DBC(Unicode)] [DBC(Unicode)] [L'C'] [L'D']
		// CharRow.KAttrs:    0      0   LEADING_BYTE  TRAILING_BYTE       0      0
		//
		// We do no fixup if SearchAndSetAttr was specified.  In this case the search buffer has
		// come straight out of the console buffer,  so is already in the required format.
#endif
		if (!SearchAndSetAttr
			&& IsCharFullWidth(*pwszSearch)
			&& pStr < pwchSearchString2End)
		{
#if defined(CON_TB_MARK)
			*pStr++ = CON_TB_MARK;
#else
			*pStr++ = *pwszSearch;
#endif
		}
		++pwszSearch;
	}

	*pStr = L'\0';

	USHORT const ColumnWidth = (USHORT)(pStr - SearchString2);
	pwszSearch = SearchString2;

	// search for the string
	BOOL RecomputeRow = TRUE;
	COORD const EndPosition = Position;
	SHORT RowIndex = 0;
    try
    {
        do
        {
    #if !defined(CON_TB_MARK)
    #if DBG
            int nLoop = 0;
    #endif
        recalc:
    #endif
            if (Reverse)
            {
                if (--Position.X < 0)
                {
                    Position.X = MaxPosition.X;
                    if (--Position.Y < 0)
                    {
                        Position.Y = MaxPosition.Y;
                    }
                    RecomputeRow = TRUE;
                }
            }
            else
            {
                if (++Position.X > MaxPosition.X)
                {
                    Position.X = 0;
                    if (++Position.Y > MaxPosition.Y)
                    {
                        Position.Y = 0;
                    }
                    RecomputeRow = TRUE;
                }
            }

            const ROW* pRow = &pScreenInfo->TextInfo->GetRowAtIndex(RowIndex);
            if (RecomputeRow)
            {
                RowIndex = (pScreenInfo->TextInfo->GetFirstRowIndex() + Position.Y) % pScreenInfo->GetScreenBufferSize().Y;
                pRow = &pScreenInfo->TextInfo->GetRowAtIndex(RowIndex);
                RecomputeRow = FALSE;
            }

    #if !defined(CON_TB_MARK)
            ASSERT(nLoop++ < 2);
            if (pRow->CharRow.KAttrs && pRow->IsTrailingByteAtColumn(Position.X))
            {
                goto recalc;
            }
    #endif
            if (IgnoreCase ?
                0 == _wcsnicmp(pwszSearch, &pRow->CharRow.Chars[Position.X], cchSearch) :
                0 == wcsncmp(pwszSearch, &pRow->CharRow.Chars[Position.X], cchSearch))
            {
                //  If this operation was a normal user find, then return now. Otherwise set the attributes of this match,
                //  and continue searching the whole buffer.
                if (!SearchAndSetAttr)
                {
                    *coordStringPosition = Position;
                    return ColumnWidth;
                }
                else
                {
                    SMALL_RECT Rect;
                    Rect.Top = Rect.Bottom = Position.Y;
                    Rect.Left = Position.X;
                    Rect.Right = Rect.Left + ColumnWidth - 1;

                    pSelection->ColorSelection(&Rect, ulAttr);
                }
            }
        } while (!((Position.X == EndPosition.X) && (Position.Y == EndPosition.Y)));
    }
    catch (...)
    {
        return 0;
    }

	return 0;   // the string was not found
}
