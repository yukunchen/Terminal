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

    virtual void GetConsoleOutputModeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                          _Out_ ULONG* const pMode) = 0;

    [[nodiscard]]
    virtual HRESULT SetConsoleInputModeImpl(_In_ IConsoleInputObject* const pInContext,
                                            _In_ ULONG const Mode) = 0;

    [[nodiscard]]
    virtual HRESULT SetConsoleOutputModeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                             _In_ ULONG const Mode) = 0;

    [[nodiscard]]
    virtual HRESULT GetNumberOfConsoleInputEventsImpl(_In_ IConsoleInputObject* const pInContext,
                                                      _Out_ ULONG* const pEvents) = 0;

    [[nodiscard]]
    virtual HRESULT PeekConsoleInputAImpl(_In_ IConsoleInputObject* const pInContext,
                                          _Out_ std::deque<std::unique_ptr<IInputEvent>>& outEvents,
                                          _In_ size_t const eventsToRead,
                                          _In_ INPUT_READ_HANDLE_DATA* const pInputReadHandleData,
                                          _Outptr_result_maybenull_ IWaitRoutine** const ppWaiter) = 0;

    [[nodiscard]]
    virtual HRESULT PeekConsoleInputWImpl(_In_ IConsoleInputObject* const pInContext,
                                          _Out_ std::deque<std::unique_ptr<IInputEvent>>& outEvents,
                                          _In_ size_t const eventsToRead,
                                          _In_ INPUT_READ_HANDLE_DATA* const pInputReadHandleData,
                                          _Outptr_result_maybenull_ IWaitRoutine** const ppWaiter) = 0;

    [[nodiscard]]
    virtual HRESULT ReadConsoleInputAImpl(_In_ IConsoleInputObject* const pInContext,
                                          _Out_ std::deque<std::unique_ptr<IInputEvent>>& outEvents,
                                          _In_ size_t const eventsToRead,
                                          _In_ INPUT_READ_HANDLE_DATA* const pInputReadHandleData,
                                          _Outptr_result_maybenull_ IWaitRoutine** const ppWaiter) = 0;

    [[nodiscard]]
    virtual HRESULT ReadConsoleInputWImpl(_In_ IConsoleInputObject* const pInContext,
                                          _Out_ std::deque<std::unique_ptr<IInputEvent>>& outEvents,
                                          _In_ size_t const eventsToRead,
                                          _In_ INPUT_READ_HANDLE_DATA* const pInputReadHandleData,
                                          _Outptr_result_maybenull_ IWaitRoutine** const ppWaiter) = 0;

    [[nodiscard]]
    virtual HRESULT ReadConsoleAImpl(_Inout_ IConsoleInputObject* const pInContext,
                                     _Out_writes_to_(cchTextBuffer, *pcchTextBufferWritten) char* const psTextBuffer,
                                     _In_ size_t const cchTextBuffer,
                                     _Out_ size_t* const pcchTextBufferWritten,
                                     _Outptr_result_maybenull_ IWaitRoutine** const ppWaiter,
                                     _In_reads_opt_(cchInitialData) const char* const psInitialData,
                                     _In_ size_t const cchInitialData,
                                     _In_reads_opt_(cchExeName) const wchar_t* const pwsExeName,
                                     _In_ size_t const cchExeName,
                                     _In_ INPUT_READ_HANDLE_DATA* const pHandleData,
                                     _In_ HANDLE const hConsoleClient,
                                     _In_ DWORD const dwControlWakeupMask,
                                     _Out_ DWORD* const pdwControlKeyState) = 0;

    [[nodiscard]]
    virtual HRESULT ReadConsoleWImpl(_Inout_ IConsoleInputObject* const pInContext,
                                     _Out_writes_to_(cchTextBufferLength, *pcchTextBufferWritten) wchar_t* const pwsTextBuffer,
                                     _In_ size_t const cchTextBufferLength,
                                     _Out_ size_t* const pcchTextBufferWritten,
                                     _Outptr_result_maybenull_ IWaitRoutine** const ppWaiter,
                                     _In_reads_opt_(cchInitialDataLength) const wchar_t* const pwsInitialData,
                                     _In_ size_t const cchInitialDataLength,
                                     _In_reads_opt_(cchExeName) const wchar_t* const pwsExeName,
                                     _In_ size_t const cchExeName,
                                     _In_ INPUT_READ_HANDLE_DATA* const pHandleData,
                                     _In_ HANDLE const hConsoleClient,
                                     _In_ DWORD const dwControlWakeupMask,
                                     _Out_ DWORD* const pdwControlKeyState) = 0;

    [[nodiscard]]
    virtual HRESULT WriteConsoleAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                      _In_reads_(cchTextBufferLength) const char* const psTextBuffer,
                                      _In_ size_t const cchTextBufferLength,
                                      _Out_ size_t* const pcchTextBufferRead,
                                      _Outptr_result_maybenull_ IWaitRoutine** const ppWaiter) = 0;

    [[nodiscard]]
    virtual HRESULT WriteConsoleWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                      _In_reads_(cchTextBufferLength) const wchar_t* const pwsTextBuffer,
                                      _In_ size_t const cchTextBufferLength,
                                      _Out_ size_t* const pcchTextBufferRead,
                                      _Outptr_result_maybenull_ IWaitRoutine** const ppWaiter) = 0;

#pragma region Thread Creation Info
    [[nodiscard]]
    virtual HRESULT GetConsoleLangIdImpl(_Out_ LANGID* const pLangId) = 0;
#pragma endregion

#pragma endregion

#pragma region L2

    [[nodiscard]]
    virtual HRESULT FillConsoleOutputAttributeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                   _In_ WORD const Attribute,
                                                   _In_ DWORD const LengthToWrite,
                                                   _In_ COORD const StartingCoordinate,
                                                   _Out_ DWORD* const pCellsModified);

    [[nodiscard]]
    virtual HRESULT FillConsoleOutputCharacterAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                    _In_ char const Character,
                                                    _In_ DWORD const LengthToWrite,
                                                    _In_ COORD const StartingCoordinate,
                                                    _Out_ DWORD* const pCellsModified);

    [[nodiscard]]
    virtual HRESULT FillConsoleOutputCharacterWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                    _In_ wchar_t const Character,
                                                    _In_ DWORD const LengthToWrite,
                                                    _In_ COORD const StartingCoordinate,
                                                    _Out_ DWORD* const pCellsModified);

    virtual void SetConsoleActiveScreenBufferImpl(_In_ IConsoleOutputObject* const pNewOutContext) = 0;

    virtual void FlushConsoleInputBuffer(_In_ IConsoleInputObject* const pInContext) = 0;

    [[nodiscard]]
    virtual HRESULT SetConsoleInputCodePageImpl(_In_ ULONG const CodePage) = 0;

    [[nodiscard]]
    virtual HRESULT SetConsoleOutputCodePageImpl(_In_ ULONG const CodePage) = 0;

    virtual void GetConsoleCursorInfoImpl(_In_ IConsoleOutputObject* const pOutContext,
                                          _Out_ ULONG* const pCursorSize,
                                          _Out_ BOOLEAN* const pIsVisible) = 0;

    [[nodiscard]]
    virtual HRESULT SetConsoleCursorInfoImpl(_In_ IConsoleOutputObject* const pOutContext,
                                             _In_ ULONG const CursorSize,
                                             _In_ BOOLEAN const IsVisible) = 0;

    // driver will pare down for non-Ex method
    virtual void GetConsoleScreenBufferInfoExImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                  _Out_ CONSOLE_SCREEN_BUFFER_INFOEX* const pScreenBufferInfoEx) = 0;

    [[nodiscard]]
    virtual HRESULT SetConsoleScreenBufferInfoExImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                     _In_ const CONSOLE_SCREEN_BUFFER_INFOEX* const pScreenBufferInfoEx) = 0;

    [[nodiscard]]
    virtual HRESULT SetConsoleScreenBufferSizeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                   _In_ const COORD* const pSize) = 0;

    [[nodiscard]]
    virtual HRESULT SetConsoleCursorPositionImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                 _In_ const COORD* const pCursorPosition) = 0;

    virtual void GetLargestConsoleWindowSizeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                 _Out_ COORD* const pSize) = 0;

    [[nodiscard]]
    virtual HRESULT ScrollConsoleScreenBufferAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                   _In_ const SMALL_RECT* const pSourceRectangle,
                                                   _In_ const COORD* const pTargetOrigin,
                                                   _In_opt_ const SMALL_RECT* const pTargetClipRectangle,
                                                   _In_ char const chFill,
                                                   _In_ WORD const attrFill) = 0;

    [[nodiscard]]
    virtual HRESULT ScrollConsoleScreenBufferWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                   _In_ const SMALL_RECT* const pSourceRectangle,
                                                   _In_ const COORD* const pTargetOrigin,
                                                   _In_opt_ const SMALL_RECT* const pTargetClipRectangle,
                                                   _In_ wchar_t const wchFill,
                                                   _In_ WORD const attrFill) = 0;

    [[nodiscard]]
    virtual HRESULT SetConsoleTextAttributeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                _In_ WORD const Attribute) = 0;

    [[nodiscard]]
    virtual HRESULT SetConsoleWindowInfoImpl(_In_ IConsoleOutputObject* const pOutContext,
                                             _In_ BOOLEAN const IsAbsoluteRectangle,
                                             _In_ const SMALL_RECT* const pWindowRectangle) = 0;

    [[nodiscard]]
    virtual HRESULT ReadConsoleOutputAttributeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                   _In_ const COORD* const pSourceOrigin,
                                                   _Out_writes_to_(AttributeBufferLength, *pAttributeBufferWritten) WORD* const pAttributeBuffer,
                                                   _In_ ULONG const AttributeBufferLength,
                                                   _Out_ ULONG* const pAttributeBufferWritten);

    [[nodiscard]]
    virtual HRESULT ReadConsoleOutputCharacterAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                    _In_ const COORD* const pSourceOrigin,
                                                    _Out_writes_to_(TextBufferLength, *pTextBufferWritten) char* const pTextBuffer,
                                                    _In_ ULONG const TextBufferLength,
                                                    _Out_ ULONG* const pTextBufferWritten);

    [[nodiscard]]
    virtual HRESULT ReadConsoleOutputCharacterWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                    _In_ const COORD* const pSourceOrigin,
                                                    _Out_writes_to_(TextBufferLength, *pTextBufferWritten) wchar_t* const pTextBuffer,
                                                    _In_ ULONG const TextBufferLength,
                                                    _Out_ ULONG* const pTextBufferWritten);

    [[nodiscard]]
    virtual HRESULT WriteConsoleInputAImpl(_In_ IConsoleInputObject* const pInContext,
                                           _In_reads_(InputBufferLength) const INPUT_RECORD* const pInputBuffer,
                                           _In_ ULONG const InputBufferLength,
                                           _Out_ ULONG* const pInputBufferRead);

    [[nodiscard]]
    virtual HRESULT WriteConsoleInputWImpl(_In_ IConsoleInputObject* const pInContext,
                                           _In_reads_(InputBufferLength) const INPUT_RECORD* const pInputBuffer,
                                           _In_ ULONG const InputBufferLength,
                                           _Out_ ULONG* const pInputBufferRead);

    [[nodiscard]]
    virtual HRESULT WriteConsoleOutputAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                            _In_reads_(pTextBufferSize->X * pTextBufferSize->Y) const CHAR_INFO* const pTextBuffer,
                                            _In_ const COORD* const pTextBufferSize,
                                            _In_ const COORD* const pTextBufferSourceOrigin,
                                            _In_ const SMALL_RECT* const pTargetRectangle,
                                            _Out_ SMALL_RECT* const pAffectedRectangle);

    [[nodiscard]]
    virtual HRESULT WriteConsoleOutputWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                            _In_reads_(pTextBufferSize->X * pTextBufferSize->Y) const CHAR_INFO* const pTextBuffer,
                                            _In_ const COORD* const pTextBufferSize,
                                            _In_ const COORD* const pTextBufferSourceOrigin,
                                            _In_ const SMALL_RECT* const pTargetRectangle,
                                            _Out_ SMALL_RECT* const pAffectedRectangle);

    [[nodiscard]]
    virtual HRESULT WriteConsoleOutputAttributeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                    _In_reads_(AttributeBufferLength) const WORD* const pAttributeBuffer,
                                                    _In_ ULONG const AttributeBufferLength,
                                                    _In_ const COORD* const pTargetOrigin,
                                                    _Out_ ULONG* const pAttributeBufferRead);

    [[nodiscard]]
    virtual HRESULT WriteConsoleOutputCharacterAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                     _In_reads_(TextBufferLength) const char* const pTextBuffer,
                                                     _In_ ULONG const TextBufferLength,
                                                     _In_ const COORD* const pTargetOrigin,
                                                     _Out_ ULONG* const pTextBufferRead);

    [[nodiscard]]
    virtual HRESULT WriteConsoleOutputCharacterWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                     _In_reads_(TextBufferLength) const wchar_t* const pTextBuffer,
                                                     _In_ ULONG const TextBufferLength,
                                                     _In_ const COORD* const pTargetOrigin,
                                                     _Out_ ULONG* const pTextBufferRead);

    [[nodiscard]]
    virtual HRESULT ReadConsoleOutputA(_In_ IConsoleOutputObject* const pOutContext,
                                       _Out_writes_(pTextBufferSize->X * pTextBufferSize->Y) CHAR_INFO* const pTextBuffer,
                                       _In_ const COORD* const pTextBufferSize,
                                       _In_ const COORD* const pTextBufferTargetOrigin,
                                       _In_ const SMALL_RECT* const pSourceRectangle,
                                       _Out_ SMALL_RECT* const pReadRectangle);

    [[nodiscard]]
    virtual HRESULT ReadConsoleOutputW(_In_ IConsoleOutputObject* const pOutContext,
                                       _Out_writes_(pTextBufferSize->X * pTextBufferSize->Y) CHAR_INFO* const pTextBuffer,
                                       _In_ const COORD* const pTextBufferSize,
                                       _In_ const COORD* const pTextBufferTargetOrigin,
                                       _In_ const SMALL_RECT* const pSourceRectangle,
                                       _Out_ SMALL_RECT* const pReadRectangle);

    [[nodiscard]]
    virtual HRESULT GetConsoleTitleAImpl(_Out_writes_to_(cchTitleBufferSize, *pcchTitleBufferWritten) _Always_(_Post_z_) char* const psTitleBuffer,
                                         _In_ size_t const cchTitleBufferSize,
                                         _Out_ size_t* const pcchTitleBufferWritten,
                                         _Out_ size_t* const pcchTitleBufferNeeded) = 0;

    virtual void GetConsoleTitleWImpl(_Out_writes_to_(cchTitleBufferSize, *pcchTitleBufferWritten) _Always_(_Post_z_) wchar_t* const pwsTitleBuffer,
                                         _In_ size_t const cchTitleBufferSize,
                                         _Out_ size_t* const pcchTitleBufferWritten,
                                         _Out_ size_t* const pcchTitleBufferNeeded) = 0;

    [[nodiscard]]
    virtual HRESULT GetConsoleOriginalTitleAImpl(_Out_writes_to_(cchTitleBufferSize, *pcchTitleBufferWritten) _Always_(_Post_z_) char* const psTitleBuffer,
                                                 _In_ size_t const cchTitleBufferSize,
                                                 _Out_ size_t* const pcchTitleBufferWritten,
                                                 _Out_ size_t* const pcchTitleBufferNeeded) = 0;

    virtual void GetConsoleOriginalTitleWImpl(_Out_writes_to_(cchTitleBufferSize, *pcchTitleBufferWritten) _Always_(_Post_z_) wchar_t* const pwsTitleBuffer,
                                                 _In_ size_t const cchTitleBufferSize,
                                                 _Out_ size_t* const pcchTitleBufferWritten,
                                                 _Out_ size_t* const pcchTitleBufferNeeded) = 0;

    [[nodiscard]]
    virtual HRESULT SetConsoleTitleAImpl(_In_reads_or_z_(cchTitleBufferSize) const char* const psTitleBuffer,
                                         _In_ size_t const cchTitleBufferSize) = 0;

    [[nodiscard]]
    virtual HRESULT SetConsoleTitleWImpl(_In_reads_or_z_(cchTitleBufferSize) const wchar_t* const pwsTitleBuffer,
                                         _In_ size_t const cchTitleBufferSize) = 0;

#pragma endregion

#pragma region L3
    virtual void GetNumberOfConsoleMouseButtonsImpl(_Out_ ULONG* const pButtons) = 0;

    [[nodiscard]]
    virtual HRESULT GetConsoleFontSizeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                           _In_ DWORD const FontIndex,
                                           _Out_ COORD* const pFontSize) = 0;

    // driver will pare down for non-Ex method
    [[nodiscard]]
    virtual HRESULT GetCurrentConsoleFontExImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                _In_ BOOLEAN const IsForMaximumWindowSize,
                                                _Out_ CONSOLE_FONT_INFOEX* const pConsoleFontInfoEx) = 0;

    [[nodiscard]]
    virtual HRESULT SetConsoleDisplayModeImpl(_In_ SCREEN_INFORMATION* const pContext,
                                              _In_ ULONG const Flags,
                                              _Out_ COORD* const pNewScreenBufferSize) = 0;

    virtual void GetConsoleDisplayModeImpl(_Out_ ULONG* const pFlags) = 0;

    [[nodiscard]]
    virtual HRESULT AddConsoleAliasAImpl(_In_reads_or_z_(cchSourceBufferLength) const char* const psSourceBuffer,
                                         _In_ size_t const cchSourceBufferLength,
                                         _In_reads_or_z_(cchTargetBufferLength) const char* const psTargetBuffer,
                                         _In_ size_t const cchTargetBufferLength,
                                         _In_reads_or_z_(cchExeNameBufferLength) const char* const psExeNameBuffer,
                                         _In_ size_t const cchExeNameBufferLength) = 0;

    [[nodiscard]]
    virtual HRESULT AddConsoleAliasWImpl(_In_reads_or_z_(cchSourceBufferLength) const wchar_t* const pwsSourceBuffer,
                                         _In_ size_t const cchSourceBufferLength,
                                         _In_reads_or_z_(cchTargetBufferLength) const wchar_t* const pwsTargetBuffer,
                                         _In_ size_t const cchTargetBufferLength,
                                         _In_reads_or_z_(cchExeNameBufferLength) const wchar_t* const pwsExeNameBuffer,
                                         _In_ size_t const cchExeNameBufferLength) = 0;

    [[nodiscard]]
    virtual HRESULT GetConsoleAliasAImpl(_In_reads_or_z_(cchSourceBufferLength) const char* const psSourceBuffer,
                                         _In_ size_t const cchSourceBufferLength,
                                         _Out_writes_to_(cchTargetBufferLength, *pcchTargetBufferWritten) _Always_(_Post_z_) char* const psTargetBuffer,
                                         _In_ size_t const cchTargetBufferLength,
                                         _Out_ size_t* const pcchTargetBufferWritten,
                                         _In_reads_or_z_(cchExeNameBufferLength) const char* const psExeNameBuffer,
                                         _In_ size_t const cchExeNameBufferLength) = 0;

    [[nodiscard]]
    virtual HRESULT GetConsoleAliasWImpl(_In_reads_or_z_(cchSourceBufferLength) const wchar_t* const pwsSourceBuffer,
                                         _In_ size_t const cchSourceBufferLength,
                                         _Out_writes_to_(cchTargetBufferLength, *pcchTargetBufferWritten) _Always_(_Post_z_) wchar_t* const pwsTargetBuffer,
                                         _In_ size_t const cchTargetBufferLength,
                                         _Out_ size_t* const pcchTargetBufferWritten,
                                         _In_reads_or_z_(cchExeNameBufferLength) const wchar_t* const pwsExeNameBuffer,
                                         _In_ size_t const cchExeNameBufferLength) = 0;

    [[nodiscard]]
    virtual HRESULT GetConsoleAliasesLengthAImpl(_In_reads_or_z_(cchExeNameBufferLength) const char* const psExeNameBuffer,
                                                 _In_ size_t const cchExeNameBufferLength,
                                                 _Out_ size_t* const pcchAliasesBufferRequired) = 0;

    [[nodiscard]]
    virtual HRESULT GetConsoleAliasesLengthWImpl(_In_reads_or_z_(cchExeNameBufferLength) const wchar_t* const pwsExeNameBuffer,
                                                 _In_ size_t const cchExeNameBufferLength,
                                                 _Out_ size_t* const pcchAliasesBufferRequired) = 0;

    [[nodiscard]]
    virtual HRESULT GetConsoleAliasExesLengthAImpl(_Out_ size_t* const pcchAliasExesBufferRequired) = 0;

    [[nodiscard]]
    virtual HRESULT GetConsoleAliasExesLengthWImpl(_Out_ size_t* const pcchAliasExesBufferRequired) = 0;

    [[nodiscard]]
    virtual HRESULT GetConsoleAliasesAImpl(_In_reads_or_z_(cchExeNameBufferLength) const char* const psExeNameBuffer,
                                           _In_ size_t const cchExeNameBufferLength,
                                           _Out_writes_to_(cchAliasBufferLength, *pcchAliasBufferWritten) _Always_(_Post_z_) char* const psAliasBuffer,
                                           _In_ size_t const cchAliasBufferLength,
                                           _Out_ size_t* const pcchAliasBufferWritten) = 0;

    [[nodiscard]]
    virtual HRESULT GetConsoleAliasesWImpl(_In_reads_or_z_(cchExeNameBufferLength) const wchar_t* const pwsExeNameBuffer,
                                           _In_ size_t const cchExeNameBufferLength,
                                           _Out_writes_to_(cchAliasBufferLength, *pcchAliasBufferWritten) _Always_(_Post_z_) wchar_t* const pwsAliasBuffer,
                                           _In_ size_t const cchAliasBufferLength,
                                           _Out_ size_t* const pcchAliasBufferWritten) = 0;

    [[nodiscard]]
    virtual HRESULT GetConsoleAliasExesAImpl(_Out_writes_to_(cchAliasExesBufferLength, *pcchAliasExesBufferWritten) _Always_(_Post_z_) char* const psAliasExesBuffer,
                                             _In_ size_t const cchAliasExesBufferLength,
                                             _Out_ size_t* const pcchAliasExesBufferWritten) = 0;

    [[nodiscard]]
    virtual HRESULT GetConsoleAliasExesWImpl(_Out_writes_to_(cchAliasExesBufferLength, *pcchAliasExesBufferWritten) _Always_(_Post_z_) wchar_t* const pwsAliasExesBuffer,
                                             _In_ size_t const cchAliasExesBufferLength,
                                             _Out_ size_t* const pcchAliasExesBufferWritten) = 0;

#pragma region CMDext Private API

    [[nodiscard]]
    virtual HRESULT ExpungeConsoleCommandHistoryAImpl(_In_reads_or_z_(cchExeNameBufferLength) const char* const psExeNameBuffer,
                                                      _In_ size_t const cchExeNameBufferLength) = 0;

    [[nodiscard]]
    virtual HRESULT ExpungeConsoleCommandHistoryWImpl(_In_reads_or_z_(cchExeNameBufferLength) const wchar_t* const pwsExeNameBuffer,
                                                      _In_ size_t const cchExeNameBufferLength) = 0;

    [[nodiscard]]
    virtual HRESULT SetConsoleNumberOfCommandsAImpl(_In_reads_or_z_(cchExeNameBufferLength) const char* const psExeNameBuffer,
                                                    _In_ size_t const cchExeNameBufferLength,
                                                    _In_ size_t const NumberOfCommands) = 0;

    [[nodiscard]]
    virtual HRESULT SetConsoleNumberOfCommandsWImpl(_In_reads_or_z_(cchExeNameBufferLength) const wchar_t* const pwsExeNameBuffer,
                                                    _In_ size_t const cchExeNameBufferLength,
                                                    _In_ size_t const NumberOfCommands) = 0;

    [[nodiscard]]
    virtual HRESULT GetConsoleCommandHistoryLengthAImpl(_In_reads_or_z_(cchExeNameBufferLength) const char* const psExeNameBuffer,
                                                        _In_ size_t const cchExeNameBufferLength,
                                                        _Out_ size_t* const pcchCommandHistoryLength) = 0;

    [[nodiscard]]
    virtual HRESULT GetConsoleCommandHistoryLengthWImpl(_In_reads_or_z_(cchExeNameBufferLength) const wchar_t* const pwsExeNameBuffer,
                                                        _In_ size_t const cchExeNameBufferLength,
                                                        _Out_ size_t* const pcchCommandHistoryLength) = 0;

    [[nodiscard]]
    virtual HRESULT GetConsoleCommandHistoryAImpl(_In_reads_or_z_(cchExeNameBufferLength) const char* const psExeNameBuffer,
                                                  _In_ size_t const cchExeNameBufferLength,
                                                  _Out_writes_to_(cchCommandHistoryBufferLength, *pcchCommandHistoryBufferWritten) _Always_(_Post_z_) char* const psCommandHistoryBuffer,
                                                  _In_ size_t const cchCommandHistoryBufferLength,
                                                  _Out_ size_t* const pcchCommandHistoryBufferWritten) = 0;

    [[nodiscard]]
    virtual HRESULT GetConsoleCommandHistoryWImpl(_In_reads_or_z_(cchExeNameBufferLength) const wchar_t* const pwsExeNameBuffer,
                                                  _In_ size_t const cchExeNameBufferLength,
                                                  _Out_writes_to_(cchCommandHistoryBufferLength, *pcchCommandHistoryBufferWritten) _Always_(_Post_z_) wchar_t* const pwsCommandHistoryBuffer,
                                                  _In_ size_t const cchCommandHistoryBufferLength,
                                                  _Out_ size_t* const pcchCommandHistoryBufferWritten) = 0;

#pragma endregion

    virtual void GetConsoleWindowImpl(_Out_ HWND* const pHwnd) = 0;

    virtual void GetConsoleSelectionInfoImpl(_Out_ CONSOLE_SELECTION_INFO* const pConsoleSelectionInfo) = 0;

    virtual void GetConsoleHistoryInfoImpl(_Out_ CONSOLE_HISTORY_INFO* const pConsoleHistoryInfo) = 0;

    [[nodiscard]]
    virtual HRESULT SetConsoleHistoryInfoImpl(_In_ const CONSOLE_HISTORY_INFO* const pConsoleHistoryInfo) = 0;

    [[nodiscard]]
    virtual HRESULT SetCurrentConsoleFontExImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                _In_ BOOLEAN const IsForMaximumWindowSize,
                                                _In_ const CONSOLE_FONT_INFOEX* const pConsoleFontInfoEx) = 0;

#pragma endregion
};
