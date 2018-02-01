/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#include "precomp.h"

#include "alias.h"

#include "_output.h"
#include "output.h"
#include "stream.h"
#include "_stream.h"
#include "dbcs.h"
#include "handle.h"
#include "misc.h"
#include "../types/inc/convert.hpp"
#include "srvinit.h"
#include "resource.h"

#include "ApiRoutines.h"
#include "KeyEventHelpers.hpp"

#include "..\interactivity\inc\ServiceLocator.hpp"

#pragma hdrstop

PEXE_ALIAS_LIST AddExeAliasList(_In_ LPVOID ExeName,
                                _In_ USHORT ExeLength, // in bytes
                                _In_ BOOLEAN UnicodeExe)
{
    CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    PEXE_ALIAS_LIST AliasList = new EXE_ALIAS_LIST();
    if (AliasList == nullptr)
    {
        return nullptr;
    }

    if (UnicodeExe)
    {
        // Length is in bytes. Add 1 so dividing by WCHAR (2) is always rounding up.
        AliasList->ExeName = new WCHAR[(ExeLength + 1) / sizeof(WCHAR)];
        if (AliasList->ExeName == nullptr)
        {
            delete AliasList;
            return nullptr;
        }
        memmove(AliasList->ExeName, ExeName, ExeLength);
        AliasList->ExeLength = ExeLength;
    }
    else
    {
        AliasList->ExeName = new WCHAR[ExeLength];
        if (AliasList->ExeName == nullptr)
        {
            delete AliasList;
            return nullptr;
        }
        AliasList->ExeLength = (USHORT)ConvertInputToUnicode(gci->CP, (LPSTR)ExeName, ExeLength, AliasList->ExeName, ExeLength);
        AliasList->ExeLength *= 2;
    }
    InitializeListHead(&AliasList->AliasList);
    InsertHeadList(&gci->ExeAliasList, &AliasList->ListLink);
    return AliasList;
}

// Routine Description:
// - This routine searches for the specified exe alias list.  It returns a pointer to the exe list if found, nullptr if not found.
PEXE_ALIAS_LIST FindExe(_In_ LPVOID ExeName,
                        _In_ USHORT ExeLength, // in bytes
                        _In_ BOOLEAN UnicodeExe)
{
    CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    LPWSTR UnicodeExeName;
    if (UnicodeExe)
    {
        UnicodeExeName = (PWSTR)ExeName;
    }
    else
    {
        UnicodeExeName = new WCHAR[ExeLength];
        if (UnicodeExeName == nullptr)
            return nullptr;
        ExeLength = (USHORT)ConvertInputToUnicode(gci->CP, (LPSTR)ExeName, ExeLength, UnicodeExeName, ExeLength);
        ExeLength *= 2;
    }
    PLIST_ENTRY const ListHead = &gci->ExeAliasList;
    PLIST_ENTRY ListNext = ListHead->Flink;
    while (ListNext != ListHead)
    {
        PEXE_ALIAS_LIST const AliasList = CONTAINING_RECORD(ListNext, EXE_ALIAS_LIST, ListLink);
        // Length is in bytes. Add 1 so dividing by WCHAR (2) is always rounding up.
        if (AliasList->ExeLength == ExeLength && !_wcsnicmp(AliasList->ExeName, UnicodeExeName, (ExeLength + 1) / sizeof(WCHAR)))
        {
            if (!UnicodeExe)
            {
                delete[] UnicodeExeName;
            }
            return AliasList;
        }
        ListNext = ListNext->Flink;
    }
    if (!UnicodeExe)
    {
        delete[] UnicodeExeName;
    }
    return nullptr;
}

// Routine Description:
// - This routine searches for the specified alias.  If it finds one,
// - it moves it to the head of the list and returns a pointer to the
// - alias. Otherwise it returns nullptr.
PALIAS FindAlias(_In_ PEXE_ALIAS_LIST AliasList, _In_reads_bytes_(AliasLength) const WCHAR *AliasName, _In_ USHORT AliasLength)
{
    PLIST_ENTRY const ListHead = &AliasList->AliasList;
    PLIST_ENTRY ListNext = ListHead->Flink;
    while (ListNext != ListHead)
    {
        PALIAS const Alias = CONTAINING_RECORD(ListNext, ALIAS, ListLink);
        // Length is in bytes. Add 1 so dividing by WCHAR (2) is always rounding up.
        if (Alias->SourceLength == AliasLength && !_wcsnicmp(Alias->Source, AliasName, (AliasLength + 1) / sizeof(WCHAR)))
        {
            if (ListNext != ListHead->Flink)
            {
                RemoveEntryList(ListNext);
                InsertHeadList(ListHead, ListNext);
            }
            return Alias;
        }
        ListNext = ListNext->Flink;
    }

    return nullptr;
}

// Routine Description:
// - This routine creates an alias and inserts it into the exe alias list.
NTSTATUS AddAlias(_In_ PEXE_ALIAS_LIST ExeAliasList,
                  _In_reads_bytes_(SourceLength) const WCHAR *Source,
                  _In_ USHORT SourceLength,
                  _In_reads_bytes_(TargetLength) const WCHAR *Target,
                  _In_ USHORT TargetLength)
{
    PALIAS const Alias = new ALIAS();
    if (Alias == nullptr)
    {
        return STATUS_NO_MEMORY;
    }

    // Length is in bytes. Add 1 so dividing by WCHAR (2) is always rounding up.
    Alias->Source = new WCHAR[(SourceLength + 1) / sizeof(WCHAR)];
    if (Alias->Source == nullptr)
    {
        delete Alias;
        return STATUS_NO_MEMORY;
    }

    // Length is in bytes. Add 1 so dividing by WCHAR (2) is always rounding up.
    Alias->Target = new WCHAR[(TargetLength + 1) / sizeof(WCHAR)];
    if (Alias->Target == nullptr)
    {
        delete[] Alias->Source;
        delete Alias;
        return STATUS_NO_MEMORY;
    }

    Alias->SourceLength = SourceLength;
    Alias->TargetLength = TargetLength;
    memmove(Alias->Source, Source, SourceLength);
    memmove(Alias->Target, Target, TargetLength);
    InsertHeadList(&ExeAliasList->AliasList, &Alias->ListLink);
    return STATUS_SUCCESS;
}

// Routine Description:
// - This routine replaces an existing target with a new target.
NTSTATUS ReplaceAlias(_In_ PALIAS Alias, _In_reads_bytes_(TargetLength) const WCHAR *Target, _In_ USHORT TargetLength)
{
    // Length is in bytes. Add 1 so dividing by WCHAR (2) is always rounding up.
    WCHAR* const NewTarget = new WCHAR[(TargetLength + 1) / sizeof(WCHAR)];
    if (NewTarget == nullptr)
    {
        return STATUS_NO_MEMORY;
    }

    delete[] Alias->Target;
    Alias->Target = NewTarget;
    Alias->TargetLength = TargetLength;
    memmove(Alias->Target, Target, TargetLength);

    return STATUS_SUCCESS;
}

// Routine Description:
// - This routine removes an alias.
NTSTATUS RemoveAlias(_In_ PALIAS Alias)
{
    RemoveEntryList(&Alias->ListLink);
    delete[] Alias->Source;
    delete[] Alias->Target;
    delete Alias;
    return STATUS_SUCCESS;
}

void FreeAliasList(_In_ PEXE_ALIAS_LIST ExeAliasList)
{
    PLIST_ENTRY const ListHead = &ExeAliasList->AliasList;
    PLIST_ENTRY ListNext = ListHead->Flink;
    while (ListNext != ListHead)
    {
        PALIAS const Alias = CONTAINING_RECORD(ListNext, ALIAS, ListLink);
        ListNext = ListNext->Flink;
        RemoveAlias(Alias);
    }
    RemoveEntryList(&ExeAliasList->ListLink);
    delete[] ExeAliasList->ExeName;
    delete ExeAliasList;
}

void FreeAliasBuffers()
{
    CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    PLIST_ENTRY const ListHead = &gci->ExeAliasList;
    PLIST_ENTRY ListNext = ListHead->Flink;
    while (ListNext != ListHead)
    {
        PEXE_ALIAS_LIST const AliasList = CONTAINING_RECORD(ListNext, EXE_ALIAS_LIST, ListLink);
        ListNext = ListNext->Flink;
        FreeAliasList(AliasList);
    }
}

// Routine Description:
// - Adds a command line alias to the global set.
// - Converts and calls the W version of this function.
// Arguments:
// - psSourceBuffer - The shorthand/alias or source buffer to set
// - cchSourceBufferLength - Length in characters of source buffer
// - psTargetBuffer - The destination/expansion or target buffer to set
// - cchTargetBufferLength - Length in characters of target buffer
// - psExeNameBuffer - The client EXE application attached to the host to whom this substitution will apply
// - cchExeNameBufferLength - Length in characters of EXE name buffer
// Return Value:
// - Check HRESULT with SUCCEEDED. Can return memory, safe math, safe string, or locale conversion errors.
HRESULT ApiRoutines::AddConsoleAliasAImpl(_In_reads_or_z_(cchSourceBufferLength) const char* const psSourceBuffer,
                                          _In_ size_t const cchSourceBufferLength,
                                          _In_reads_or_z_(cchTargetBufferLength) const char* const psTargetBuffer,
                                          _In_ size_t const cchTargetBufferLength,
                                          _In_reads_or_z_(cchExeNameBufferLength) const char* const psExeNameBuffer,
                                          _In_ size_t const cchExeNameBufferLength)
{
    const CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    UINT const uiCodePage = gci->CP;

    wistd::unique_ptr<wchar_t[]> pwsSource;
    size_t cchSource;
    RETURN_IF_FAILED(ConvertToW(uiCodePage, psSourceBuffer, cchSourceBufferLength, pwsSource, cchSource));

    wistd::unique_ptr<wchar_t[]> pwsTarget;
    size_t cchTarget;
    RETURN_IF_FAILED(ConvertToW(uiCodePage, psTargetBuffer, cchTargetBufferLength, pwsTarget, cchTarget));

    wistd::unique_ptr<wchar_t[]> pwsExeName;
    size_t cchExeName;
    RETURN_IF_FAILED(ConvertToW(uiCodePage, psExeNameBuffer, cchExeNameBufferLength, pwsExeName, cchExeName));

    return AddConsoleAliasWImpl(pwsSource.get(), cchSource, pwsTarget.get(), cchTarget, pwsExeName.get(), cchExeName);
}

// Routine Description:
// - Adds a command line alias to the global set.
// Arguments:
// - pwsSourceBuffer - The shorthand/alias or source buffer to set
// - cchSourceBufferLength - Length in characters of source buffer
// - pwsTargetBuffer - The destination/expansion or target buffer to set
// - cchTargetBufferLength - Length in characters of target buffer
// - pwsExeNameBuffer - The client EXE application attached to the host to whom this substitution will apply
// - cchExeNameBufferLength - Length in characters of EXE name buffer
// Return Value:
// - Check HRESULT with SUCCEEDED. Can return memory, safe math, safe string, or locale conversion errors.
HRESULT ApiRoutines::AddConsoleAliasWImpl(_In_reads_or_z_(cchSourceBufferLength) const wchar_t* const pwsSourceBuffer,
                                          _In_ size_t const cchSourceBufferLength,
                                          _In_reads_or_z_(cchTargetBufferLength) const wchar_t* const pwsTargetBuffer,
                                          _In_ size_t const cchTargetBufferLength,
                                          _In_reads_or_z_(cchExeNameBufferLength) const wchar_t* const pwsExeNameBuffer,
                                          _In_ size_t const cchExeNameBufferLength)
{
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    RETURN_HR_IF(E_INVALIDARG, cchSourceBufferLength == 0);

    // Convert size_ts into SHORTs for existing alias functions to use.
    USHORT cbExeNameBufferLength;
    RETURN_IF_FAILED(GetUShortByteCount(cchExeNameBufferLength, &cbExeNameBufferLength));
    USHORT cbSourceBufferLength;
    RETURN_IF_FAILED(GetUShortByteCount(cchSourceBufferLength, &cbSourceBufferLength));
    USHORT cbTargetBufferLength;
    RETURN_IF_FAILED(GetUShortByteCount(cchTargetBufferLength, &cbTargetBufferLength));

    // find specified exe.  if it's not there, add it if we're not removing an alias.
    PEXE_ALIAS_LIST ExeAliasList = FindExe((LPVOID)pwsExeNameBuffer, cbExeNameBufferLength, TRUE);
    if (ExeAliasList != nullptr)
    {
        PALIAS Alias = FindAlias(ExeAliasList, pwsSourceBuffer, cbSourceBufferLength);
        if (cbTargetBufferLength > 0)
        {
            if (Alias != nullptr)
            {
                RETURN_NTSTATUS(ReplaceAlias(Alias, pwsTargetBuffer, cbTargetBufferLength));
            }
            else
            {
                RETURN_NTSTATUS(AddAlias(ExeAliasList, pwsSourceBuffer, cbSourceBufferLength, pwsTargetBuffer, cbTargetBufferLength));
            }
        }
        else
        {
            if (Alias != nullptr)
            {
                RETURN_NTSTATUS(RemoveAlias(Alias));
            }
        }
    }
    else
    {
        if (cbTargetBufferLength > 0)
        {
            ExeAliasList = AddExeAliasList((LPVOID)pwsExeNameBuffer, cbExeNameBufferLength, TRUE);
            if (ExeAliasList != nullptr)
            {
                RETURN_NTSTATUS(AddAlias(ExeAliasList, pwsSourceBuffer, cbSourceBufferLength, pwsTargetBuffer, cbTargetBufferLength));
            }
            else
            {
                RETURN_HR(E_OUTOFMEMORY);
            }
        }
    }

    return S_OK;
}

// Routine Description:
// - Retrieves a command line alias from the global set.
// - It is permitted to call this function without having a target buffer. Use the result to allocate
//   the appropriate amount of space and call again.
// - This behavior exists to allow the A version of the function to help allocate the right temp buffer for conversion of
//   the output/result data.
// Arguments:
// - pwsSourceBuffer - The shorthand/alias or source buffer to use in lookup
// - cchSourceBufferLength - Length in characters of source buffer
// - pwsTargetBuffer - The destination/expansion or target buffer we are attempting to retrieve. Optionally nullptr to retrieve needed space.
// - cchTargetBufferLength - Length in characters of target buffer. Set to 0 when pwsTargetBuffer is nullptr.
// - pcchTargetBufferWrittenOrNeeded - Pointer to space that will specify how many characters were written (if pwsTargetBuffer is valid)
//                                     or how many characters would have been consumed (if pwsTargetBuffer was valid.)
// - pwsExeNameBuffer - The client EXE application attached to the host whose set we should check
// - cchExeNameBufferLength - Length in characters of EXE name buffer
// Return Value:
// - Check HRESULT with SUCCEEDED. Can return memory, safe math, safe string, or locale conversion errors.
HRESULT GetConsoleAliasWImplHelper(_In_reads_or_z_(cchSourceBufferLength) const wchar_t* const pwsSourceBuffer,
                                   _In_ size_t const cchSourceBufferLength,
                                   _Out_writes_to_opt_(cchTargetBufferLength, *pcchTargetBufferWrittenOrNeeded) _Always_(_Post_z_) wchar_t* const pwsTargetBuffer,
                                   _In_ size_t const cchTargetBufferLength,
                                   _Out_ size_t* const pcchTargetBufferWrittenOrNeeded,
                                   _In_reads_or_z_(cchExeNameBufferLength) const wchar_t* const pwsExeNameBuffer,
                                   _In_ size_t const cchExeNameBufferLength)
{
    // Ensure output variables are initialized
    *pcchTargetBufferWrittenOrNeeded = 0;
    if (nullptr != pwsTargetBuffer && cchTargetBufferLength > 0)
    {
        *pwsTargetBuffer = L'\0';
    }

    // Convert size_ts into SHORTs for existing alias functions to use.
    USHORT cbExeNameBufferLength;
    RETURN_IF_FAILED(GetUShortByteCount(cchExeNameBufferLength, &cbExeNameBufferLength));
    USHORT cbSourceBufferLength;
    RETURN_IF_FAILED(GetUShortByteCount(cchSourceBufferLength, &cbSourceBufferLength));

    PEXE_ALIAS_LIST const pExeAliasList = FindExe((LPVOID)pwsExeNameBuffer, cbExeNameBufferLength, TRUE);
    RETURN_HR_IF(HRESULT_FROM_WIN32(ERROR_GEN_FAILURE), nullptr == pExeAliasList);

    PALIAS const pAlias = FindAlias(pExeAliasList, pwsSourceBuffer, cbSourceBufferLength);
    RETURN_HR_IF(HRESULT_FROM_WIN32(ERROR_GEN_FAILURE), nullptr == pAlias);

    // TargetLength is a byte count, convert to characters.
    size_t cchTarget = pAlias->TargetLength / sizeof(wchar_t);
    size_t const cchNull = 1;

    // The total space we need is the length of the string + the null terminator.
    size_t cchNeeded;
    RETURN_IF_FAILED(SizeTAdd(cchTarget, cchNull, &cchNeeded));

    *pcchTargetBufferWrittenOrNeeded = cchNeeded;

    if (nullptr != pwsTargetBuffer)
    {
        // if the user didn't give us enough space, return with insufficient buffer code early.
        RETURN_HR_IF(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER), cchTargetBufferLength < cchNeeded);

        RETURN_IF_FAILED(StringCchCopyNW(pwsTargetBuffer, cchTargetBufferLength, pAlias->Target, cchTarget));
    }

    return S_OK;
}

// Routine Description:
// - Retrieves a command line alias from the global set.
// - This function will convert input parameters from A to W, call the W version of the routine,
//   and attempt to convert the resulting data back to A for return.
// Arguments:
// - pwsSourceBuffer - The shorthand/alias or source buffer to use in lookup
// - cchSourceBufferLength - Length in characters of source buffer
// - pwsTargetBuffer - The destination/expansion or target buffer we are attempting to retrieve.
// - cchTargetBufferLength - Length in characters of target buffer.
// - pcchTargetBufferWritten - Pointer to space that will specify how many characters were written
// - pwsExeNameBuffer - The client EXE application attached to the host whose set we should check
// - cchExeNameBufferLength - Length in characters of EXE name buffer
// Return Value:
// - Check HRESULT with SUCCEEDED. Can return memory, safe math, safe string, or locale conversion errors.
HRESULT ApiRoutines::GetConsoleAliasAImpl(_In_reads_or_z_(cchSourceBufferLength) const char* const psSourceBuffer,
                                          _In_ size_t const cchSourceBufferLength,
                                          _Out_writes_to_(cchTargetBufferLength, *pcchTargetBufferWritten) _Always_(_Post_z_) char* const psTargetBuffer,
                                          _In_ size_t const cchTargetBufferLength,
                                          _Out_ size_t* const pcchTargetBufferWritten,
                                          _In_reads_or_z_(cchExeNameBufferLength) const char* const psExeNameBuffer,
                                          _In_ size_t const cchExeNameBufferLength)
{
    const CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    UINT const uiCodePage = gci->CP;

    // Ensure output variables are initialized
    *pcchTargetBufferWritten = 0;
    if (nullptr != psTargetBuffer && cchTargetBufferLength > 0)
    {
        *psTargetBuffer = L'\0';
    }

    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    // Convert our input parameters to Unicode.
    wistd::unique_ptr<wchar_t[]> pwsSource;
    size_t cchSource;
    RETURN_IF_FAILED(ConvertToW(uiCodePage, psSourceBuffer, cchSourceBufferLength, pwsSource, cchSource));

    wistd::unique_ptr<wchar_t[]> pwsExeName;
    size_t cchExeName;
    RETURN_IF_FAILED(ConvertToW(uiCodePage, psExeNameBuffer, cchExeNameBufferLength, pwsExeName, cchExeName));

    // Figure out how big our temporary Unicode buffer must be to retrieve output
    size_t cchTargetBufferNeeded;
    RETURN_IF_FAILED(GetConsoleAliasWImplHelper(pwsSource.get(), cchSource, nullptr, 0, &cchTargetBufferNeeded, pwsExeName.get(), cchExeName));

    // If there's nothing to get, then simply return.
    RETURN_HR_IF(S_OK, 0 == cchTargetBufferNeeded);

    // If the user hasn't given us a buffer at all and we need one, return an error.
    RETURN_HR_IF(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER), 0 == cchTargetBufferLength);

    // Allocate a unicode buffer of the right size.
    wistd::unique_ptr<wchar_t[]> pwsTarget = wil::make_unique_nothrow<wchar_t[]>(cchTargetBufferNeeded);
    RETURN_IF_NULL_ALLOC(pwsTarget);

    // Call the Unicode version of this method
    size_t cchTargetBufferWritten;
    RETURN_IF_FAILED(GetConsoleAliasWImplHelper(pwsSource.get(), cchSource, pwsTarget.get(), cchTargetBufferNeeded, &cchTargetBufferWritten, pwsExeName.get(), cchExeName));

    // Set the return size copied to the size given before we attempt to copy.
    // Then multiply by sizeof(wchar_t) due to a long standing bug that we must preserve for compatibility.
    // On failure, the API has historically given back this value.
    *pcchTargetBufferWritten = cchTargetBufferLength * sizeof(wchar_t);

    // Convert result to A
    wistd::unique_ptr<char[]> psConverted;
    size_t cchConverted;
    RETURN_IF_FAILED(ConvertToA(uiCodePage, pwsTarget.get(), cchTargetBufferWritten, psConverted, cchConverted));

    // Copy safely to output buffer
    RETURN_IF_FAILED(StringCchCopyNA(psTargetBuffer, cchTargetBufferLength, psConverted.get(), cchConverted));

    // And return the size copied.
    *pcchTargetBufferWritten = cchConverted;

    return S_OK;
}

// Routine Description:
// - Retrieves a command line alias from the global set.
// Arguments:
// - pwsSourceBuffer - The shorthand/alias or source buffer to use in lookup
// - cchSourceBufferLength - Length in characters of source buffer
// - pwsTargetBuffer - The destination/expansion or target buffer we are attempting to retrieve.
// - cchTargetBufferLength - Length in characters of target buffer.
// - pcchTargetBufferWritten - Pointer to space that will specify how many characters were written
// - pwsExeNameBuffer - The client EXE application attached to the host whose set we should check
// - cchExeNameBufferLength - Length in characters of EXE name buffer
// Return Value:
// - Check HRESULT with SUCCEEDED. Can return memory, safe math, safe string, or locale conversion errors.
HRESULT ApiRoutines::GetConsoleAliasWImpl(_In_reads_or_z_(cchSourceBufferLength) const wchar_t* const pwsSourceBuffer,
                                          _In_ size_t const cchSourceBufferLength,
                                          _Out_writes_to_(cchTargetBufferLength, *pcchTargetBufferWritten) _Always_(_Post_z_) wchar_t* const pwsTargetBuffer,
                                          _In_ size_t const cchTargetBufferLength,
                                          _Out_ size_t* const pcchTargetBufferWritten,
                                          _In_reads_or_z_(cchExeNameBufferLength) const wchar_t* const pwsExeNameBuffer,
                                          _In_ size_t const cchExeNameBufferLength)
{
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    HRESULT hr = GetConsoleAliasWImplHelper(pwsSourceBuffer, cchSourceBufferLength, pwsTargetBuffer, cchTargetBufferLength, pcchTargetBufferWritten, pwsExeNameBuffer, cchExeNameBufferLength);

    if (FAILED(hr))
    {
        *pcchTargetBufferWritten = cchTargetBufferLength;
    }

    return hr;
}

// These variables define the seperator character and the length of the string.
// They will be used to as the joiner between source and target strings when returning alias data in list form.
static PCWSTR const pwszAliasesSeperator = L"=";
static size_t const cchAliasesSeperator = wcslen(pwszAliasesSeperator);

// Routine Description:
// - Retrieves the amount of space needed to hold all aliases (source=target pairs) for the given EXE name
// - Works for both Unicode and Multibyte text.
// - This method configuration is called for both A/W routines to allow us an efficient way of asking the system
//   the lengths of how long each conversion would be without actually performing the full allocations/conversions.
// Arguments:
// - pwsExeNameBuffer - The client EXE application attached to the host whose set we should check
// - cchExeNameBufferLength - Length in characters of EXE name buffer
// - fCountInUnicode - True for W version (UCS-2 Unicode) calls. False for A version calls (all multibyte formats.)
// - uiCodePage - Set to valid Windows Codepage for A version calls. Ignored for W (but typically just set to 0.)
// - pcchAliasesBufferRequired - Pointer to receive the length of buffer that would be required to retrieve all aliases for the given exe.
// Return Value:
// - Check HRESULT with SUCCEEDED. Can return memory, safe math, safe string, or locale conversion errors.
HRESULT GetConsoleAliasesLengthWImplHelper(_In_reads_or_z_(cchExeNameBufferLength) const wchar_t* const pwsExeNameBuffer,
                                           _In_ size_t const cchExeNameBufferLength,
                                           _In_ bool const fCountInUnicode,
                                           _In_ UINT const uiCodePage,
                                           _Out_ size_t* const pcchAliasesBufferRequired)
{
    // Ensure output variables are initialized
    *pcchAliasesBufferRequired = 0;

    // Convert size_ts into SHORTs for existing alias functions to use.
    USHORT cbExeNameBufferLength;
    RETURN_IF_FAILED(GetUShortByteCount(cchExeNameBufferLength, &cbExeNameBufferLength));

    PEXE_ALIAS_LIST const pExeAliasList = FindExe((PVOID)pwsExeNameBuffer, cbExeNameBufferLength, TRUE);
    if (nullptr != pExeAliasList)
    {
        size_t cchNeeded = 0;

        // Each of the aliases will be made up of the source, a seperator, the target, then a null character.
        // They are of the form "Source=Target" when returned.

        size_t const cchNull = 1;
        size_t cchSeperator = cchAliasesSeperator;

        // If we're counting how much multibyte space will be needed, trial convert the seperator before we add.
        if (!fCountInUnicode)
        {
            RETURN_IF_FAILED(GetALengthFromW(uiCodePage, pwszAliasesSeperator, cchSeperator, &cchSeperator));
        }

        PLIST_ENTRY const ListHead = &pExeAliasList->AliasList;
        PLIST_ENTRY ListNext = ListHead->Flink;
        while (ListNext != ListHead)
        {
            PALIAS Alias = CONTAINING_RECORD(ListNext, ALIAS, ListLink);

            // Alias stores lengths in bytes.
            size_t cchSource = Alias->SourceLength / sizeof(wchar_t);
            size_t cchTarget = Alias->TargetLength / sizeof(wchar_t);

            // If we're counting how much multibyte space will be needed, trial convert the source and target strings before we add.
            if (!fCountInUnicode)
            {
                RETURN_IF_FAILED(GetALengthFromW(uiCodePage, Alias->Source, cchSource, &cchSource));
                RETURN_IF_FAILED(GetALengthFromW(uiCodePage, Alias->Target, cchTarget, &cchTarget));
            }

            // Accumulate all sizes to the final string count.
            RETURN_IF_FAILED(SizeTAdd(cchNeeded, cchSource, &cchNeeded));
            RETURN_IF_FAILED(SizeTAdd(cchNeeded, cchSeperator, &cchNeeded));
            RETURN_IF_FAILED(SizeTAdd(cchNeeded, cchTarget, &cchNeeded));
            RETURN_IF_FAILED(SizeTAdd(cchNeeded, cchNull, &cchNeeded));

            ListNext = ListNext->Flink;
        }

        *pcchAliasesBufferRequired = cchNeeded;
    }

    return S_OK;
}

// Routine Description:
// - Retrieves the amount of space needed to hold all aliases (source=target pairs) for the given EXE name
// - Converts input text from A to W then makes the call to the W implementation.
// Arguments:
// - pwsExeNameBuffer - The client EXE application attached to the host whose set we should check
// - cchExeNameBufferLength - Length in characters of EXE name buffer
// - pcchAliasesBufferRequired - Pointer to receive the length of buffer that would be required to retrieve all aliases for the given exe.
// Return Value:
// - Check HRESULT with SUCCEEDED. Can return memory, safe math, safe string, or locale conversion errors.
HRESULT ApiRoutines::GetConsoleAliasesLengthAImpl(_In_reads_or_z_(cchExeNameBufferLength) const char* const psExeNameBuffer,
                                                  _In_ size_t const cchExeNameBufferLength,
                                                  _Out_ size_t* const pcchAliasesBufferRequired)
{
    const CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    UINT const uiCodePage = gci->CP;

    // Ensure output variables are initialized
    *pcchAliasesBufferRequired = 0;

    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    // Convert our input parameters to Unicode
    wistd::unique_ptr<wchar_t[]> pwsExeName;
    size_t cchExeName;
    RETURN_IF_FAILED(ConvertToW(uiCodePage, psExeNameBuffer, cchExeNameBufferLength, pwsExeName, cchExeName));

    return GetConsoleAliasesLengthWImplHelper(pwsExeName.get(), cchExeName, false, uiCodePage, pcchAliasesBufferRequired);
}

// Routine Description:
// - Retrieves the amount of space needed to hold all aliases (source=target pairs) for the given EXE name
// - Converts input text from A to W then makes the call to the W implementation.
// Arguments:
// - pwsExeNameBuffer - The client EXE application attached to the host whose set we should check
// - cchExeNameBufferLength - Length in characters of EXE name buffer
// - pcchAliasesBufferRequired - Pointer to receive the length of buffer that would be required to retrieve all aliases for the given exe.
// Return Value:
// - Check HRESULT with SUCCEEDED. Can return memory, safe math, safe string, or locale conversion errors.
HRESULT ApiRoutines::GetConsoleAliasesLengthWImpl(_In_reads_or_z_(cchExeNameBufferLength) const wchar_t* const pwsExeNameBuffer,
                                                  _In_ size_t const cchExeNameBufferLength,
                                                  _Out_ size_t* const pcchAliasesBufferRequired)
{
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    return GetConsoleAliasesLengthWImplHelper(pwsExeNameBuffer, cchExeNameBufferLength, true, 0, pcchAliasesBufferRequired);
}

VOID ClearCmdExeAliases()
{
    PEXE_ALIAS_LIST const ExeAliasList = FindExe(L"cmd.exe", 14, TRUE);
    if (ExeAliasList == nullptr)
    {
        return;
    }

    PLIST_ENTRY const ListHead = &ExeAliasList->AliasList;
    PLIST_ENTRY ListNext = ListHead->Flink;
    while (ListNext != ListHead)
    {
        PALIAS const Alias = CONTAINING_RECORD(ListNext, ALIAS, ListLink);
        ListNext = ListNext->Flink;
        RemoveAlias(Alias);
    }
}

// Routine Description:
// - Retrieves all source=target pairs representing alias definitions for a given EXE name
// - It is permitted to call this function without having a target buffer. Use the result to allocate
//   the appropriate amount of space and call again.
// - This behavior exists to allow the A version of the function to help allocate the right temp buffer for conversion of
//   the output/result data.
// Arguments:
// - pwsExeNameBuffer - The client EXE application attached to the host whose set we should check
// - cchExeNameBufferLength - Length in characters of EXE name buffer
// - pwsAliasBuffer - The target buffer to hold all alias pairs we are trying to retrieve.
//                    Optionally nullptr to retrieve needed space.
// - cchAliasBufferLength - Length in characters of target buffer. Set to 0 when buffer is nullptr.
// - pcchAliasBufferWrittenOrNeeded - Pointer to space that will specify how many characters were written (if buffer is valid)
//                                     or how many characters would have been consumed.
// Return Value:
// - Check HRESULT with SUCCEEDED. Can return memory, safe math, safe string, or locale conversion errors.
HRESULT GetConsoleAliasesWImplHelper(_In_reads_or_z_(cchExeNameBufferLength) const wchar_t* const pwsExeNameBuffer,
                                     _In_ size_t const cchExeNameBufferLength,
                                     _Out_writes_to_opt_(cchAliasBufferLength, *pcchAliasBufferWrittenOrNeeded) _Always_(_Post_z_) wchar_t* const pwsAliasBuffer,
                                     _In_ size_t const cchAliasBufferLength,
                                     _Out_ size_t* const pcchAliasBufferWrittenOrNeeded)
{
    // Ensure output variables are initialized.
    *pcchAliasBufferWrittenOrNeeded = 0;
    if (nullptr != pwsAliasBuffer)
    {
        *pwsAliasBuffer = L'\0';
    }

    // Convert size_ts into SHORTs for existing alias functions to use.
    USHORT cbExeNameBufferLength;
    RETURN_IF_FAILED(GetUShortByteCount(cchExeNameBufferLength, &cbExeNameBufferLength));

    PEXE_ALIAS_LIST const pExeAliasList = FindExe((LPVOID)pwsExeNameBuffer, cbExeNameBufferLength, TRUE);
    if (nullptr != pExeAliasList)
    {
        LPWSTR AliasesBufferPtrW = pwsAliasBuffer;
        size_t cchTotalLength = 0; // accumulate the characters we need/have copied as we walk the list

        // Each of the alises will be made up of the source, a seperator, the target, then a null character.
        // They are of the form "Source=Target" when returned.

        size_t const cchNull = 1;

        PLIST_ENTRY const ListHead = &pExeAliasList->AliasList;
        PLIST_ENTRY ListNext = ListHead->Flink;
        while (ListNext != ListHead)
        {
            PALIAS const Alias = CONTAINING_RECORD(ListNext, ALIAS, ListLink);

            // Alias stores lengths in bytes.
            size_t const cchSource = Alias->SourceLength / sizeof(wchar_t);
            size_t const cchTarget = Alias->TargetLength / sizeof(wchar_t);

            // Add up how many characters we will need for the full alias data.
            size_t cchNeeded = 0;
            RETURN_IF_FAILED(SizeTAdd(cchNeeded, cchSource, &cchNeeded));
            RETURN_IF_FAILED(SizeTAdd(cchNeeded, cchAliasesSeperator, &cchNeeded));
            RETURN_IF_FAILED(SizeTAdd(cchNeeded, cchTarget, &cchNeeded));
            RETURN_IF_FAILED(SizeTAdd(cchNeeded, cchNull, &cchNeeded));

            // If we can return the data, attempt to do so until we're done or it overflows.
            // If we cannot return data, we're just going to loop anyway and count how much space we'd need.
            if (nullptr != pwsAliasBuffer)
            {
                // Calculate the new final total after we add what we need to see if it will exceed the limit
                size_t cchNewTotal;
                RETURN_IF_FAILED(SizeTAdd(cchTotalLength, cchNeeded, &cchNewTotal));

                RETURN_HR_IF(HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW), cchNewTotal > cchAliasBufferLength);

                size_t cchAliasBufferRemaining;
                RETURN_IF_FAILED(SizeTSub(cchAliasBufferLength, cchTotalLength, &cchAliasBufferRemaining));

                RETURN_IF_FAILED(StringCchCopyNW(AliasesBufferPtrW, cchAliasBufferRemaining, Alias->Source, cchSource));
                RETURN_IF_FAILED(SizeTSub(cchAliasBufferRemaining, cchSource, &cchAliasBufferRemaining));
                AliasesBufferPtrW += cchSource;

                RETURN_IF_FAILED(StringCchCopyNW(AliasesBufferPtrW, cchAliasBufferRemaining, pwszAliasesSeperator, cchAliasesSeperator));
                RETURN_IF_FAILED(SizeTSub(cchAliasBufferRemaining, cchAliasesSeperator, &cchAliasBufferRemaining));
                AliasesBufferPtrW += cchAliasesSeperator;

                RETURN_IF_FAILED(StringCchCopyNW(AliasesBufferPtrW, cchAliasBufferRemaining, Alias->Target, cchTarget));
                RETURN_IF_FAILED(SizeTSub(cchAliasBufferRemaining, cchTarget, &cchAliasBufferRemaining));
                AliasesBufferPtrW += cchTarget;

                // StringCchCopyNW ensures that the destination string is null terminated, so simply advance the pointer.
                RETURN_IF_FAILED(SizeTSub(cchAliasBufferRemaining, 1, &cchAliasBufferRemaining));
                AliasesBufferPtrW += cchNull;
            }

            RETURN_IF_FAILED(SizeTAdd(cchTotalLength, cchNeeded, &cchTotalLength));

            ListNext = ListNext->Flink;
        }

        *pcchAliasBufferWrittenOrNeeded = cchTotalLength;
    }

    return S_OK;
}

// Routine Description:
// - Retrieves all source=target pairs representing alias definitions for a given EXE name
// - Will convert all input from A to W, call the W version of the function, then convert resulting W to A text and return.
// Arguments:
// - psExeNameBuffer - The client EXE application attached to the host whose set we should check
// - cchExeNameBufferLength - Length in characters of EXE name buffer
// - psAliasBuffer - The target buffer to hold all alias pairs we are trying to retrieve.
// - cchAliasBufferLength - Length in characters of target buffer. Set to 0 when buffer is nullptr.
// - pcchAliasBufferWritten - Pointer to space that will specify how many characters were written
// Return Value:
// - Check HRESULT with SUCCEEDED. Can return memory, safe math, safe string, or locale conversion errors.
HRESULT ApiRoutines::GetConsoleAliasesAImpl(_In_reads_or_z_(cchExeNameBufferLength) const char* const psExeNameBuffer,
                                            _In_ size_t const cchExeNameBufferLength,
                                            _Out_writes_to_(cchAliasBufferLength, *pcchAliasBufferWritten) _Always_(_Post_z_) char* const psAliasBuffer,
                                            _In_ size_t const cchAliasBufferLength,
                                            _Out_ size_t* const pcchAliasBufferWritten)
{
    const CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    UINT const uiCodePage = gci->CP;

    // Ensure output variables are initialized
    *pcchAliasBufferWritten = 0;
    *psAliasBuffer = '\0';

    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    // Convert our input parameters to Unicode.
    wistd::unique_ptr<wchar_t[]> pwsExeName;
    size_t cchExeName;
    RETURN_IF_FAILED(ConvertToW(uiCodePage, psExeNameBuffer, cchExeNameBufferLength, pwsExeName, cchExeName));

    // Figure out how big our temporary Unicode buffer must be to retrieve output
    size_t cchAliasBufferNeeded;
    RETURN_IF_FAILED(GetConsoleAliasesWImplHelper(pwsExeName.get(), cchExeName, nullptr, 0, &cchAliasBufferNeeded));

    // If there's nothing to get, then simply return.
    RETURN_HR_IF(S_OK, 0 == cchAliasBufferNeeded);

    // Allocate a unicode buffer of the right size.
    wistd::unique_ptr<wchar_t[]> pwsAlias = wil::make_unique_nothrow<wchar_t[]>(cchAliasBufferNeeded);
    RETURN_IF_NULL_ALLOC(pwsAlias);

    // Call the Unicode version of this method
    size_t cchAliasBufferWritten;
    RETURN_IF_FAILED(GetConsoleAliasesWImplHelper(pwsExeName.get(), cchExeName, pwsAlias.get(), cchAliasBufferNeeded, &cchAliasBufferWritten));

    // Convert result to A
    wistd::unique_ptr<char[]> psConverted;
    size_t cchConverted;
    RETURN_IF_FAILED(ConvertToA(uiCodePage, pwsAlias.get(), cchAliasBufferWritten, psConverted, cchConverted));

    // Copy safely to the output buffer
    // - Aliases are a series of null terminated strings. We cannot use a SafeString function to copy.
    //   So instead, validate and use raw memory copy.
    RETURN_HR_IF(HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW), cchConverted > cchAliasBufferLength);
    memcpy_s(psAliasBuffer, cchAliasBufferLength, psConverted.get(), cchConverted);

    // And return the size copied.
    *pcchAliasBufferWritten = cchConverted;

    return S_OK;
}

// Routine Description:
// - Retrieves all source=target pairs representing alias definitions for a given EXE name
// Arguments:
// - pwsExeNameBuffer - The client EXE application attached to the host whose set we should check
// - cchExeNameBufferLength - Length in characters of EXE name buffer
// - pwsAliasBuffer - The target buffer to hold all alias pairs we are trying to retrieve.
// - cchAliasBufferLength - Length in characters of target buffer. Set to 0 when buffer is nullptr.
// - pcchAliasBufferWritten - Pointer to space that will specify how many characters were written
// Return Value:
// - Check HRESULT with SUCCEEDED. Can return memory, safe math, safe string, or locale conversion errors.
HRESULT ApiRoutines::GetConsoleAliasesWImpl(_In_reads_or_z_(cchExeNameBufferLength) const wchar_t* const pwsExeNameBuffer,
                                            _In_ size_t const cchExeNameBufferLength,
                                            _Out_writes_to_(cchAliasBufferLength, *pcchAliasBufferWritten) _Always_(_Post_z_) wchar_t* const pwsAliasBuffer,
                                            _In_ size_t const cchAliasBufferLength,
                                            _Out_ size_t* const pcchAliasBufferWritten)
{
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    return GetConsoleAliasesWImplHelper(pwsExeNameBuffer, cchExeNameBufferLength, pwsAliasBuffer, cchAliasBufferLength, pcchAliasBufferWritten);
}

// Routine Description:
// - Retrieves the amount of space needed to hold all EXE names with aliases defined that are known to the console
// - Works for both Unicode and Multibyte text.
// - This method configuration is called for both A/W routines to allow us an efficient way of asking the system
//   the lengths of how long each conversion would be without actually performing the full allocations/conversions.
// Arguments:
// - fCountInUnicode - True for W version (UCS-2 Unicode) calls. False for A version calls (all multibyte formats.)
// - uiCodePage - Set to valid Windows Codepage for A version calls. Ignored for W (but typically just set to 0.)
// - pcchAliasExesBufferRequired - Pointer to receive the length of buffer that would be required to retrieve all relevant EXE names.
// Return Value:
// - Check HRESULT with SUCCEEDED. Can return memory, safe math, safe string, or locale conversion errors.
HRESULT GetConsoleAliasExesLengthImplHelper(_In_ bool const fCountInUnicode, _In_ UINT const uiCodePage, _Out_ size_t* const pcchAliasExesBufferRequired)
{
    CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    // Ensure output variables are initialized
    *pcchAliasExesBufferRequired = 0;

    size_t cchNeeded = 0;

    // Each alias exe will be made up of the string payload and a null terminator.
    size_t const cchNull = 1;

    PLIST_ENTRY const ListHead = &gci->ExeAliasList;
    PLIST_ENTRY ListNext = ListHead->Flink;
    while (ListNext != ListHead)
    {
        PEXE_ALIAS_LIST const AliasList = CONTAINING_RECORD(ListNext, EXE_ALIAS_LIST, ListLink);

        // AliasList stores lengths in bytes.
        size_t cchExe = AliasList->ExeLength / sizeof(wchar_t);

        // If we're counting how much multibyte space will be needed, trial convert the exe string before we add.
        if (!fCountInUnicode)
        {
            RETURN_IF_FAILED(GetALengthFromW(uiCodePage, AliasList->ExeName, cchExe, &cchExe));
        }

        // Accumulate to total
        RETURN_IF_FAILED(SizeTAdd(cchNeeded, cchExe, &cchNeeded));
        RETURN_IF_FAILED(SizeTAdd(cchNeeded, cchNull, &cchNeeded));

        ListNext = ListNext->Flink;
    }

    *pcchAliasExesBufferRequired = cchNeeded;

    return S_OK;
}

// Routine Description:
// - Retrieves the amount of space needed to hold all EXE names with aliases defined that are known to the console
// Arguments:
// - pcchAliasExesBufferRequired - Pointer to receive the length of buffer that would be required to retrieve all relevant EXE names.
// Return Value:
// - Check HRESULT with SUCCEEDED. Can return memory, safe math, safe string, or locale conversion errors.
HRESULT ApiRoutines::GetConsoleAliasExesLengthAImpl(_Out_ size_t* const pcchAliasExesBufferRequired)
{
    LockConsole();
    const CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    return GetConsoleAliasExesLengthImplHelper(false, gci->CP, pcchAliasExesBufferRequired);
}

// Routine Description:
// - Retrieves the amount of space needed to hold all EXE names with aliases defined that are known to the console
// Arguments:
// - pcchAliasExesBufferRequired - Pointer to receive the length of buffer that would be required to retrieve all relevant EXE names.
// Return Value:
// - Check HRESULT with SUCCEEDED. Can return memory, safe math, safe string, or locale conversion errors.
HRESULT ApiRoutines::GetConsoleAliasExesLengthWImpl(_Out_ size_t* const pcchAliasExesBufferRequired)
{
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    return GetConsoleAliasExesLengthImplHelper(true, 0, pcchAliasExesBufferRequired);
}

// Routine Description:
// - Retrieves all EXE names with aliases defined that are known to the console.
// - It is permitted to call this function without having a target buffer. Use the result to allocate
//   the appropriate amount of space and call again.
// - This behavior exists to allow the A version of the function to help allocate the right temp buffer for conversion of
//   the output/result data.
// Arguments:
// - pwsAliasExesBuffer - The target buffer to hold all known EXE names we are trying to retrieve.
//                        Optionally nullptr to retrieve needed space.
// - cchAliasExesBufferLength - Length in characters of target buffer. Set to 0 when buffer is nullptr.
// - pcchAliasExesBufferWrittenOrNeeded - Pointer to space that will specify how many characters were written (if buffer is valid)
//                                        or how many characters would have been consumed.
// Return Value:
// - Check HRESULT with SUCCEEDED. Can return memory, safe math, safe string, or locale conversion errors.
HRESULT GetConsoleAliasExesWImplHelper(_Out_writes_to_opt_(cchAliasExesBufferLength, *pcchAliasExesBufferWrittenOrNeeded) _Always_(_Post_z_) wchar_t* const pwsAliasExesBuffer,
                                       _In_ size_t const cchAliasExesBufferLength,
                                       _Out_ size_t* const pcchAliasExesBufferWrittenOrNeeded)
{
    CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    // Ensure output variables are initialized.
    *pcchAliasExesBufferWrittenOrNeeded = 0;
    if (nullptr != pwsAliasExesBuffer)
    {
        *pwsAliasExesBuffer = L'\0';
    }

    LPWSTR AliasExesBufferPtrW = pwsAliasExesBuffer;
    size_t cchTotalLength = 0; // accumulate the characters we need/have copied as we walk the list

    size_t const cchNull = 1;

    PLIST_ENTRY const ListHead = &gci->ExeAliasList;
    PLIST_ENTRY ListNext = ListHead->Flink;
    while (ListNext != ListHead)
    {
        PEXE_ALIAS_LIST const AliasList = CONTAINING_RECORD(ListNext, EXE_ALIAS_LIST, ListLink);

        // AliasList stores length in bytes. Add 1 for null terminator.
        size_t const cchExe = (AliasList->ExeLength) / sizeof(wchar_t);

        size_t cchNeeded;
        RETURN_IF_FAILED(SizeTAdd(cchExe, cchNull, &cchNeeded));

        // If we can return the data, attempt to do so until we're done or it overflows.
        // If we cannot return data, we're just going to loop anyway and count how much space we'd need.
        if (nullptr != pwsAliasExesBuffer)
        {
            // Calculate the new total length after we add to the buffer
            // Error out early if there is a problem.
            size_t cchNewTotal;
            RETURN_IF_FAILED(SizeTAdd(cchTotalLength, cchNeeded, &cchNewTotal));
            RETURN_HR_IF(HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW), cchNewTotal > cchAliasExesBufferLength);

            size_t cchRemaining;
            RETURN_IF_FAILED(SizeTSub(cchAliasExesBufferLength, cchTotalLength, &cchRemaining));

            RETURN_IF_FAILED(StringCchCopyNW(AliasExesBufferPtrW, cchRemaining, AliasList->ExeName, cchExe));
            AliasExesBufferPtrW += cchNeeded;
        }

        // Accumulate the total written amount.
        RETURN_IF_FAILED(SizeTAdd(cchTotalLength, cchNeeded, &cchTotalLength));

        ListNext = ListNext->Flink;
    }

    *pcchAliasExesBufferWrittenOrNeeded = cchTotalLength;

    return S_OK;
}

// Routine Description:
// - Retrieves all EXE names with aliases defined that are known to the console.
// - Will call the W version of the function and convert all text back to A on returning.
// Arguments:
// - psAliasExesBuffer - The target buffer to hold all known EXE names we are trying to retrieve.
// - cchAliasExesBufferLength - Length in characters of target buffer. Set to 0 when buffer is nullptr.
// - pcchAliasExesBufferWrittenOrNeeded - Pointer to space that will specify how many characters were written
// Return Value:
// - Check HRESULT with SUCCEEDED. Can return memory, safe math, safe string, or locale conversion errors.
HRESULT ApiRoutines::GetConsoleAliasExesAImpl(_Out_writes_to_(cchAliasExesBufferLength, *pcchAliasExesBufferWritten) _Always_(_Post_z_) char* const psAliasExesBuffer,
                                              _In_ size_t const cchAliasExesBufferLength,
                                              _Out_ size_t* const pcchAliasExesBufferWritten)
{
    const CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    UINT const uiCodePage = gci->CP;

    // Ensure output variables are initialized
    *pcchAliasExesBufferWritten = 0;
    *psAliasExesBuffer = '\0';

    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    // Figure our how big our temporary Unicode buffer must be to retrieve output
    size_t cchAliasExesBufferNeeded;
    RETURN_IF_FAILED(GetConsoleAliasExesWImplHelper(nullptr, 0, &cchAliasExesBufferNeeded));

    // If there's nothing to get, then simply return.
    RETURN_HR_IF(S_OK, 0 == cchAliasExesBufferNeeded);

    // Allocate a unicode buffer of the right size.
    wistd::unique_ptr<wchar_t[]> pwsTarget = wil::make_unique_nothrow<wchar_t[]>(cchAliasExesBufferNeeded);
    RETURN_IF_NULL_ALLOC(pwsTarget);

    // Call the Unicode version of this method
    size_t cchAliasExesBufferWritten;
    RETURN_IF_FAILED(GetConsoleAliasExesWImplHelper(pwsTarget.get(), cchAliasExesBufferNeeded, &cchAliasExesBufferWritten));

    // Convert result to A
    wistd::unique_ptr<char[]> psConverted;
    size_t cchConverted;
    RETURN_IF_FAILED(ConvertToA(uiCodePage, pwsTarget.get(), cchAliasExesBufferWritten, psConverted, cchConverted));

    // Copy safely to the output buffer
    // - AliasExes are a series of null terminated strings. We cannot use a SafeString function to copy.
    //   So instead, validate and use raw memory copy.
    RETURN_HR_IF(HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW), cchConverted > cchAliasExesBufferLength);
    memcpy_s(psAliasExesBuffer, cchAliasExesBufferLength, psConverted.get(), cchConverted);

    // And return the size copied.
    *pcchAliasExesBufferWritten = cchConverted;

    return S_OK;
}

// Routine Description:
// - Retrieves all EXE names with aliases defined that are known to the console.
// Arguments:
// - pwsAliasExesBuffer - The target buffer to hold all known EXE names we are trying to retrieve.
// - cchAliasExesBufferLength - Length in characters of target buffer. Set to 0 when buffer is nullptr.
// - pcchAliasExesBufferWrittenOrNeeded - Pointer to space that will specify how many characters were written
// Return Value:
// - Check HRESULT with SUCCEEDED. Can return memory, safe math, safe string, or locale conversion errors.
HRESULT ApiRoutines::GetConsoleAliasExesWImpl(_Out_writes_to_(cchAliasExesBufferLength, *pcchAliasExesBufferWritten) _Always_(_Post_z_)  wchar_t* const pwsAliasExesBuffer,
                                              _In_ size_t const cchAliasExesBufferLength,
                                              _Out_ size_t* const pcchAliasExesBufferWritten)
{
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    return GetConsoleAliasExesWImplHelper(pwsAliasExesBuffer, cchAliasExesBufferLength, pcchAliasExesBufferWritten);
}

#define MAX_ARGS 9

// Routine Description:
// - This routine matches the input string with an alias and copies the alias to the input buffer.
// Arguments:
// - pwchSource - string to match
// - cbSource - length of pwchSource in bytes
// - pwchTarget - where to store matched string
// - pcbTarget - on input, contains size of pwchTarget.  On output, contains length of alias stored in pwchTarget.
// - SourceIsCommandLine - if true, source buffer is a command line, where
//                         the first blank separate token is to be check for an alias, and if
//                         it matches, replaced with the value of the alias.  if false, then
//                         the source string is a null terminated alias name.
// - LineCount - aliases can contain multiple commands.  $T is the command separator
// Return Value:
// - SUCCESS - match was found and alias was copied to buffer.
NTSTATUS MatchAndCopyAlias(_In_reads_bytes_(cbSource) PWCHAR pwchSource,
                           _In_ USHORT cbSource,
                           _Out_writes_bytes_(*pcbTarget) PWCHAR pwchTarget,
                           _Inout_ PUSHORT pcbTarget,
                           _In_reads_bytes_(cbExe) PWCHAR pwchExe,
                           _In_ USHORT cbExe,
                           _Out_ PDWORD pcLines)
{
    NTSTATUS Status = STATUS_SUCCESS;

    // Alloc of exename may have failed.
    if (pwchExe == nullptr)
    {
        return STATUS_UNSUCCESSFUL;
    }

    // Find exe.
    PEXE_ALIAS_LIST const ExeAliasList = FindExe(pwchExe, cbExe, TRUE);
    if (ExeAliasList == nullptr)
    {
        return STATUS_UNSUCCESSFUL;
    }

    // Find first blank.
    PWCHAR Tmp = pwchSource;
    USHORT SourceUpToFirstBlank = 0; // in chars
#pragma prefast(suppress:26019, "Legacy. This is bounded appropriately by cbSource.")
    for (; *Tmp != (WCHAR)' ' && SourceUpToFirstBlank < (USHORT)(cbSource / sizeof(WCHAR)); Tmp++, SourceUpToFirstBlank++)
    {
        /* Do nothing */
    }

    // find char past first blank
    USHORT j = SourceUpToFirstBlank;
    while (j < (USHORT)(cbSource / sizeof(WCHAR)) && *Tmp == (WCHAR)' ')
    {
        Tmp++;
        j++;
    }

    LPWSTR SourcePtr = Tmp;
    USHORT const SourceRemainderLength = (USHORT)((cbSource / sizeof(WCHAR)) - j); // in chars

    // find alias
    PALIAS const Alias = FindAlias(ExeAliasList, pwchSource, (USHORT)(SourceUpToFirstBlank * sizeof(WCHAR)));
    if (Alias == nullptr)
    {
        return STATUS_UNSUCCESSFUL;
    }

    // Length is in bytes. Add 1 so dividing by WCHAR (2) is always rounding up.
    PWCHAR const TmpBuffer = new WCHAR[(*pcbTarget + 1) / sizeof(WCHAR)];
    if (!TmpBuffer)
    {
        return STATUS_NO_MEMORY;
    }

    // count args in target
    USHORT ArgCount = 0;
    *pcLines = 1;
    Tmp = Alias->Target;
    for (USHORT i = 0; (USHORT)(i + 1) < (USHORT)(Alias->TargetLength / sizeof(WCHAR)); i++)
    {
        if (*Tmp == (WCHAR)'$' && *(Tmp + 1) >= (WCHAR)'1' && *(Tmp + 1) <= (WCHAR)'9')
        {
            USHORT ArgNum = *(Tmp + 1) - (WCHAR)'0';

            if (ArgNum > ArgCount)
            {
                ArgCount = ArgNum;
            }

            Tmp++;
            i++;
        }
        else if (*Tmp == (WCHAR)'$' && *(Tmp + 1) == (WCHAR)'*')
        {
            if (ArgCount == 0)
            {
                ArgCount = 1;
            }

            Tmp++;
            i++;
        }

        Tmp++;
    }

    // Package up space separated strings in source into array of args.
    USHORT NumSourceArgs = 0;
    Tmp = SourcePtr;
    LPWSTR Args[MAX_ARGS];
    USHORT ArgsLength[MAX_ARGS];    // in bytes
    for (USHORT i = 0, k = 0; i < ArgCount; i++)
    {
        if (k < SourceRemainderLength)
        {
            Args[NumSourceArgs] = Tmp;
            ArgsLength[NumSourceArgs] = 0;
            while (k++ < SourceRemainderLength && *Tmp++ != (WCHAR)' ')
            {
                ArgsLength[NumSourceArgs] += sizeof(WCHAR);
            }

            while (k < SourceRemainderLength && *Tmp == (WCHAR)' ')
            {
                k++;
                Tmp++;
            }

            NumSourceArgs++;
        }
        else
        {
            break;
        }
    }

    // Put together the target string.
    PWCHAR Buffer = TmpBuffer;
    USHORT NewTargetLength = 2 * sizeof(WCHAR);    // for CRLF
    PWCHAR TargetAlias = Alias->Target;
    for (USHORT i = 0; i < (USHORT)(Alias->TargetLength / sizeof(WCHAR)); i++)
    {
        if (NewTargetLength >= *pcbTarget)
        {
            Status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        if (*TargetAlias == (WCHAR)'$' && (USHORT)(i + 1) < (USHORT)(Alias->TargetLength / sizeof(WCHAR)))
        {
            TargetAlias++;
            i++;
            if (*TargetAlias >= (WCHAR)'1' && *TargetAlias <= (WCHAR)'9')
            {
                // do numbered parameter substitution
                USHORT ArgNumber;

                ArgNumber = (USHORT)(*TargetAlias - (WCHAR)'1');
                if (ArgNumber < NumSourceArgs)
                {
                    if ((NewTargetLength + ArgsLength[ArgNumber]) <= *pcbTarget)
                    {
                        memmove(Buffer, Args[ArgNumber], ArgsLength[ArgNumber]);
                        Buffer += ArgsLength[ArgNumber] / sizeof(WCHAR);
                        NewTargetLength += ArgsLength[ArgNumber];
                    }
                    else
                    {
                        Status = STATUS_BUFFER_TOO_SMALL;
                        break;
                    }
                }
            }
            else if (*TargetAlias == (WCHAR)'*')
            {
                // Do * parameter substitution.
                if (NumSourceArgs)
                {
                    if ((USHORT)(NewTargetLength + (SourceRemainderLength * sizeof(WCHAR))) <= *pcbTarget)
                    {
                        memmove(Buffer, Args[0], SourceRemainderLength * sizeof(WCHAR));
                        Buffer += SourceRemainderLength;
                        NewTargetLength += SourceRemainderLength * sizeof(WCHAR);
                    }
                    else
                    {
                        Status = STATUS_BUFFER_TOO_SMALL;
                        break;
                    }
                }
            }
            else if (*TargetAlias == (WCHAR)'l' || *TargetAlias == (WCHAR)'L')
            {
                // Do < substitution.
                *Buffer++ = (WCHAR)'<';
                NewTargetLength += sizeof(WCHAR);
            }
            else if (*TargetAlias == (WCHAR)'g' || *TargetAlias == (WCHAR)'G')
            {
                // Do > substitution.
                *Buffer++ = (WCHAR)'>';
                NewTargetLength += sizeof(WCHAR);
            }
            else if (*TargetAlias == (WCHAR)'b' || *TargetAlias == (WCHAR)'B')
            {
                // Do | substitution.
                *Buffer++ = (WCHAR)'|';
                NewTargetLength += sizeof(WCHAR);
            }
            else if (*TargetAlias == (WCHAR)'t' || *TargetAlias == (WCHAR)'T')
            {
                // do newline substitution
                if ((USHORT)(NewTargetLength + (sizeof(WCHAR) * 2)) > *pcbTarget)
                {
                    Status = STATUS_BUFFER_TOO_SMALL;
                    break;
                }

                *pcLines += 1;
                *Buffer++ = UNICODE_CARRIAGERETURN;
                *Buffer++ = UNICODE_LINEFEED;
                NewTargetLength += sizeof(WCHAR) * 2;
            }
            else
            {
                // copy $X
                *Buffer++ = (WCHAR)'$';
                NewTargetLength += sizeof(WCHAR);
                *Buffer++ = *TargetAlias;
                NewTargetLength += sizeof(WCHAR);
            }
            TargetAlias++;
        }
        else
        {
            // copy char
            *Buffer++ = *TargetAlias++;
            NewTargetLength += sizeof(WCHAR);
        }
    }

    __analysis_assume(!NT_SUCCESS(Status) || NewTargetLength <= *pcbTarget);
    if (NT_SUCCESS(Status))
    {
        // We pre-reserve space for these two characters so we know there's enough room here.
        ASSERT(NewTargetLength <= *pcbTarget);
        *Buffer++ = UNICODE_CARRIAGERETURN;
        *Buffer++ = UNICODE_LINEFEED;

        memmove(pwchTarget, TmpBuffer, NewTargetLength);
    }

    delete[] TmpBuffer;
    *pcbTarget = NewTargetLength;

    return Status;
}
