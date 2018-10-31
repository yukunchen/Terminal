/*++
Copyright (c) Microsoft Corporation

Module Name:
- IApiRoutines.h

Abstract:
- This file specifies the interface that must be defined by a server application to respond to all API calls.

Author:
- Michael Niksa (miniksa) 12-Oct-2016

Revision History:
- Adapted from original items in srvinit.cpp, getset.cpp, directio.cpp, stream.cpp
--*/

#pragma once

// TODO: 9115192 - Temporarily forward declare the real objects until I create an interface representing a console object
// This will be required so the server doesn't actually need to understand the implementation of a console object, just the few methods it needs to call.
class SCREEN_INFORMATION;
typedef SCREEN_INFORMATION IConsoleOutputObject;

class InputBuffer;
typedef InputBuffer IConsoleInputObject;

class INPUT_READ_HANDLE_DATA;

#include "IWaitRoutine.h"
#include <deque>
#include <memory>
#include "../types/inc/IInputEvent.hpp"

class IApiRoutines
{
public:

#pragma region ObjectManagement
    // TODO: 9115192 - We will need to make the objects via an interface eventually. This represents that idea.
    /*virtual HRESULT CreateInitialObjects(_Out_ IConsoleInputObject** const ppInputObject,
                                          _Out_ IConsoleOutputObject** const ppOutputObject);
*/

#pragma endregion

#pragma region L1
    virtual void GetConsoleInputCodePageImpl(_Out_ ULONG* const pCodePage) = 0;

    virtual void GetConsoleOutputCodePageImpl(_Out_ ULONG* const pCodePage) = 0;

    virtual void GetConsoleInputModeImpl(_In_ IConsoleInputObject* const pInContext,
                                         _Out_ ULONG* const pMode) = 0;

    virtual void GetConsoleOutputModeImpl(const IConsoleOutputObject& OutContext,
                                          _Out_ ULONG* const pMode) = 0;

    [[nodiscard]]
    virtual HRESULT SetConsoleInputModeImpl(_In_ IConsoleInputObject* const pInContext,
                                            const ULONG Mode) = 0;

    [[nodiscard]]
    virtual HRESULT SetConsoleOutputModeImpl(IConsoleOutputObject& OutContext,
                                             const ULONG Mode) = 0;

    [[nodiscard]]
    virtual HRESULT GetNumberOfConsoleInputEventsImpl(_In_ IConsoleInputObject* const pInContext,
                                                      _Out_ ULONG* const pEvents) = 0;

    [[nodiscard]]
    virtual HRESULT PeekConsoleInputAImpl(_In_ IConsoleInputObject* const pInContext,
                                          _Out_ std::deque<std::unique_ptr<IInputEvent>>& outEvents,
                                          const size_t eventsToRead,
                                          _In_ INPUT_READ_HANDLE_DATA* const pInputReadHandleData,
                                          _Outptr_result_maybenull_ IWaitRoutine** const ppWaiter) = 0;

    [[nodiscard]]
    virtual HRESULT PeekConsoleInputWImpl(_In_ IConsoleInputObject* const pInContext,
                                          _Out_ std::deque<std::unique_ptr<IInputEvent>>& outEvents,
                                          const size_t eventsToRead,
                                          _In_ INPUT_READ_HANDLE_DATA* const pInputReadHandleData,
                                          _Outptr_result_maybenull_ IWaitRoutine** const ppWaiter) = 0;

    [[nodiscard]]
    virtual HRESULT ReadConsoleInputAImpl(_In_ IConsoleInputObject* const pInContext,
                                          _Out_ std::deque<std::unique_ptr<IInputEvent>>& outEvents,
                                          const size_t eventsToRead,
                                          _In_ INPUT_READ_HANDLE_DATA* const pInputReadHandleData,
                                          _Outptr_result_maybenull_ IWaitRoutine** const ppWaiter) = 0;

    [[nodiscard]]
    virtual HRESULT ReadConsoleInputWImpl(_In_ IConsoleInputObject* const pInContext,
                                          _Out_ std::deque<std::unique_ptr<IInputEvent>>& outEvents,
                                          const size_t eventsToRead,
                                          _In_ INPUT_READ_HANDLE_DATA* const pInputReadHandleData,
                                          _Outptr_result_maybenull_ IWaitRoutine** const ppWaiter) = 0;

    [[nodiscard]]
    virtual HRESULT ReadConsoleAImpl(_Inout_ IConsoleInputObject* const pInContext,
                                     _Out_writes_to_(cchTextBuffer, *pcchTextBufferWritten) char* const psTextBuffer,
                                     const size_t cchTextBuffer,
                                     _Out_ size_t* const pcchTextBufferWritten,
                                     _Outptr_result_maybenull_ IWaitRoutine** const ppWaiter,
                                     _In_reads_opt_(cchInitialData) const char* const psInitialData,
                                     const size_t cchInitialData,
                                     _In_reads_opt_(cchExeName) const wchar_t* const pwsExeName,
                                     const size_t cchExeName,
                                     _In_ INPUT_READ_HANDLE_DATA* const pHandleData,
                                     const HANDLE hConsoleClient,
                                     const DWORD dwControlWakeupMask,
                                     _Out_ DWORD* const pdwControlKeyState) = 0;

    [[nodiscard]]
    virtual HRESULT ReadConsoleWImpl(_Inout_ IConsoleInputObject* const pInContext,
                                     _Out_writes_to_(cchTextBufferLength, *pcchTextBufferWritten) wchar_t* const pwsTextBuffer,
                                     const size_t cchTextBufferLength,
                                     _Out_ size_t* const pcchTextBufferWritten,
                                     _Outptr_result_maybenull_ IWaitRoutine** const ppWaiter,
                                     _In_reads_opt_(cchInitialDataLength) const wchar_t* const pwsInitialData,
                                     const size_t cchInitialDataLength,
                                     _In_reads_opt_(cchExeName) const wchar_t* const pwsExeName,
                                     const size_t cchExeName,
                                     _In_ INPUT_READ_HANDLE_DATA* const pHandleData,
                                     const HANDLE hConsoleClient,
                                     const DWORD dwControlWakeupMask,
                                     _Out_ DWORD* const pdwControlKeyState) = 0;

    [[nodiscard]]
    virtual HRESULT WriteConsoleAImpl(IConsoleOutputObject& OutContext,
                                      _In_reads_(cchTextBufferLength) const char* const psTextBuffer,
                                      const size_t cchTextBufferLength,
                                      _Out_ size_t* const pcchTextBufferRead,
                                      _Outptr_result_maybenull_ IWaitRoutine** const ppWaiter) = 0;

    [[nodiscard]]
    virtual HRESULT WriteConsoleWImpl(IConsoleOutputObject& OutContext,
                                      _In_reads_(cchTextBufferLength) const wchar_t* const pwsTextBuffer,
                                      const size_t cchTextBufferLength,
                                      _Out_ size_t* const pcchTextBufferRead,
                                      _Outptr_result_maybenull_ IWaitRoutine** const ppWaiter) = 0;

#pragma region Thread Creation Info
    [[nodiscard]]
    virtual HRESULT GetConsoleLangIdImpl(_Out_ LANGID* const pLangId) = 0;
#pragma endregion

#pragma endregion

#pragma region L2

    [[nodiscard]]
    virtual HRESULT FillConsoleOutputAttributeImpl(IConsoleOutputObject& OutContext,
                                                   const WORD attribute,
                                                   const size_t lengthToWrite,
                                                   const COORD startingCoordinate,
                                                   size_t& cellsModified) noexcept = 0;

    [[nodiscard]]
    virtual HRESULT FillConsoleOutputCharacterAImpl(IConsoleOutputObject& OutContext,
                                                    const char character,
                                                    const size_t lengthToWrite,
                                                    const COORD startingCoordinate,
                                                    size_t& cellsModified) noexcept = 0;

    [[nodiscard]]
    virtual HRESULT FillConsoleOutputCharacterWImpl(IConsoleOutputObject& OutContext,
                                                    const wchar_t character,
                                                    const size_t lengthToWrite,
                                                    const COORD startingCoordinate,
                                                    size_t& cellsModified) noexcept = 0;

    virtual void SetConsoleActiveScreenBufferImpl(IConsoleOutputObject& NewOutContext) = 0;

    virtual void FlushConsoleInputBuffer(_In_ IConsoleInputObject* const pInContext) = 0;

    [[nodiscard]]
    virtual HRESULT SetConsoleInputCodePageImpl(const ULONG CodePage) = 0;

    [[nodiscard]]
    virtual HRESULT SetConsoleOutputCodePageImpl(const ULONG CodePage) = 0;

    virtual void GetConsoleCursorInfoImpl(const IConsoleOutputObject& OutContext,
                                          _Out_ ULONG* const pCursorSize,
                                          _Out_ bool* const pIsVisible) = 0;

    [[nodiscard]]
    virtual HRESULT SetConsoleCursorInfoImpl(IConsoleOutputObject& OutContext,
                                             const ULONG CursorSize,
                                             const bool IsVisible) = 0;

    // driver will pare down for non-Ex method
    virtual void GetConsoleScreenBufferInfoExImpl(const IConsoleOutputObject& OutContext,
                                                  _Out_ CONSOLE_SCREEN_BUFFER_INFOEX* const pScreenBufferInfoEx) = 0;

    [[nodiscard]]
    virtual HRESULT SetConsoleScreenBufferInfoExImpl(IConsoleOutputObject& OutContext,
                                                     const CONSOLE_SCREEN_BUFFER_INFOEX* const pScreenBufferInfoEx) = 0;

    [[nodiscard]]
    virtual HRESULT SetConsoleScreenBufferSizeImpl(IConsoleOutputObject& OutContext,
                                                   const COORD* const pSize) = 0;

    [[nodiscard]]
    virtual HRESULT SetConsoleCursorPositionImpl(IConsoleOutputObject& OutContext,
                                                 const COORD* const pCursorPosition) = 0;

    virtual void GetLargestConsoleWindowSizeImpl(const IConsoleOutputObject& OutContext,
                                                 _Out_ COORD* const pSize) = 0;

    [[nodiscard]]
    virtual HRESULT ScrollConsoleScreenBufferAImpl(IConsoleOutputObject& OutContext,
                                                   const SMALL_RECT* const pSourceRectangle,
                                                   const COORD* const pTargetOrigin,
                                                   _In_opt_ const SMALL_RECT* const pTargetClipRectangle,
                                                   const char chFill,
                                                   const WORD attrFill) = 0;

    [[nodiscard]]
    virtual HRESULT ScrollConsoleScreenBufferWImpl(IConsoleOutputObject& OutContext,
                                                   const SMALL_RECT* const pSourceRectangle,
                                                   const COORD* const pTargetOrigin,
                                                   _In_opt_ const SMALL_RECT* const pTargetClipRectangle,
                                                   const wchar_t wchFill,
                                                   const WORD attrFill) = 0;

    [[nodiscard]]
    virtual HRESULT SetConsoleTextAttributeImpl(IConsoleOutputObject& OutContext,
                                                const WORD Attribute) = 0;

    [[nodiscard]]
    virtual HRESULT SetConsoleWindowInfoImpl(IConsoleOutputObject& OutContext,
                                             const bool IsAbsoluteRectangle,
                                             const SMALL_RECT* const pWindowRectangle) = 0;

    [[nodiscard]]
    virtual HRESULT ReadConsoleOutputAttributeImpl(const IConsoleOutputObject& OutContext,
                                                   const COORD* const pSourceOrigin,
                                                   _Out_writes_to_(AttributeBufferLength, *pAttributeBufferWritten) WORD* const pAttributeBuffer,
                                                   const ULONG AttributeBufferLength,
                                                   _Out_ ULONG* const pAttributeBufferWritten);

    [[nodiscard]]
    virtual HRESULT ReadConsoleOutputCharacterAImpl(const IConsoleOutputObject& OutContext,
                                                    const COORD* const pSourceOrigin,
                                                    _Out_writes_to_(TextBufferLength, *pTextBufferWritten) char* const pTextBuffer,
                                                    const ULONG TextBufferLength,
                                                    _Out_ ULONG* const pTextBufferWritten);

    [[nodiscard]]
    virtual HRESULT ReadConsoleOutputCharacterWImpl(const IConsoleOutputObject& OutContext,
                                                    const COORD* const pSourceOrigin,
                                                    _Out_writes_to_(TextBufferLength, *pTextBufferWritten) wchar_t* const pTextBuffer,
                                                    const ULONG TextBufferLength,
                                                    _Out_ ULONG* const pTextBufferWritten);

    [[nodiscard]]
    virtual HRESULT WriteConsoleInputAImpl(_In_ IConsoleInputObject* const pInContext,
                                           _In_reads_(InputBufferLength) const INPUT_RECORD* const pInputBuffer,
                                           const ULONG InputBufferLength,
                                           _Out_ ULONG* const pInputBufferRead);

    [[nodiscard]]
    virtual HRESULT WriteConsoleInputWImpl(_In_ IConsoleInputObject* const pInContext,
                                           _In_reads_(InputBufferLength) const INPUT_RECORD* const pInputBuffer,
                                           const ULONG InputBufferLength,
                                           _Out_ ULONG* const pInputBufferRead);

    [[nodiscard]]
    virtual HRESULT WriteConsoleOutputAImpl(IConsoleOutputObject& OutContext,
                                            _In_reads_(pTextBufferSize->X * pTextBufferSize->Y) const CHAR_INFO* const pTextBuffer,
                                            const COORD* const pTextBufferSize,
                                            const COORD* const pTextBufferSourceOrigin,
                                            const SMALL_RECT* const pTargetRectangle,
                                            _Out_ SMALL_RECT* const pAffectedRectangle);

    [[nodiscard]]
    virtual HRESULT WriteConsoleOutputWImpl(IConsoleOutputObject&  OutContext,
                                            _In_reads_(pTextBufferSize->X * pTextBufferSize->Y) const CHAR_INFO* const pTextBuffer,
                                            const COORD* const pTextBufferSize,
                                            const COORD* const pTextBufferSourceOrigin,
                                            const SMALL_RECT* const pTargetRectangle,
                                            _Out_ SMALL_RECT* const pAffectedRectangle);

    [[nodiscard]]
    virtual HRESULT WriteConsoleOutputAttributeImpl(IConsoleOutputObject& OutContext,
                                                    const std::basic_string_view<WORD> attrs,
                                                    const COORD target,
                                                    size_t& used) noexcept = 0;

    [[nodiscard]]
    virtual HRESULT WriteConsoleOutputCharacterAImpl(IConsoleOutputObject& OutContext,
                                                     const std::string_view text,
                                                     const COORD target,
                                                     size_t& used) noexcept = 0;

    [[nodiscard]]
    virtual HRESULT WriteConsoleOutputCharacterWImpl(IConsoleOutputObject& OutContext,
                                                     const std::wstring_view text,
                                                     const COORD target,
                                                     size_t& used) noexcept = 0;

    [[nodiscard]]
    virtual HRESULT ReadConsoleOutputA(const IConsoleOutputObject& OutContext,
                                       _Out_writes_(pTextBufferSize->X * pTextBufferSize->Y) CHAR_INFO* const pTextBuffer,
                                       const COORD* const pTextBufferSize,
                                       const COORD* const pTextBufferTargetOrigin,
                                       const SMALL_RECT* const pSourceRectangle,
                                       _Out_ SMALL_RECT* const pReadRectangle);

    [[nodiscard]]
    virtual HRESULT ReadConsoleOutputW(const IConsoleOutputObject& OutContext,
                                       _Out_writes_(pTextBufferSize->X * pTextBufferSize->Y) CHAR_INFO* const pTextBuffer,
                                       const COORD* const pTextBufferSize,
                                       const COORD* const pTextBufferTargetOrigin,
                                       const SMALL_RECT* const pSourceRectangle,
                                       _Out_ SMALL_RECT* const pReadRectangle);

    [[nodiscard]]
    virtual HRESULT GetConsoleTitleAImpl(_Out_writes_to_(cchTitleBufferSize, *pcchTitleBufferWritten) _Always_(_Post_z_) char* const psTitleBuffer,
                                         const size_t cchTitleBufferSize,
                                         _Out_ size_t* const pcchTitleBufferWritten,
                                         _Out_ size_t* const pcchTitleBufferNeeded) = 0;

    virtual void GetConsoleTitleWImpl(_Out_writes_to_(cchTitleBufferSize, *pcchTitleBufferWritten) wchar_t* const pwsTitleBuffer,
                                      const size_t cchTitleBufferSize,
                                      _Out_ size_t* const pcchTitleBufferWritten,
                                      _Out_ size_t* const pcchTitleBufferNeeded) = 0;

    [[nodiscard]]
    virtual HRESULT GetConsoleOriginalTitleAImpl(_Out_writes_to_(cchTitleBufferSize, *pcchTitleBufferWritten) _Always_(_Post_z_) char* const psTitleBuffer,
                                                 const size_t cchTitleBufferSize,
                                                 _Out_ size_t* const pcchTitleBufferWritten,
                                                 _Out_ size_t* const pcchTitleBufferNeeded) = 0;

    virtual void GetConsoleOriginalTitleWImpl(_Out_writes_to_(cchTitleBufferSize, *pcchTitleBufferWritten) wchar_t* const pwsTitleBuffer,
                                              const size_t cchTitleBufferSize,
                                              _Out_ size_t* const pcchTitleBufferWritten,
                                              _Out_ size_t* const pcchTitleBufferNeeded) = 0;

    [[nodiscard]]
    virtual HRESULT SetConsoleTitleAImpl(const std::string_view title) noexcept = 0;

    [[nodiscard]]
    virtual HRESULT SetConsoleTitleWImpl(const std::wstring_view title) noexcept = 0;

#pragma endregion

#pragma region L3
    virtual void GetNumberOfConsoleMouseButtonsImpl(_Out_ ULONG* const pButtons) = 0;

    [[nodiscard]]
    virtual HRESULT GetConsoleFontSizeImpl(const IConsoleOutputObject& OutContext,
                                           const DWORD FontIndex,
                                           _Out_ COORD* const pFontSize) = 0;

    // driver will pare down for non-Ex method
    [[nodiscard]]
    virtual HRESULT GetCurrentConsoleFontExImpl(const IConsoleOutputObject& OutContext,
                                                const bool IsForMaximumWindowSize,
                                                _Out_ CONSOLE_FONT_INFOEX* const pConsoleFontInfoEx) = 0;

    [[nodiscard]]
    virtual HRESULT SetConsoleDisplayModeImpl(SCREEN_INFORMATION& Context,
                                              const ULONG Flags,
                                              _Out_ COORD* const pNewScreenBufferSize) = 0;

    virtual void GetConsoleDisplayModeImpl(_Out_ ULONG* const pFlags) = 0;

    [[nodiscard]]
    virtual HRESULT AddConsoleAliasAImpl(const std::string_view source,
                                         const std::string_view target,
                                         const std::string_view exeName) noexcept = 0;

    [[nodiscard]]
    virtual HRESULT AddConsoleAliasWImpl(const std::wstring_view source,
                                         const std::wstring_view target,
                                         const std::wstring_view exeName) noexcept = 0;

    [[nodiscard]]
    virtual HRESULT GetConsoleAliasAImpl(const std::string_view source,
                                         gsl::span<char> target,
                                         size_t& written,
                                         const std::string_view exeName) noexcept = 0;

    [[nodiscard]]
    virtual HRESULT GetConsoleAliasWImpl(const std::wstring_view source,
                                         gsl::span<wchar_t> target,
                                         size_t& written,
                                         const std::wstring_view exeName) noexcept = 0;

    [[nodiscard]]
    virtual HRESULT GetConsoleAliasesLengthAImpl(const std::string_view exeName,
                                                 size_t& bufferRequired) noexcept = 0;

    [[nodiscard]]
    virtual HRESULT GetConsoleAliasesLengthWImpl(const std::wstring_view exeName,
                                                 size_t& bufferRequired) noexcept = 0;

    [[nodiscard]]
    virtual HRESULT GetConsoleAliasExesLengthAImpl(size_t& bufferRequired) noexcept = 0;

    [[nodiscard]]
    virtual HRESULT GetConsoleAliasExesLengthWImpl(size_t& bufferRequired) noexcept = 0;

    [[nodiscard]]
    virtual HRESULT GetConsoleAliasesAImpl(const std::string_view exeName,
                                           gsl::span<char> alias,
                                           size_t& written) noexcept = 0;

    [[nodiscard]]
    virtual HRESULT GetConsoleAliasesWImpl(const std::wstring_view exeName,
                                           gsl::span<wchar_t> alias,
                                           size_t& written) noexcept = 0;

    [[nodiscard]]
    virtual HRESULT GetConsoleAliasExesAImpl(gsl::span<char> aliasExes,
                                             size_t& written) noexcept = 0;

    [[nodiscard]]
    virtual HRESULT GetConsoleAliasExesWImpl(gsl::span<wchar_t> aliasExes,
                                             size_t& written) noexcept = 0;

#pragma region CMDext Private API

    [[nodiscard]]
    virtual HRESULT ExpungeConsoleCommandHistoryAImpl(const std::string_view exeName) noexcept = 0;

    [[nodiscard]]
    virtual HRESULT ExpungeConsoleCommandHistoryWImpl(const std::wstring_view exeName) noexcept = 0;

    [[nodiscard]]
    virtual HRESULT SetConsoleNumberOfCommandsAImpl(const std::string_view exeName,
                                                    const size_t numberOfCommands) noexcept = 0;

    [[nodiscard]]
    virtual HRESULT SetConsoleNumberOfCommandsWImpl(const std::wstring_view exeName,
                                                    const size_t numberOfCommands) noexcept = 0;

    [[nodiscard]]
    virtual HRESULT GetConsoleCommandHistoryLengthAImpl(const std::string_view exeName,
                                                        size_t& length) noexcept = 0;

    [[nodiscard]]
    virtual HRESULT GetConsoleCommandHistoryLengthWImpl(const std::wstring_view exeName,
                                                        size_t& length) noexcept = 0;

    [[nodiscard]]
    virtual HRESULT GetConsoleCommandHistoryAImpl(const std::string_view exeName,
                                                  gsl::span<char> commandHistory,
                                                  size_t& written) noexcept = 0;

    [[nodiscard]]
    virtual HRESULT GetConsoleCommandHistoryWImpl(const std::wstring_view exeName,
                                                  gsl::span<wchar_t> commandHistory,
                                                  size_t& written) noexcept = 0;

#pragma endregion

    virtual void GetConsoleWindowImpl(_Out_ HWND* const pHwnd) = 0;

    virtual void GetConsoleSelectionInfoImpl(_Out_ CONSOLE_SELECTION_INFO* const pConsoleSelectionInfo) = 0;

    virtual void GetConsoleHistoryInfoImpl(_Out_ CONSOLE_HISTORY_INFO* const pConsoleHistoryInfo) = 0;

    [[nodiscard]]
    virtual HRESULT SetConsoleHistoryInfoImpl(const CONSOLE_HISTORY_INFO* const pConsoleHistoryInfo) = 0;

    [[nodiscard]]
    virtual HRESULT SetCurrentConsoleFontExImpl(IConsoleOutputObject& OutContext,
                                                const bool IsForMaximumWindowSize,
                                                const CONSOLE_FONT_INFOEX* const pConsoleFontInfoEx) = 0;

#pragma endregion
};
