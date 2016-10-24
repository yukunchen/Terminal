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
#pragma region ObjectManagement
    /*HRESULT CreateInitialObjects(_Out_ INPUT_INFORMATION** const ppInputObject,
    _Out_ SCREEN_INFORMATION** const ppOutputObject);
    */

#pragma endregion

#pragma region L1
    HRESULT GetConsoleInputCodePageImpl(_Out_ ULONG* const pCodePage);

    HRESULT GetConsoleOutputCodePageImpl(_Out_ ULONG* const pCodePage);

    HRESULT GetConsoleInputModeImpl(_In_ INPUT_INFORMATION* const pContext,
                                            _Out_ ULONG* const pMode);

    HRESULT GetConsoleOutputModeImpl(_In_ SCREEN_INFORMATION* const pContext,
                                             _Out_ ULONG* const pMode);

    HRESULT SetConsoleInputModeImpl(_In_ INPUT_INFORMATION* const pContext,
                                            _In_ ULONG const Mode);

    HRESULT SetConsoleOutputModeImpl(_In_ SCREEN_INFORMATION* const pContext,
                                             _In_ ULONG const Mode);

    HRESULT GetNumberOfConsoleInputEventsImpl(_In_ INPUT_INFORMATION* const pContext,
                                                      _Out_ ULONG* const pEvents);

   // HRESULT PeekConsoleInputAImpl(_In_ INPUT_INFORMATION* const pContext,
   //                                       _Out_writes_to_(InputRecordsBufferLength, *pRecordsWritten) INPUT_RECORD* const pInputRecordsBuffer,
   //                                       _In_ ULONG const InputRecordsBufferLength,
   //                                       _Out_ ULONG* const pRecordsWritten);

   // HRESULT PeekConsoleInputWImpl(_In_ INPUT_INFORMATION* const pContext,
   //                                       _Out_writes_to_(InputRecordsBufferLength, *pRecordsWritten) INPUT_RECORD* const pInputRecordsBuffer,
   //                                       _In_ ULONG const InputRecordsBufferLength,
   //                                       _Out_ ULONG* const pRecordsWritten);

   // HRESULT ReadConsoleInputAImpl(_In_ INPUT_INFORMATION* const pContext,
   //                                       _Out_writes_to_(InputRecordsBufferLength, *pRecordsWritten) INPUT_RECORD* const pInputRecordsBuffer,
   //                                       _In_ ULONG const InputRecordsBufferLength,
   //                                       _Out_ ULONG* const pRecordsWritten);

   // HRESULT ReadConsoleInputWImpl(_In_ INPUT_INFORMATION* const pContext,
   //                                       _Out_writes_to_(InputRecordsBufferLength, *pRecordsWritten) INPUT_RECORD* const pInputRecordsBuffer,
   //                                       _In_ ULONG const InputRecordsBufferLength,
   //                                       _Out_ ULONG* const pRecordsWritten);

   // HRESULT ReadConsoleAImpl(_In_ INPUT_INFORMATION* const pContext,
   //                                  _Out_writes_to_(TextBufferLength, *pTextBufferWritten) char* const pTextBuffer,
   //                                  _In_ ULONG const TextBufferLength,
   //                                  _Out_ ULONG* const pTextBufferWritten,
   //                                  _In_opt_ CONSOLE_READCONSOLE_CONTROL* const pReadControl);

   // HRESULT ReadConsoleWImpl(_In_ INPUT_INFORMATION* const pContext,
   //                                  _Out_writes_to_(TextBufferLength, *pTextBufferWritten) wchar_t* const pTextBuffer,
   //                                  _In_ ULONG const TextBufferLength,
   //                                  _Out_ ULONG* const pTextBufferWritten,
   //                                  _In_opt_ CONSOLE_READCONSOLE_CONTROL* const pReadControl);

   // HRESULT WriteConsoleAImpl(_In_ SCREEN_INFORMATION* const pContext,
   //                                   _In_reads_(TextBufferLength) char* const pTextBuffer,
   //                                   _In_ ULONG const TextBufferLength,
   //                                   _Out_ ULONG* const pTextBufferRead);

   // HRESULT WriteConsoleWImpl(_In_ SCREEN_INFORMATION* const pContext,
   //                                   _In_reads_(TextBufferLength) wchar_t* const pTextBuffer,
   //                                   _In_ ULONG const TextBufferLength,
   //                                   _Out_ ULONG* const pTextBufferRead);

#pragma endregion

#pragma region L2

    //HRESULT FillConsoleOutputAttributeImpl(_In_ SCREEN_INFORMATION* const pContext,
    //                                               _In_ WORD const Attribute,
    //                                               _In_ DWORD const LengthToWrite,
    //                                               _In_ COORD const StartingCoordinate,
    //                                               _Out_ DWORD* const pCellsModified);

    //HRESULT FillConsoleOutputCharacterAImpl(_In_ SCREEN_INFORMATION* const pContext,
    //                                                _In_ char const Character,
    //                                                _In_ DWORD const LengthToWrite,
    //                                                _In_ COORD const StartingCoordinate,
    //                                                _Out_ DWORD* const pCellsModified);

    //HRESULT FillConsoleOutputCharacterWImpl(_In_ SCREEN_INFORMATION* const pContext,
    //                                                _In_ wchar_t const Character,
    //                                                _In_ DWORD const LengthToWrite,
    //                                                _In_ COORD const StartingCoordinate,
    //                                                _Out_ DWORD* const pCellsModified);

    //// Process based. Restrict in protocol side?
    //HRESULT GenerateConsoleCtrlEventImpl(_In_ ULONG const ProcessGroupFilter,
    //                                             _In_ ULONG const ControlEvent);

    //HRESULT SetConsoleActiveScreenBufferImpl(_In_ HANDLE const NewOutContext);

    //HRESULT FlushConsoleInputBuffer(_In_ INPUT_INFORMATION* const pContext);

    //HRESULT SetConsoleInputCodePageImpl(_In_ ULONG const CodePage);

    //HRESULT SetConsoleOutputCodePageImpl(_In_ ULONG const CodePage);

    //HRESULT GetConsoleCursorInfoImpl(_In_ SCREEN_INFORMATION* const pContext,
    //                                         _Out_ ULONG* const pCursorSize,
    //                                         _Out_ BOOLEAN* const pIsVisible);

    //HRESULT SetConsoleCursorInfoImpl(_In_ SCREEN_INFORMATION* const pContext,
    //                                         _In_ ULONG const CursorSize,
    //                                         _In_ BOOLEAN const IsVisible);

    //// driver will pare down for non-Ex method
    //HRESULT GetConsoleScreenBufferInfoExImpl(_In_ SCREEN_INFORMATION* const pContext,
    //                                                 _Out_ CONSOLE_SCREEN_BUFFER_INFOEX* const pScreenBufferInfoEx);

    //HRESULT SetConsoleScreenBufferInfoExImpl(_In_ SCREEN_INFORMATION* const pContext,
    //                                                 _In_ const CONSOLE_SCREEN_BUFFER_INFOEX* const pScreenBufferInfoEx);

    //HRESULT SetConsoleScreenBufferSizeImpl(_In_ SCREEN_INFORMATION* const pContext,
    //                                               _In_ const COORD* const pSize);

    //HRESULT SetConsoleCursorPositionImpl(_In_ SCREEN_INFORMATION* const pContext,
    //                                             _In_ const COORD* const pCursorPosition);

    //HRESULT GetLargestConsoleWindowSizeImpl(_In_ SCREEN_INFORMATION* const pContext,
    //                                                _Out_ COORD* const pSize);

    //HRESULT ScrollConsoleScreenBufferAImpl(_In_ SCREEN_INFORMATION* const pContext,
    //                                               _In_ const SMALL_RECT* const pSourceRectangle,
    //                                               _In_ const COORD* const pTargetOrigin,
    //                                               _In_opt_ const SMALL_RECT* const pTargetClipRectangle,
    //                                               _In_ const CHAR_INFO* const pFill);

    //HRESULT ScrollConsoleScreenBufferWImpl(_In_ SCREEN_INFORMATION* const pContext,
    //                                               _In_ const SMALL_RECT* const pSourceRectangle,
    //                                               _In_ const COORD* const pTargetOrigin,
    //                                               _In_opt_ const SMALL_RECT* const pTargetClipRectangle,
    //                                               _In_ const CHAR_INFO* const pFill);

    //HRESULT SetConsoleTextAttributeImpl(_In_ SCREEN_INFORMATION* const pContext,
    //                                            _In_ WORD const Attribute);

    //HRESULT SetConsoleWindowInfoImpl(_In_ SCREEN_INFORMATION* const pContext,
    //                                         _In_ BOOLEAN const IsAbsoluteRectangle,
    //                                         _In_ const SMALL_RECT* const pWindowRectangle);

    //HRESULT ReadConsoleOutputAttributeImpl(_In_ SCREEN_INFORMATION* const pContext,
    //                                               _In_ const COORD* const pSourceOrigin,
    //                                               _Out_writes_to_(AttributeBufferLength, *pAttributeBufferWritten) WORD* const pAttributeBuffer,
    //                                               _In_ ULONG const AttributeBufferLength,
    //                                               _Out_ ULONG* const pAttributeBufferWritten);

    //HRESULT ReadConsoleOutputCharacterAImpl(_In_ SCREEN_INFORMATION* const pContext,
    //                                                _In_ const COORD* const pSourceOrigin,
    //                                                _Out_writes_to_(TextBufferLength, *pTextBufferWritten) char* const pTextBuffer,
    //                                                _In_ ULONG const pTextBufferLength,
    //                                                _Out_ ULONG* const pTextBufferWritten);

    //HRESULT ReadConsoleOutputCharacterWImpl(_In_ SCREEN_INFORMATION* const pContext,
    //                                                _In_ const COORD* const pSourceOrigin,
    //                                                _Out_writes_to_(TextBufferLength, *pTextBufferWritten) wchar_t* const pTextBuffer,
    //                                                _In_ ULONG const TextBufferLength,
    //                                                _Out_ ULONG* const pTextBufferWritten);

    //HRESULT WriteConsoleInputAImpl(_In_ INPUT_INFORMATION* const pContext,
    //                                       _In_reads_(InputBufferLength) const INPUT_RECORD* const pInputBuffer,
    //                                       _In_ ULONG const InputBufferLength,
    //                                       _Out_ ULONG* const pInputBufferRead);

    //HRESULT WriteConsoleInputWImpl(_In_ INPUT_INFORMATION* const pContext,
    //                                       _In_reads_(InputBufferLength) const INPUT_RECORD* const pInputBuffer,
    //                                       _In_ ULONG const InputBufferLength,
    //                                       _Out_ ULONG* const pInputBufferRead);

    //HRESULT WriteConsoleOutputAImpl(_In_ SCREEN_INFORMATION* const pContext,
    //                                        _In_reads_(pTextBufferSize->X * pTextBufferSize->Y) const CHAR_INFO* const pTextBuffer,
    //                                        _In_ const COORD* const pTextBufferSize,
    //                                        _In_ const COORD* const pTextBufferSourceOrigin,
    //                                        _In_ const SMALL_RECT* const pTargetRectangle,
    //                                        _Out_ SMALL_RECT* const pAffectedRectangle);

    //HRESULT WriteConsoleOutputWImpl(_In_ SCREEN_INFORMATION* const pContext,
    //                                        _In_reads_(pTextBufferSize->X * pTextBufferSize->Y) const CHAR_INFO* const pTextBuffer,
    //                                        _In_ const COORD* const pTextBufferSize,
    //                                        _In_ const COORD* const pTextBufferSourceOrigin,
    //                                        _In_ const SMALL_RECT* const pTargetRectangle,
    //                                        _Out_ SMALL_RECT* const pAffectedRectangle);

    //HRESULT WriteConsoleOutputAttributeImpl(_In_ SCREEN_INFORMATION* const pContext,
    //                                                _In_reads_(AttributeBufferLength) const WORD* const pAttributeBuffer,
    //                                                _In_ ULONG const AttributeBufferLength,
    //                                                _In_ const COORD* const pTargetOrigin,
    //                                                _Out_ ULONG* const pAttributeBufferRead);

    //HRESULT WriteConsoleOutputCharacterAImpl(_In_ SCREEN_INFORMATION* const pContext,
    //                                                 _In_reads_(TextBufferLength) const char* const pTextBuffer,
    //                                                 _In_ ULONG const TextBufferLength,
    //                                                 _In_ const COORD* const pTargetOrigin,
    //                                                 _Out_ ULONG* const pTextBufferRead);

    //HRESULT WriteConsoleOutputCharacterWImpl(_In_ SCREEN_INFORMATION* const pContext,
    //                                                 _In_reads_(TextBufferLength) const wchar_t* const pTextBuffer,
    //                                                 _In_ ULONG const TextBufferLength,
    //                                                 _In_ const COORD* const pTargetOrigin,
    //                                                 _Out_ ULONG* const pTextBufferRead);

    //HRESULT ReadConsoleOutputA(_In_ SCREEN_INFORMATION* const pContext,
    //                                   _Out_writes_(pTextBufferSize->X * pTextBufferSize->Y) CHAR_INFO* const pTextBuffer,
    //                                   _In_ const COORD* const pTextBufferSize,
    //                                   _In_ const COORD* const pTextBufferTargetOrigin,
    //                                   _In_ const SMALL_RECT* const pSourceRectangle,
    //                                   _Out_ SMALL_RECT* const pReadRectangle);

    //HRESULT ReadConsoleOutputW(_In_ SCREEN_INFORMATION* const pContext,
    //                                   _Out_writes_(pTextBufferSize->X * pTextBufferSize->Y) CHAR_INFO* const pTextBuffer,
    //                                   _In_ const COORD* const pTextBufferSize,
    //                                   _In_ const COORD* const pTextBufferTargetOrigin,
    //                                   _In_ const SMALL_RECT* const pSourceRectangle,
    //                                   _Out_ SMALL_RECT* const pReadRectangle);

    //HRESULT GetConsoleTitleAImpl(_Out_writes_(TextBufferSize) char* const pTextBuffer,
    //                                     _In_ ULONG const TextBufferSize);

    //HRESULT GetConsoleTitleWImpl(_Out_writes_(TextBufferSize) wchar_t* const pTextBuffer,
    //                                     _In_ ULONG const TextBufferSize);

    //HRESULT GetConsoleOriginalTitleAImpl(_Out_writes_(TextBufferSize) char* const pTextBuffer,
    //                                             _In_ ULONG const TextBufferSize);

    //HRESULT GetConsoleOriginalTitleWImpl(_Out_writes_(TextBufferSize) char* const pTextBuffer,
    //                                             _In_ ULONG const TextBufferSize);

    //HRESULT SetConsoleTitleAImpl(_In_reads_(TextBufferSize) char* const pTextBuffer,
    //                                     _In_ ULONG const TextBufferSize);

    //HRESULT SetConsoleTitleWImpl(_In_reads_(TextBufferSize) wchar_t* const pTextBuffer,
    //                                     _In_ ULONG const TextBufferSize);

#pragma endregion

#pragma region L3
    //HRESULT GetNumberOfConsoleMouseButtonsImpl(_Out_ ULONG* const pButtons);

    //HRESULT GetConsoleFontSizeImpl(_In_ SCREEN_INFORMATION* const pContext,
    //                                       _In_ DWORD const FontIndex,
    //                                       _Out_ COORD* const pFontSize);

    //// driver will pare down for non-Ex method
    //HRESULT GetCurrentConsoleFontExImpl(_In_ SCREEN_INFORMATION* const pContext,
    //                                            _In_ BOOLEAN const IsForMaximumWindowSize,
    //                                            _Out_ CONSOLE_FONT_INFOEX* const pConsoleFontInfoEx);

    //HRESULT SetConsoleDisplayModeImpl(_In_ SCREEN_INFORMATION* const pContext,
    //                                          _In_ ULONG const Flags,
    //                                          _Out_ COORD* const pNewScreenBufferSize);

    //HRESULT GetConsoleDisplayModeImpl(_In_ SCREEN_INFORMATION* const pContext,
    //                                          _Out_ ULONG* const pFlags);

    //HRESULT AddConsoleAliasAImpl(_In_reads_(SourceBufferLength) const char* const pSourceBuffer,
    //                                     _In_ ULONG const SourceBufferLength,
    //                                     _In_reads_(TargetBufferLength) const char* const pTargetBuffer,
    //                                     _In_ ULONG const TargetBufferLength,
    //                                     _In_reads_(ExeNameBufferLength) const char* const pExeNameBuffer,
    //                                     _In_ ULONG const ExeNameBufferLength);

    //HRESULT AddConsoleAliasWImpl(_In_reads_(SourceBufferLength) const wchar_t* const pSourceBuffer,
    //                                     _In_ ULONG const SourceBufferLength,
    //                                     _In_reads_(TargetBufferLength) const wchar_t* const pTargetBuffer,
    //                                     _In_ ULONG const TargetBufferLength,
    //                                     _In_reads_(ExeNameBufferLength) const wchar_t* const pExeNameBuffer,
    //                                     _In_ ULONG const ExeNameBufferLength);

    //HRESULT GetConsoleAliasAImpl(_In_reads_(SourceBufferLength) const char* const pSourceBuffer,
    //                                     _In_ ULONG const SourceBufferLength,
    //                                     _Out_writes_(TargetBufferLength) char* const pTargetBuffer,
    //                                     _In_ ULONG const TargetBufferLength,
    //                                     _In_reads_(ExeNameBufferLength) const char* const pExeNameBuffer,
    //                                     _In_ ULONG const ExeNameBufferLength);

    //HRESULT GetConsoleAliasWImpl(_In_reads_(SourceBufferLength) const wchar_t* const pSourceBuffer,
    //                                     _In_ ULONG const SourceBufferLength,
    //                                     _Out_writes_(TargetBufferLength) wchar_t* const pTargetBuffer,
    //                                     _In_ ULONG const TargetBufferLength,
    //                                     _In_reads_(ExeNameBufferLength) const wchar_t* const pExeNameBuffer,
    //                                     _In_ ULONG const ExeNameBufferLength);

    //HRESULT GetConsoleAliasesLengthAImpl(_In_reads_(ExeNameBufferLength) const char* const pExeNameBuffer,
    //                                             _In_ ULONG const ExeNameBufferLength,
    //                                             _Out_ ULONG* const pAliasesBufferRequired);

    //HRESULT GetConsoleAliasesLengthWImpl(_In_reads_(ExeNameBufferLength) const wchar_t* const pExeNameBuffer,
    //                                             _In_ ULONG const ExeNameBufferLength,
    //                                             _Out_ ULONG* const pAliasesBufferRequired);

    //HRESULT GetConsoleAliasExesLengthAImpl(_Out_ ULONG* const pAliasExesBufferRequired);

    //HRESULT GetConsoleAliasExesLengthWImpl(_Out_ ULONG* const pAliasExesBufferRequired);

    //HRESULT GetConsoleAliasesAImpl(_In_reads_(ExeNameBufferLength) const char* const pExeNameBuffer,
    //                                       _In_ ULONG const ExeNameBufferLength,
    //                                       _Out_writes_(AliasBufferLength) char* const pAliasBuffer,
    //                                       _In_ ULONG const AliasBufferLength);

    //HRESULT GetConsoleAliasesWImpl(_In_reads_(ExeNameBufferLength) const wchar_t* const pExeNameBuffer,
    //                                       _In_ ULONG const ExeNameBufferLength,
    //                                       _Out_writes_(AliasBufferLength) wchar_t* const pAliasBuffer,
    //                                       _In_ ULONG const AliasBufferLength);

    //HRESULT GetConsoleAliasExesAImpl(_Out_writes_(AliasExesBufferLength) char* const pAliasExesBuffer,
    //                                         _In_ ULONG const AliasExesBufferLength);

    //HRESULT GetConsoleAliasExesWImpl(_Out_writes_(AliasExesBufferLength) wchar_t* const pAliasExesBuffer,
    //                                         _In_ ULONG const AliasExesBufferLength);

    //HRESULT GetConsoleWindowImpl(_Out_ HWND* const pHwnd);

    //HRESULT GetConsoleSelectionInfoImpl(_Out_ CONSOLE_SELECTION_INFO* const pConsoleSelectionInfo);

    //HRESULT GetConsoleHistoryInfoImpl(_Out_ CONSOLE_HISTORY_INFO* const pConsoleHistoryInfo);

    //HRESULT SetConsoleHistoryInfoImpl(_In_ const CONSOLE_HISTORY_INFO* const pConsoleHistoryInfo);

    //HRESULT SetCurrentConsoleFontExImpl(_In_ SCREEN_INFORMATION* const pContext,
    //                                            _In_ BOOLEAN const IsForMaximumWindowSize,
    //                                            _In_ const CONSOLE_FONT_INFOEX* const pConsoleFontInfoEx);

#pragma endregion
};