/*++
Copyright (c) Microsoft Corporation

Module Name:
- ApiRoutines.h

Abstract:
- This file defines the interface to respond to all API calls.

Author:
- Michael Niksa (miniksa) 12-Oct-2016

Revision History:
- Adapted from original items in srvinit.cpp, getset.cpp, directio.cpp, stream.cpp
--*/

#pragma once

#include "..\server\IApiRoutines.h"

class ApiRoutines : public IApiRoutines
{
public:
#pragma region ObjectManagement
    /*HRESULT CreateInitialObjects(_Out_ InputBuffer** const ppInputObject,
    _Out_ SCREEN_INFORMATION** const ppOutputObject);
    */

#pragma endregion

#pragma region L1
    void GetConsoleInputCodePageImpl(_Out_ ULONG* const pCodePage) override;

    void GetConsoleOutputCodePageImpl(_Out_ ULONG* const pCodePage) override;

    void GetConsoleInputModeImpl(_In_ InputBuffer* const pContext,
                                 _Out_ ULONG* const pMode) override;

    void GetConsoleOutputModeImpl(_In_ const SCREEN_INFORMATION& Context,
                                  _Out_ ULONG* const pMode) override;

    [[nodiscard]]
    HRESULT SetConsoleInputModeImpl(_In_ InputBuffer* const pContext,
                                    const ULONG Mode) override;

    [[nodiscard]]
    HRESULT SetConsoleOutputModeImpl(_Inout_ SCREEN_INFORMATION& Context,
                                     const ULONG Mode) override;

    [[nodiscard]]
    HRESULT GetNumberOfConsoleInputEventsImpl(_In_ InputBuffer* const pContext,
                                              _Out_ ULONG* const pEvents) override;

    [[nodiscard]]
    HRESULT PeekConsoleInputAImpl(_In_ IConsoleInputObject* const pInContext,
                                  _Out_ std::deque<std::unique_ptr<IInputEvent>>& outEvents,
                                  const size_t eventsToRead,
                                  _In_ INPUT_READ_HANDLE_DATA* const pInputReadHandleData,
                                  _Outptr_result_maybenull_ IWaitRoutine** const ppWaiter) override;

    [[nodiscard]]
    HRESULT PeekConsoleInputWImpl(_In_ IConsoleInputObject* const pInContext,
                                  _Out_ std::deque<std::unique_ptr<IInputEvent>>& outEvents,
                                  const size_t eventsToRead,
                                  _In_ INPUT_READ_HANDLE_DATA* const pInputReadHandleData,
                                  _Outptr_result_maybenull_ IWaitRoutine** const ppWaiter) override;

    [[nodiscard]]
    HRESULT ReadConsoleInputAImpl(_In_ IConsoleInputObject* const pInContext,
                                  _Out_ std::deque<std::unique_ptr<IInputEvent>>& outEvents,
                                  const size_t eventsToRead,
                                  _In_ INPUT_READ_HANDLE_DATA* const pInputReadHandleData,
                                  _Outptr_result_maybenull_ IWaitRoutine** const ppWaiter) override;

    [[nodiscard]]
    HRESULT ReadConsoleInputWImpl(_In_ IConsoleInputObject* const pInContext,
                                  _Out_ std::deque<std::unique_ptr<IInputEvent>>& outEvents,
                                  const size_t eventsToRead,
                                  _In_ INPUT_READ_HANDLE_DATA* const pInputReadHandleData,
                                  _Outptr_result_maybenull_ IWaitRoutine** const ppWaiter) override;

    [[nodiscard]]
    HRESULT ReadConsoleAImpl(_Inout_ IConsoleInputObject* const pInContext,
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
                             _Out_ DWORD* const pdwControlKeyState) override;

    [[nodiscard]]
    HRESULT ReadConsoleWImpl(_Inout_ IConsoleInputObject* const pInContext,
                             _Out_writes_to_(cchTextBuffer, *pcchTextBufferWritten) wchar_t* const pwsTextBuffer,
                             const size_t cchTextBuffer,
                             _Out_ size_t* const pcchTextBufferWritten,
                             _Outptr_result_maybenull_ IWaitRoutine** const ppWaiter,
                             _In_reads_opt_(cchInitialData) const wchar_t* const pwsInitialData,
                             const size_t cchInitialData,
                             _In_reads_opt_(cchExeName) const wchar_t* const pwsExeName,
                             const size_t cchExeName,
                             _In_ INPUT_READ_HANDLE_DATA* const pHandleData,
                             const HANDLE hConsoleClient,
                             const DWORD dwControlWakeupMask,
                             _Out_ DWORD* const pdwControlKeyState) override;

    [[nodiscard]]
    HRESULT WriteConsoleAImpl(IConsoleOutputObject& OutContext,
                              _In_reads_(cchTextBufferLength) const char* const psTextBuffer,
                              const size_t cchTextBufferLength,
                              _Out_ size_t* const pcchTextBufferRead,
                              _Outptr_result_maybenull_ IWaitRoutine** const ppWaiter) override;

    [[nodiscard]]
    HRESULT WriteConsoleWImpl(IConsoleOutputObject& OutContext,
                              _In_reads_(cchTextBufferLength) const wchar_t* const pwsTextBuffer,
                              const size_t cchTextBufferLength,
                              _Out_ size_t* const pcchTextBufferRead,
                              _Outptr_result_maybenull_ IWaitRoutine** const ppWaiter) override;

#pragma region ThreadCreationInfo
    [[nodiscard]]
    HRESULT GetConsoleLangIdImpl(_Out_ LANGID* const pLangId) override;
#pragma endregion

#pragma endregion

#pragma region L2

    [[nodiscard]]
    virtual HRESULT FillConsoleOutputAttributeImpl(IConsoleOutputObject& OutContext,
                                                   const WORD attribute,
                                                   const size_t lengthToWrite,
                                                   const COORD startingCoordinate,
                                                   size_t& cellsModified) noexcept override;

    [[nodiscard]]
    virtual HRESULT FillConsoleOutputCharacterAImpl(IConsoleOutputObject& OutContext,
                                                    const char character,
                                                    const size_t lengthToWrite,
                                                    const COORD startingCoordinate,
                                                    size_t& cellsModified) noexcept override;

    [[nodiscard]]
    virtual HRESULT FillConsoleOutputCharacterWImpl(IConsoleOutputObject& OutContext,
                                                    const wchar_t character,
                                                    const size_t lengthToWrite,
                                                    const COORD startingCoordinate,
                                                    size_t& cellsModified) noexcept override;

    //// Process based. Restrict in protocol side?
    //HRESULT GenerateConsoleCtrlEventImpl(const ULONG ProcessGroupFilter,
    //                                             const ULONG ControlEvent);

    void SetConsoleActiveScreenBufferImpl(SCREEN_INFORMATION& NewContext) override;

    void FlushConsoleInputBuffer(_In_ InputBuffer* const pContext) override;

    [[nodiscard]]
    HRESULT SetConsoleInputCodePageImpl(const ULONG CodePage) override;

    [[nodiscard]]
    HRESULT SetConsoleOutputCodePageImpl(const ULONG CodePage) override;

    void GetConsoleCursorInfoImpl(_In_ const SCREEN_INFORMATION& Context,
                                  _Out_ ULONG* const pCursorSize,
                                  _Out_ bool* const pIsVisible) override;

    [[nodiscard]]
    HRESULT SetConsoleCursorInfoImpl(SCREEN_INFORMATION& Context,
                                     const ULONG CursorSize,
                                     const bool IsVisible) override;

    //// driver will pare down for non-Ex method
    void GetConsoleScreenBufferInfoExImpl(const SCREEN_INFORMATION& Context,
                                          _Out_ CONSOLE_SCREEN_BUFFER_INFOEX* const pScreenBufferInfoEx) override;

    [[nodiscard]]
    HRESULT SetConsoleScreenBufferInfoExImpl(SCREEN_INFORMATION& Context,
                                             const CONSOLE_SCREEN_BUFFER_INFOEX* const pScreenBufferInfoEx) override;

    [[nodiscard]]
    HRESULT SetConsoleScreenBufferSizeImpl(SCREEN_INFORMATION& Context,
                                           const COORD* const pSize) override;

    [[nodiscard]]
    HRESULT SetConsoleCursorPositionImpl(SCREEN_INFORMATION& Context,
                                         const COORD* const pCursorPosition) override;

    void GetLargestConsoleWindowSizeImpl(const SCREEN_INFORMATION& Context,
                                         _Out_ COORD* const pSize) override;

    [[nodiscard]]
    HRESULT ScrollConsoleScreenBufferAImpl(SCREEN_INFORMATION& Context,
                                           const SMALL_RECT* const pSourceRectangle,
                                           const COORD* const pTargetOrigin,
                                           _In_opt_ const SMALL_RECT* const pTargetClipRectangle,
                                           const char chFill,
                                           const WORD attrFill) override;

    [[nodiscard]]
    HRESULT ScrollConsoleScreenBufferWImpl(SCREEN_INFORMATION& Context,
                                           const SMALL_RECT* const pSourceRectangle,
                                           const COORD* const pTargetOrigin,
                                           _In_opt_ const SMALL_RECT* const pTargetClipRectangle,
                                           const wchar_t wchFill,
                                           const WORD attrFill) override;

    [[nodiscard]]
    HRESULT SetConsoleTextAttributeImpl(SCREEN_INFORMATION& Context,
                                        const WORD Attribute) override;

    [[nodiscard]]
    HRESULT SetConsoleWindowInfoImpl(SCREEN_INFORMATION& Context,
                                     const bool IsAbsoluteRectangle,
                                     const SMALL_RECT* const pWindowRectangle) override;

    //HRESULT ReadConsoleOutputAttributeImpl(_In_ SCREEN_INFORMATION* const pContext,
    //                                               const COORD* const pSourceOrigin,
    //                                               _Out_writes_to_(AttributeBufferLength, *pAttributeBufferWritten) WORD* const pAttributeBuffer,
    //                                               const ULONG AttributeBufferLength,
    //                                               _Out_ ULONG* const pAttributeBufferWritten);

    //HRESULT ReadConsoleOutputCharacterAImpl(_In_ SCREEN_INFORMATION* const pContext,
    //                                                const COORD* const pSourceOrigin,
    //                                                _Out_writes_to_(TextBufferLength, *pTextBufferWritten) char* const pTextBuffer,
    //                                                const ULONG pTextBufferLength,
    //                                                _Out_ ULONG* const pTextBufferWritten);

    //HRESULT ReadConsoleOutputCharacterWImpl(_In_ SCREEN_INFORMATION* const pContext,
    //                                                const COORD* const pSourceOrigin,
    //                                                _Out_writes_to_(TextBufferLength, *pTextBufferWritten) wchar_t* const pTextBuffer,
    //                                                const ULONG TextBufferLength,
    //                                                _Out_ ULONG* const pTextBufferWritten);

    //HRESULT WriteConsoleInputAImpl(_In_ InputBuffer* const pContext,
    //                                       _In_reads_(InputBufferLength) const INPUT_RECORD* const pInputBuffer,
    //                                       const ULONG InputBufferLength,
    //                                       _Out_ ULONG* const pInputBufferRead);

    //HRESULT WriteConsoleInputWImpl(_In_ InputBuffer* const pContext,
    //                                       _In_reads_(InputBufferLength) const INPUT_RECORD* const pInputBuffer,
    //                                       const ULONG InputBufferLength,
    //                                       _Out_ ULONG* const pInputBufferRead);

    //HRESULT WriteConsoleOutputAImpl(_In_ SCREEN_INFORMATION* const pContext,
    //                                        _In_reads_(pTextBufferSize->X * pTextBufferSize->Y) const CHAR_INFO* const pTextBuffer,
    //                                        const COORD* const pTextBufferSize,
    //                                        const COORD* const pTextBufferSourceOrigin,
    //                                        const SMALL_RECT* const pTargetRectangle,
    //                                        _Out_ SMALL_RECT* const pAffectedRectangle);

    //HRESULT WriteConsoleOutputWImpl(_In_ SCREEN_INFORMATION* const pContext,
    //                                        _In_reads_(pTextBufferSize->X * pTextBufferSize->Y) const CHAR_INFO* const pTextBuffer,
    //                                        const COORD* const pTextBufferSize,
    //                                        const COORD* const pTextBufferSourceOrigin,
    //                                        const SMALL_RECT* const pTargetRectangle,
    //                                        _Out_ SMALL_RECT* const pAffectedRectangle);

    [[nodiscard]]
    virtual HRESULT WriteConsoleOutputAttributeImpl(IConsoleOutputObject& OutContext,
                                                    const std::basic_string_view<WORD> attrs,
                                                    const COORD target,
                                                    size_t& used) noexcept override;

    [[nodiscard]]
    virtual HRESULT WriteConsoleOutputCharacterAImpl(IConsoleOutputObject& OutContext,
                                                     const std::string_view text,
                                                     const COORD target,
                                                     size_t& used) noexcept override;

    [[nodiscard]]
    virtual HRESULT WriteConsoleOutputCharacterWImpl(IConsoleOutputObject& OutContext,
                                                     const std::wstring_view text,
                                                     const COORD target,
                                                     size_t& used) noexcept override;

    //HRESULT ReadConsoleOutputA(_In_ SCREEN_INFORMATION* const pContext,
    //                                   _Out_writes_(pTextBufferSize->X * pTextBufferSize->Y) CHAR_INFO* const pTextBuffer,
    //                                   const COORD* const pTextBufferSize,
    //                                   const COORD* const pTextBufferTargetOrigin,
    //                                   const SMALL_RECT* const pSourceRectangle,
    //                                   _Out_ SMALL_RECT* const pReadRectangle);

    //HRESULT ReadConsoleOutputW(_In_ SCREEN_INFORMATION* const pContext,
    //                                   _Out_writes_(pTextBufferSize->X * pTextBufferSize->Y) CHAR_INFO* const pTextBuffer,
    //                                   const COORD* const pTextBufferSize,
    //                                   const COORD* const pTextBufferTargetOrigin,
    //                                   const SMALL_RECT* const pSourceRectangle,
    //                                   _Out_ SMALL_RECT* const pReadRectangle);

    [[nodiscard]]
    HRESULT GetConsoleTitleAImpl(_Out_writes_to_(cchTitleBufferSize, *pcchTitleBufferWritten) _Always_(_Post_z_) char* const psTitleBuffer,
                                 const size_t cchTitleBufferSize,
                                 _Out_ size_t* const pcchTitleBufferWritten,
                                 _Out_ size_t* const pcchTitleBufferNeeded) override;

    void GetConsoleTitleWImpl(_Out_writes_to_(cchTitleBufferSize, *pcchTitleBufferWritten) wchar_t* const pwsTitleBuffer,
                              const size_t cchTitleBufferSize,
                              _Out_ size_t* const pcchTitleBufferWritten,
                              _Out_ size_t* const pcchTitleBufferNeeded) override;

    [[nodiscard]]
    HRESULT GetConsoleOriginalTitleAImpl(_Out_writes_to_(cchTitleBufferSize, *pcchTitleBufferWritten) _Always_(_Post_z_) char* const psTitleBuffer,
                                         const size_t cchTitleBufferSize,
                                         _Out_ size_t* const pcchTitleBufferWritten,
                                         _Out_ size_t* const pcchTitleBufferNeeded) override;

    void GetConsoleOriginalTitleWImpl(_Out_writes_to_(cchTitleBufferSize, *pcchTitleBufferWritten) wchar_t* const pwsTitleBuffer,
                                      const size_t cchTitleBufferSize,
                                      _Out_ size_t* const pcchTitleBufferWritten,
                                      _Out_ size_t* const pcchTitleBufferNeeded) override;

    [[nodiscard]]
    HRESULT SetConsoleTitleAImpl(const std::string_view title) noexcept override;

    [[nodiscard]]
    HRESULT SetConsoleTitleWImpl(const std::wstring_view title) noexcept override;

#pragma endregion

#pragma region L3
    void GetNumberOfConsoleMouseButtonsImpl(_Out_ ULONG* const pButtons) override;

    [[nodiscard]]
    HRESULT GetConsoleFontSizeImpl(const SCREEN_INFORMATION& Context,
                                   const DWORD FontIndex,
                                   _Out_ COORD* const pFontSize) override;

    //// driver will pare down for non-Ex method
    [[nodiscard]]
    HRESULT GetCurrentConsoleFontExImpl(const SCREEN_INFORMATION& Context,
                                        const bool IsForMaximumWindowSize,
                                        _Out_ CONSOLE_FONT_INFOEX* const pConsoleFontInfoEx) override;

    [[nodiscard]]
    HRESULT SetConsoleDisplayModeImpl(SCREEN_INFORMATION& Context,
                                      const ULONG Flags,
                                      _Out_ COORD* const pNewScreenBufferSize) override;

    void GetConsoleDisplayModeImpl(_Out_ ULONG* const pFlags) override;

    [[nodiscard]]
    virtual HRESULT AddConsoleAliasAImpl(const std::string_view source,
                                         const std::string_view target,
                                         const std::string_view exeName) noexcept override;

    [[nodiscard]]
    virtual HRESULT AddConsoleAliasWImpl(const std::wstring_view source,
                                         const std::wstring_view target,
                                         const std::wstring_view exeName) noexcept override;

    [[nodiscard]]
    virtual HRESULT GetConsoleAliasAImpl(const std::string_view source,
                                         gsl::span<char> target,
                                         size_t& written,
                                         const std::string_view exeName) noexcept override;

    [[nodiscard]]
    virtual HRESULT GetConsoleAliasWImpl(const std::wstring_view source,
                                         gsl::span<wchar_t> target,
                                         size_t& written,
                                         const std::wstring_view exeName) noexcept override;

    [[nodiscard]]
    virtual HRESULT GetConsoleAliasesLengthAImpl(const std::string_view exeName,
                                                 size_t& bufferRequired) noexcept override;

    [[nodiscard]]
    virtual HRESULT GetConsoleAliasesLengthWImpl(const std::wstring_view exeName,
                                                 size_t& bufferRequired) noexcept override;

    [[nodiscard]]
    virtual HRESULT GetConsoleAliasExesLengthAImpl(size_t& bufferRequired) noexcept override;

    [[nodiscard]]
    virtual HRESULT GetConsoleAliasExesLengthWImpl(size_t& bufferRequired) noexcept override;

    [[nodiscard]]
    virtual HRESULT GetConsoleAliasesAImpl(const std::string_view exeName,
                                           gsl::span<char> alias,
                                           size_t& written) noexcept override;

    [[nodiscard]]
    virtual HRESULT GetConsoleAliasesWImpl(const std::wstring_view exeName,
                                           gsl::span<wchar_t> alias,
                                           size_t& written) noexcept override;

    [[nodiscard]]
    virtual HRESULT GetConsoleAliasExesAImpl(gsl::span<char> aliasExes,
                                             size_t& written) noexcept override;

    [[nodiscard]]
    virtual HRESULT GetConsoleAliasExesWImpl(gsl::span<wchar_t> aliasExes,
                                             size_t& written) noexcept override;

#pragma region CMDext Private API

    [[nodiscard]]
    virtual HRESULT ExpungeConsoleCommandHistoryAImpl(const std::string_view exeName) noexcept override;

    [[nodiscard]]
    virtual HRESULT ExpungeConsoleCommandHistoryWImpl(const std::wstring_view exeName) noexcept override;

    [[nodiscard]]
    virtual HRESULT SetConsoleNumberOfCommandsAImpl(const std::string_view exeName,
                                                    const size_t numberOfCommands) noexcept override;

    [[nodiscard]]
    virtual HRESULT SetConsoleNumberOfCommandsWImpl(const std::wstring_view exeName,
                                                    const size_t numberOfCommands) noexcept override;

    [[nodiscard]]
    virtual HRESULT GetConsoleCommandHistoryLengthAImpl(const std::string_view exeName,
                                                        size_t& length) noexcept override;

    [[nodiscard]]
    virtual HRESULT GetConsoleCommandHistoryLengthWImpl(const std::wstring_view exeName,
                                                        size_t& length) noexcept override;

    [[nodiscard]]
    virtual HRESULT GetConsoleCommandHistoryAImpl(const std::string_view exeName,
                                                  gsl::span<char> commandHistory,
                                                  size_t& written) noexcept override;

    [[nodiscard]]
    virtual HRESULT GetConsoleCommandHistoryWImpl(const std::wstring_view exeName,
                                                  gsl::span<wchar_t> commandHistory,
                                                  size_t& written) noexcept override;

#pragma endregion

    void GetConsoleWindowImpl(_Out_ HWND* const pHwnd) override;

    void GetConsoleSelectionInfoImpl(_Out_ CONSOLE_SELECTION_INFO* const pConsoleSelectionInfo) override;

    void GetConsoleHistoryInfoImpl(_Out_ CONSOLE_HISTORY_INFO* const pConsoleHistoryInfo) override;

    [[nodiscard]]
    HRESULT SetConsoleHistoryInfoImpl(const CONSOLE_HISTORY_INFO* const pConsoleHistoryInfo) override;

    [[nodiscard]]
    HRESULT SetCurrentConsoleFontExImpl(SCREEN_INFORMATION& Context,
                                        const bool IsForMaximumWindowSize,
                                        const CONSOLE_FONT_INFOEX* const pConsoleFontInfoEx) override;

#pragma endregion
};
