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

#include "..\interactivity\inc\ServiceLocator.hpp"

#pragma hdrstop

struct case_insensitive_hash
{
    std::size_t operator()(const std::wstring& key) const
    {
        std::wstring lower(key);
        std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);
        std::hash<std::wstring> hash;
        return hash(lower);
    }
};

struct case_insensitive_equality
{
    bool operator()(const std::wstring& lhs, const std::wstring& rhs) const
    {
        return 0 == _wcsicmp(lhs.data(), rhs.data());
    }
};

std::unordered_map<std::wstring,
                   std::unordered_map<std::wstring,
                       std::wstring,
                       case_insensitive_hash,
                       case_insensitive_equality>,
                   case_insensitive_hash,
                   case_insensitive_equality> g_aliasData;

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
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    UINT const uiCodePage = gci.CP;

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

    try
    {
        std::wstring exeName(pwsExeNameBuffer, cchExeNameBufferLength);
        std::wstring source(pwsSourceBuffer, cchSourceBufferLength);
        std::wstring target(pwsTargetBuffer, cchTargetBufferLength);

        std::transform(exeName.begin(), exeName.end(), exeName.begin(), towlower);
        std::transform(source.begin(), source.end(), source.begin(), towlower);

        if (target.size() == 0)
        {
            // Only try to dig in and erase if the exeName exists.
            auto exeData = g_aliasData.find(exeName);
            if (exeData != g_aliasData.end())
            {
                g_aliasData[exeName].erase(source);
            }
        }
        else
        {
            // Map will auto-create each level as necessary
            g_aliasData[exeName][source] = target;
        }
    }
    CATCH_RETURN();

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

    try
    {
        std::wstring exeName(pwsExeNameBuffer, cchExeNameBufferLength);
        std::wstring source(pwsSourceBuffer, cchSourceBufferLength);

        // For compatibility, return ERROR_GEN_FAILURE for any result where the alias can't be found.
        // We use .find for the iterators then dereference to search without creating entries.
        const auto exeIter = g_aliasData.find(exeName);
        RETURN_HR_IF(HRESULT_FROM_WIN32(ERROR_GEN_FAILURE), exeIter == g_aliasData.end());
        const auto exeData = exeIter->second;
        const auto sourceIter = exeData.find(source);
        RETURN_HR_IF(HRESULT_FROM_WIN32(ERROR_GEN_FAILURE), sourceIter == exeData.end());
        const auto target = sourceIter->second;
        RETURN_HR_IF(HRESULT_FROM_WIN32(ERROR_GEN_FAILURE), target.size() == 0);

        // TargetLength is a byte count, convert to characters.
        size_t cchTarget = target.size();
        size_t const cchNull = 1;

        // The total space we need is the length of the string + the null terminator.
        size_t cchNeeded;
        RETURN_IF_FAILED(SizeTAdd(cchTarget, cchNull, &cchNeeded));

        *pcchTargetBufferWrittenOrNeeded = cchNeeded;

        if (nullptr != pwsTargetBuffer)
        {
            // if the user didn't give us enough space, return with insufficient buffer code early.
            RETURN_HR_IF(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER), cchTargetBufferLength < cchNeeded);

            RETURN_IF_FAILED(StringCchCopyNW(pwsTargetBuffer, cchTargetBufferLength, target.data(), cchTarget));
        }
    }
    CATCH_RETURN();

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
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    UINT const uiCodePage = gci.CP;

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

    try
    {
        std::wstring exeName(pwsExeNameBuffer, cchExeNameBufferLength);

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

        // Find without creating.
        auto exeIter = g_aliasData.find(exeName);
        if (exeIter != g_aliasData.end())
        {
            auto list = exeIter->second;
            for (auto& pair : list)
            {
                // Alias stores lengths in bytes.
                size_t cchSource = pair.first.size();
                size_t cchTarget = pair.second.size();

                // If we're counting how much multibyte space will be needed, trial convert the source and target strings before we add.
                if (!fCountInUnicode)
                {
                    RETURN_IF_FAILED(GetALengthFromW(uiCodePage, pair.first.data(), cchSource, &cchSource));
                    RETURN_IF_FAILED(GetALengthFromW(uiCodePage, pair.second.data(), cchTarget, &cchTarget));
                }

                // Accumulate all sizes to the final string count.
                RETURN_IF_FAILED(SizeTAdd(cchNeeded, cchSource, &cchNeeded));
                RETURN_IF_FAILED(SizeTAdd(cchNeeded, cchSeperator, &cchNeeded));
                RETURN_IF_FAILED(SizeTAdd(cchNeeded, cchTarget, &cchNeeded));
                RETURN_IF_FAILED(SizeTAdd(cchNeeded, cchNull, &cchNeeded));
            }
        }

        *pcchAliasesBufferRequired = cchNeeded;
    }
    CATCH_RETURN();

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
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    UINT const uiCodePage = gci.CP;

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

// Routine Description:
// - Clears all aliases on CMD.exe.
void Alias::s_ClearCmdExeAliases()
{
    // find without creating.
    auto exeIter = g_aliasData.find(L"cmd.exe");
    if (exeIter != g_aliasData.end())
    {
        exeIter->second.clear();
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

    try
    {
        std::wstring exeName(pwsExeNameBuffer, cchExeNameBufferLength);

        LPWSTR AliasesBufferPtrW = pwsAliasBuffer;
        size_t cchTotalLength = 0; // accumulate the characters we need/have copied as we walk the list

        // Each of the alises will be made up of the source, a seperator, the target, then a null character.
        // They are of the form "Source=Target" when returned.
        size_t const cchNull = 1;

        // Find without creating.
        auto exeIter = g_aliasData.find(exeName);
        if (exeIter != g_aliasData.end())
        {
            auto list = exeIter->second;
            for (auto& pair : list)
            {
                // Alias stores lengths in bytes.
                size_t const cchSource = pair.first.size();
                size_t const cchTarget = pair.second.size();

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

                    RETURN_IF_FAILED(StringCchCopyNW(AliasesBufferPtrW, cchAliasBufferRemaining, pair.first.data(), cchSource));
                    RETURN_IF_FAILED(SizeTSub(cchAliasBufferRemaining, cchSource, &cchAliasBufferRemaining));
                    AliasesBufferPtrW += cchSource;

                    RETURN_IF_FAILED(StringCchCopyNW(AliasesBufferPtrW, cchAliasBufferRemaining, pwszAliasesSeperator, cchAliasesSeperator));
                    RETURN_IF_FAILED(SizeTSub(cchAliasBufferRemaining, cchAliasesSeperator, &cchAliasBufferRemaining));
                    AliasesBufferPtrW += cchAliasesSeperator;

                    RETURN_IF_FAILED(StringCchCopyNW(AliasesBufferPtrW, cchAliasBufferRemaining, pair.second.data(), cchTarget));
                    RETURN_IF_FAILED(SizeTSub(cchAliasBufferRemaining, cchTarget, &cchAliasBufferRemaining));
                    AliasesBufferPtrW += cchTarget;

                    // StringCchCopyNW ensures that the destination string is null terminated, so simply advance the pointer.
                    RETURN_IF_FAILED(SizeTSub(cchAliasBufferRemaining, 1, &cchAliasBufferRemaining));
                    AliasesBufferPtrW += cchNull;
                }

                RETURN_IF_FAILED(SizeTAdd(cchTotalLength, cchNeeded, &cchTotalLength));
            }
        }

        *pcchAliasBufferWrittenOrNeeded = cchTotalLength;

    }
    CATCH_RETURN();

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
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    UINT const uiCodePage = gci.CP;

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
    // Ensure output variables are initialized
    *pcchAliasExesBufferRequired = 0;

    try
    {
        size_t cchNeeded = 0;

        // Each alias exe will be made up of the string payload and a null terminator.
        size_t const cchNull = 1;

        for (auto& pair : g_aliasData)
        {
            size_t cchExe = pair.first.size();

            // If we're counting how much multibyte space will be needed, trial convert the exe string before we add.
            if (!fCountInUnicode)
            {
                RETURN_IF_FAILED(GetALengthFromW(uiCodePage, pair.first.data(), cchExe, &cchExe));
            }

            // Accumulate to total
            RETURN_IF_FAILED(SizeTAdd(cchNeeded, cchExe, &cchNeeded));
            RETURN_IF_FAILED(SizeTAdd(cchNeeded, cchNull, &cchNeeded));
        }

        *pcchAliasExesBufferRequired = cchNeeded;
    }
    CATCH_RETURN();

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
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    return GetConsoleAliasExesLengthImplHelper(false, gci.CP, pcchAliasExesBufferRequired);
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
    // Ensure output variables are initialized.
    *pcchAliasExesBufferWrittenOrNeeded = 0;
    if (nullptr != pwsAliasExesBuffer)
    {
        *pwsAliasExesBuffer = L'\0';
    }

    try
    {
        LPWSTR AliasExesBufferPtrW = pwsAliasExesBuffer;
        size_t cchTotalLength = 0; // accumulate the characters we need/have copied as we walk the list

        size_t const cchNull = 1;

        for (auto& pair : g_aliasData)
        {
            // AliasList stores length in bytes. Add 1 for null terminator.
            size_t const cchExe = pair.first.size();

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

                RETURN_IF_FAILED(StringCchCopyNW(AliasExesBufferPtrW, cchRemaining, pair.first.data(), cchExe));
                AliasExesBufferPtrW += cchNeeded;
            }

            // Accumulate the total written amount.
            RETURN_IF_FAILED(SizeTAdd(cchTotalLength, cchNeeded, &cchTotalLength));

        }

        *pcchAliasExesBufferWrittenOrNeeded = cchTotalLength;
    }
    CATCH_RETURN();

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
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    UINT const uiCodePage = gci.CP;

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

// Routine Description:
// - Trims trailing \r\n off of a string
// Arguments:
// - str - String to trim
void Alias::s_TrimTrailingCrLf(std::wstring& str)
{
    const auto trailingCrLfPos = str.find_last_of(UNICODE_CARRIAGERETURN);
    if (std::wstring::npos != trailingCrLfPos)
    {
        str.erase(trailingCrLfPos);
    }
}

// Routine Description:
// - Tokenizes a string into a collection using space as a separator
// Arguments:
// - str - String to tokenize
// Return Value:
// - Collection of tokenized strings
std::deque<std::wstring> Alias::s_Tokenize(const std::wstring& str)
{
    std::deque<std::wstring> result;

    size_t prevIndex = 0;
    auto spaceIndex = str.find(L' ');
    while (std::wstring::npos != spaceIndex)
    {
        const auto length = spaceIndex - prevIndex;

        result.emplace_back(str.substr(prevIndex, length));

        spaceIndex++;
        prevIndex = spaceIndex;

        spaceIndex = str.find(L' ', spaceIndex);
    }

    // Place the final one into the set.
    result.emplace_back(str.substr(prevIndex));

    return result;
}

// Routine Description:
// - Gets just the arguments portion of the command string
//   Specifically, all text after the first space character.
// Arguments:
// - str - String to split into just args
// Return Value:
// - Only the arguments part of the string or empty if there are no arguments.
std::wstring Alias::s_GetArgString(const std::wstring& str)
{
    std::wstring result;
    auto firstSpace = str.find_first_of(L' ');
    if (std::wstring::npos != firstSpace)
    {
        firstSpace++;
        if (firstSpace < str.size())
        {
            result = str.substr(firstSpace);
        }
    }

    return result;
}

// Routine Description:
// - Checks the given character to see if it is a numbered arg replacement macro
//   and replaces it with the counted argument if there is a match
// Arguments:
// - ch - Character to test as a macro
// - appendToStr - Append the macro result here if it matched
// - tokens - Tokens of the original command string. 0 is alias. 1-N are arguments.
// Return Value:
// - True if we found the macro and appended to the string.
// - False if the given character doesn't match this macro.
bool Alias::s_TryReplaceNumberedArgMacro(const wchar_t ch,
                                         std::wstring& appendToStr,
                                         const std::deque<std::wstring>& tokens)
{
    if (ch >= L'1' && ch <= L'9')
    {
        // Numerical macros substitute that numbered argument
        const size_t index = ch - L'0';

        if (index < tokens.size() && index > 0)
        {
            appendToStr.append(tokens[index]);
        }

        return true;
    }

    return false;
}

// Routine Description:
// - Checks the given character to see if it is a wildcard arg replacement macro
//   and replaces it with the entire argument string if there is a match
// Arguments:
// - ch - Character to test as a macro
// - appendToStr - Append the macro result here if it matched
// - fullArgString - All of the arguments as one big string.
// Return Value:
// - True if we found the macro and appended to the string.
// - False if the given character doesn't match this macro.
bool Alias::s_TryReplaceWildcardArgMacro(const wchar_t ch,
                                         std::wstring& appendToStr,
                                         const std::wstring fullArgString)
{
    if (L'*' == ch)
    {
        // Wildcard substitutes all arguments
        appendToStr.append(fullArgString);
        return true;
    }

    return false;
}

// Routine Description:
// - Checks the given character to see if it is an input redirection macro
//   and replaces it with the < redirector if there is a match
// Arguments:
// - ch - Character to test as a macro
// - appendToStr - Append the macro result here if it matched
// Return Value:
// - True if we found the macro and appended to the string.
// - False if the given character doesn't match this macro.
bool Alias::s_TryReplaceInputRedirMacro(const wchar_t ch,
                                        std::wstring& appendToStr)
{
    if (L'L' == towupper(ch))
    {
        // L (either case) replaces with input redirector <
        appendToStr.push_back(L'<');
        return true;
    }
    return false;
}

// Routine Description:
// - Checks the given character to see if it is an output redirection macro
//   and replaces it with the > redirector if there is a match
// Arguments:
// - ch - Character to test as a macro
// - appendToStr - Append the macro result here if it matched
// Return Value:
// - True if we found the macro and appended to the string.
// - False if the given character doesn't match this macro.
bool Alias::s_TryReplaceOutputRedirMacro(const wchar_t ch,
                                         std::wstring& appendToStr)
{
    if (L'G' == towupper(ch))
    {
        // G (either case) replaces with output redirector >
        appendToStr.push_back(L'>');
        return true;
    }
    return false;
}

// Routine Description:
// - Checks the given character to see if it is a pipe redirection macro
//   and replaces it with the | redirector if there is a match
// Arguments:
// - ch - Character to test as a macro
// - appendToStr - Append the macro result here if it matched
// Return Value:
// - True if we found the macro and appended to the string.
// - False if the given character doesn't match this macro.
bool Alias::s_TryReplacePipeRedirMacro(const wchar_t ch,
                                       std::wstring& appendToStr)
{
    if (L'B' == towupper(ch))
    {
        // B (either case) replaces with pipe operator |
        appendToStr.push_back(L'|');
        return true;
    }
    return false;
}

// Routine Description:
// - Checks the given character to see if it is a next command macro
//   and replaces it with CRLF if there is a match
// Arguments:
// - ch - Character to test as a macro
// - appendToStr - Append the macro result here if it matched
// - lineCount - Updates the rolling count of lines if we add a CRLF.
// Return Value:
// - True if we found the macro and appended to the string.
// - False if the given character doesn't match this macro.
bool Alias::s_TryReplaceNextCommandMacro(const wchar_t ch,
                                         std::wstring& appendToStr,
                                         size_t& lineCount)
{
    if (L'T' == towupper(ch))
    {
        // T (either case) inserts a CRLF to chain commands
        s_AppendCrLf(appendToStr, lineCount);
        return true;
    }
    return false;
}

// Routine Description:
// - Appends the system line feed (CRLF) to the given string
// Arguments:
// - appendToStr - Append the system line feed here 
// - lineCount - Updates the rolling count of lines if we add a CRLF.
void Alias::s_AppendCrLf(std::wstring& appendToStr,
                         size_t& lineCount)
{
    appendToStr.push_back(L'\r');
    appendToStr.push_back(L'\n');
    lineCount++;
}

// Routine Description:
// - Searches through the given string for macros and replaces them
//   with the matching action
// Arguments:
// - str - On input, the string to search. On output, the string is replaced.
// - tokens - The tokenized command line input. 0 is the alias, 1-N are arguments.
// - fullArgString - Shorthand to 1-N argument string in case of wildcard match.
// Return Value:
// - The number of commands in the final string (line feeds, CRLFs)
size_t Alias::s_ReplaceMacros(std::wstring& str,
                              const std::deque<std::wstring>& tokens,
                              const std::wstring& fullArgString)
{
    size_t lineCount = 0;
    std::wstring finalText;

    // The target text may contain substitution macros indicated by $. 
    // Walk through and substitute them as appropriate.
    for (auto ch = str.cbegin(); ch < str.cend(); ch++)
    {
        if (L'$' == *ch)
        {
            // Attempt to read ahead by one character.
            const auto chNext = ch + 1;

            if (chNext < str.cend())
            {
                auto isProcessed = s_TryReplaceNumberedArgMacro(*chNext, finalText, tokens);
                if (!isProcessed)
                {
                    isProcessed = s_TryReplaceWildcardArgMacro(*chNext, finalText, fullArgString);
                }
                if (!isProcessed)
                {
                    isProcessed = s_TryReplaceInputRedirMacro(*chNext, finalText);
                }
                if (!isProcessed)
                {
                    isProcessed = s_TryReplaceOutputRedirMacro(*chNext, finalText);
                }
                if (!isProcessed)
                {
                    isProcessed = s_TryReplacePipeRedirMacro(*chNext, finalText);
                }
                if (!isProcessed)
                {
                    isProcessed = s_TryReplaceNextCommandMacro(*chNext, finalText, lineCount);
                }
                if (!isProcessed)
                {
                    // If nothing matches, just push these two characters in.
                    finalText.push_back(*ch);
                    finalText.push_back(*chNext);
                }

                // Since we read ahead and used that character, 
                // advance the iterator one extra to compensate.
                ch++;
            }
            else
            {
                // If no read-ahead, just push this character and be done.
                finalText.push_back(*ch);
            }
        }
        else
        {
            // If it didn't match the macro specifier $, push the character.
            finalText.push_back(*ch);
        }
    }

    // We always terminate with a CRLF to symbolize end of command.
    s_AppendCrLf(finalText, lineCount);

    // Give back the final text and count.
    str.swap(finalText);
    return lineCount;
}

// Routine Description:
// - Takes the source text and searches it for an alias belonging to exe name's list.
// Arguments:
// - sourceText - The string to search for an alias
// - exeName - The name of the EXE that has aliases associated
// - lineCount - Number of lines worth of text processed.
// Return Value:
// - If we found a matching alias, this will be the processed data
//   and lineCount is updated to the new number of lines.
// - If we didn't match and process an alias, return an empty string.
std::wstring Alias::s_MatchAndCopyAlias(const std::wstring& sourceText,
                                        const std::wstring& exeName,
                                        size_t& lineCount)
{
    // Copy source text into a local for manipulation.
    std::wstring sourceCopy(sourceText);

    // Trim trailing \r\n off of sourceCopy if it has one.
    s_TrimTrailingCrLf(sourceCopy);

    // Check if we have an EXE in the list that matches the request first.
    auto exeIter = g_aliasData.find(exeName);
    if (exeIter == g_aliasData.end())
    {
        // We found no data for this exe. Give back an empty string.
        return std::wstring();
    }

    auto exeList = exeIter->second;
    if (exeList.size() == 0)
    {
        // If there's no match, give back an empty string.
        return std::wstring();
    }

    // Tokenize the text by spaces
    const auto tokens = s_Tokenize(sourceCopy);

    // If there are no tokens, return an empty string
    if (tokens.size() == 0)
    {
        return std::wstring();
    }

    // Find alias. If there isn't one, return an empty string
    const auto alias = tokens.front();
    const auto aliasIter = exeList.find(alias);
    if (aliasIter == exeList.end())
    {
        // We found no alias pair with this name. Give back an empty string.
        return std::wstring();
    }

    const auto target = aliasIter->second;
    if (target.size() == 0)
    {
        return std::wstring();
    }

    // Get the string of all parameters as a shorthand for $* later.
    const auto allParams = s_GetArgString(sourceCopy);

    // The final text will be the target but with macros replaced.
    std::wstring finalText(target);
    lineCount = s_ReplaceMacros(finalText, tokens, allParams); 

    return finalText;
}

// Routine Description:
// - This routine matches the input string with an alias and copies the alias to the input buffer.
// Arguments:
// - pwchSource - string to match
// - cbSource - length of pwchSource in bytes
// - pwchTarget - where to store matched string
// - cbTargetSize - on input, contains size of pwchTarget.  
// - pcbTargetWritten - On output, contains length of alias stored in pwchTarget.
// - pwchExe - Name of exe that command is associated with to find related aliases
// - cbExe - Length in bytes of exe name
// - LineCount - aliases can contain multiple commands.  $T is the command separator
// Return Value:
// - None. It will just maintain the source as the target if we can't match an alias.
void Alias::s_MatchAndCopyAliasLegacy(_In_reads_bytes_(cbSource) PWCHAR pwchSource,
                                      _In_ ULONG cbSource,
                                      _Out_writes_bytes_(*pcbTarget) PWCHAR pwchTarget,
                                      _In_ ULONG cbTargetSize,
                                      _Out_ PULONG pcbTargetWritten,
                                      _In_reads_bytes_(cbExe) PWCHAR pwchExe,
                                      _In_ USHORT cbExe,
                                      _Out_ PDWORD pcLines)
{
    try
    {
        THROW_HR_IF(E_POINTER, pwchExe == nullptr);
        std::wstring exeName(pwchExe, cbExe / sizeof(WCHAR));
        std::wstring sourceText(pwchSource, cbSource / sizeof(WCHAR));
        size_t lineCount = *pcLines;

        const auto targetText = s_MatchAndCopyAlias(sourceText, exeName, lineCount);

        // Only return data if the reply was non-empty (we had a match).
        if (!targetText.empty())
        {
            const auto cchTargetSize = cbTargetSize / sizeof(wchar_t);

            // If the target text will fit in the result buffer, fill out the results.
            if (targetText.size() <= cchTargetSize)
            {
                // Non-null terminated copy into memory space
                std::copy_n(targetText.data(), targetText.size(), pwchTarget);

                // Return bytes copied.
                *pcbTargetWritten = gsl::narrow<ULONG>(targetText.size() * sizeof(wchar_t));

                // Return lines info.
                *pcLines = gsl::narrow<DWORD>(lineCount);
            }
        }
    }
    catch (...)
    {
        LOG_HR(wil::ResultFromCaughtException());
    }
}

#ifdef UNIT_TESTING
bool Alias::s_TestAddAlias(std::wstring& exe,
                           std::wstring& alias,
                           std::wstring& target)
{
    try
    {
        g_aliasData[exe][alias] = target;
    }
    catch (...)
    {
        LOG_HR(wil::ResultFromCaughtException());
        return false;
    }

    return true;
}

#endif
