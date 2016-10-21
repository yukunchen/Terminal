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
    /*HRESULT CreateInitialObjects(_Out_ IConsoleInputObject** const ppInputObject,
    _Out_ IConsoleOutputObject** const ppOutputObject) = 0;
    */

#pragma endregion

#pragma region L1
    HRESULT GetConsoleInputCodePageImpl(_Out_ ULONG* const pCodePage);

    HRESULT GetConsoleOutputCodePageImpl(_Out_ ULONG* const pCodePage);

   /* HRESULT GetConsoleInputModeImpl(_In_ IConsoleInputObject* const pInContext,
                                            _Out_ ULONG* const pMode) = 0;

    HRESULT GetConsoleOutputModeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                             _Out_ ULONG* const pMode) = 0;

    HRESULT SetConsoleInputModeImpl(_In_ IConsoleInputObject* const pInContext,
                                            _In_ ULONG const Mode) = 0;

    HRESULT SetConsoleOutputModeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                             _In_ ULONG const Mode) = 0;

    HRESULT GetNumberOfConsoleInputEventsImpl(_In_ IConsoleInputObject* const pInContext,
                                                      _Out_ ULONG* const pEvents) = 0;

    HRESULT PeekConsoleInputAImpl(_In_ IConsoleInputObject* const pInContext,
                                          _Out_writes_to_(InputRecordsBufferLength, *pRecordsWritten) INPUT_RECORD* const pInputRecordsBuffer,
                                          _In_ ULONG const InputRecordsBufferLength,
                                          _Out_ ULONG* const pRecordsWritten) = 0;

    HRESULT PeekConsoleInputWImpl(_In_ IConsoleInputObject* const pInContext,
                                          _Out_writes_to_(InputRecordsBufferLength, *pRecordsWritten) INPUT_RECORD* const pInputRecordsBuffer,
                                          _In_ ULONG const InputRecordsBufferLength,
                                          _Out_ ULONG* const pRecordsWritten) = 0;

    HRESULT ReadConsoleInputAImpl(_In_ IConsoleInputObject* const pInContext,
                                          _Out_writes_to_(InputRecordsBufferLength, *pRecordsWritten) INPUT_RECORD* const pInputRecordsBuffer,
                                          _In_ ULONG const InputRecordsBufferLength,
                                          _Out_ ULONG* const pRecordsWritten) = 0;

    HRESULT ReadConsoleInputWImpl(_In_ IConsoleInputObject* const pInContext,
                                          _Out_writes_to_(InputRecordsBufferLength, *pRecordsWritten) INPUT_RECORD* const pInputRecordsBuffer,
                                          _In_ ULONG const InputRecordsBufferLength,
                                          _Out_ ULONG* const pRecordsWritten) = 0;

    HRESULT ReadConsoleAImpl(_In_ IConsoleInputObject* const pInContext,
                                     _Out_writes_to_(TextBufferLength, *pTextBufferWritten) char* const pTextBuffer,
                                     _In_ ULONG const TextBufferLength,
                                     _Out_ ULONG* const pTextBufferWritten,
                                     _In_opt_ CONSOLE_READCONSOLE_CONTROL* const pReadControl) = 0;

    HRESULT ReadConsoleWImpl(_In_ IConsoleInputObject* const pInContext,
                                     _Out_writes_to_(TextBufferLength, *pTextBufferWritten) wchar_t* const pTextBuffer,
                                     _In_ ULONG const TextBufferLength,
                                     _Out_ ULONG* const pTextBufferWritten,
                                     _In_opt_ CONSOLE_READCONSOLE_CONTROL* const pReadControl) = 0;

    HRESULT WriteConsoleAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                      _In_reads_(TextBufferLength) char* const pTextBuffer,
                                      _In_ ULONG const TextBufferLength,
                                      _Out_ ULONG* const pTextBufferRead) = 0;

    HRESULT WriteConsoleWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                      _In_reads_(TextBufferLength) wchar_t* const pTextBuffer,
                                      _In_ ULONG const TextBufferLength,
                                      _Out_ ULONG* const pTextBufferRead) = 0;*/

#pragma endregion

#pragma region L2

    //HRESULT FillConsoleOutputAttributeImpl(_In_ IConsoleOutputObject* const pOutContext,
    //                                               _In_ WORD const Attribute,
    //                                               _In_ DWORD const LengthToWrite,
    //                                               _In_ COORD const StartingCoordinate,
    //                                               _Out_ DWORD* const pCellsModified) = 0;

    //HRESULT FillConsoleOutputCharacterAImpl(_In_ IConsoleOutputObject* const pOutContext,
    //                                                _In_ char const Character,
    //                                                _In_ DWORD const LengthToWrite,
    //                                                _In_ COORD const StartingCoordinate,
    //                                                _Out_ DWORD* const pCellsModified) = 0;

    //HRESULT FillConsoleOutputCharacterWImpl(_In_ IConsoleOutputObject* const pOutContext,
    //                                                _In_ wchar_t const Character,
    //                                                _In_ DWORD const LengthToWrite,
    //                                                _In_ COORD const StartingCoordinate,
    //                                                _Out_ DWORD* const pCellsModified) = 0;

    //// Process based. Restrict in protocol side?
    //HRESULT GenerateConsoleCtrlEventImpl(_In_ ULONG const ProcessGroupFilter,
    //                                             _In_ ULONG const ControlEvent) = 0;

    //HRESULT SetConsoleActiveScreenBufferImpl(_In_ HANDLE const NewOutContext) = 0;

    //HRESULT FlushConsoleInputBuffer(_In_ IConsoleInputObject* const pInContext) = 0;

    //HRESULT SetConsoleInputCodePageImpl(_In_ ULONG const CodePage) = 0;

    //HRESULT SetConsoleOutputCodePageImpl(_In_ ULONG const CodePage) = 0;

    //HRESULT GetConsoleCursorInfoImpl(_In_ IConsoleOutputObject* const pOutContext,
    //                                         _Out_ ULONG* const pCursorSize,
    //                                         _Out_ BOOLEAN* const pIsVisible) = 0;

    //HRESULT SetConsoleCursorInfoImpl(_In_ IConsoleOutputObject* const pOutContext,
    //                                         _In_ ULONG const CursorSize,
    //                                         _In_ BOOLEAN const IsVisible) = 0;

    //// driver will pare down for non-Ex method
    //HRESULT GetConsoleScreenBufferInfoExImpl(_In_ IConsoleOutputObject* const pOutContext,
    //                                                 _Out_ CONSOLE_SCREEN_BUFFER_INFOEX* const pScreenBufferInfoEx) = 0;

    //HRESULT SetConsoleScreenBufferInfoExImpl(_In_ IConsoleOutputObject* const pOutContext,
    //                                                 _In_ const CONSOLE_SCREEN_BUFFER_INFOEX* const pScreenBufferInfoEx) = 0;

    //HRESULT SetConsoleScreenBufferSizeImpl(_In_ IConsoleOutputObject* const pOutContext,
    //                                               _In_ const COORD* const pSize) = 0;

    //HRESULT SetConsoleCursorPositionImpl(_In_ IConsoleOutputObject* const pOutContext,
    //                                             _In_ const COORD* const pCursorPosition) = 0;

    //HRESULT GetLargestConsoleWindowSizeImpl(_In_ IConsoleOutputObject* const pOutContext,
    //                                                _Out_ COORD* const pSize) = 0;

    //HRESULT ScrollConsoleScreenBufferAImpl(_In_ IConsoleOutputObject* const pOutContext,
    //                                               _In_ const SMALL_RECT* const pSourceRectangle,
    //                                               _In_ const COORD* const pTargetOrigin,
    //                                               _In_opt_ const SMALL_RECT* const pTargetClipRectangle,
    //                                               _In_ const CHAR_INFO* const pFill) = 0;

    //HRESULT ScrollConsoleScreenBufferWImpl(_In_ IConsoleOutputObject* const pOutContext,
    //                                               _In_ const SMALL_RECT* const pSourceRectangle,
    //                                               _In_ const COORD* const pTargetOrigin,
    //                                               _In_opt_ const SMALL_RECT* const pTargetClipRectangle,
    //                                               _In_ const CHAR_INFO* const pFill) = 0;

    //HRESULT SetConsoleTextAttributeImpl(_In_ IConsoleOutputObject* const pOutContext,
    //                                            _In_ WORD const Attribute) = 0;

    //HRESULT SetConsoleWindowInfoImpl(_In_ IConsoleOutputObject* const pOutContext,
    //                                         _In_ BOOLEAN const IsAbsoluteRectangle,
    //                                         _In_ const SMALL_RECT* const pWindowRectangle) = 0;

    //HRESULT ReadConsoleOutputAttributeImpl(_In_ IConsoleOutputObject* const pOutContext,
    //                                               _In_ const COORD* const pSourceOrigin,
    //                                               _Out_writes_to_(AttributeBufferLength, *pAttributeBufferWritten) WORD* const pAttributeBuffer,
    //                                               _In_ ULONG const AttributeBufferLength,
    //                                               _Out_ ULONG* const pAttributeBufferWritten) = 0;

    //HRESULT ReadConsoleOutputCharacterAImpl(_In_ IConsoleOutputObject* const pOutContext,
    //                                                _In_ const COORD* const pSourceOrigin,
    //                                                _Out_writes_to_(TextBufferLength, *pTextBufferWritten) char* const pTextBuffer,
    //                                                _In_ ULONG const pTextBufferLength,
    //                                                _Out_ ULONG* const pTextBufferWritten) = 0;

    //HRESULT ReadConsoleOutputCharacterWImpl(_In_ IConsoleOutputObject* const pOutContext,
    //                                                _In_ const COORD* const pSourceOrigin,
    //                                                _Out_writes_to_(TextBufferLength, *pTextBufferWritten) wchar_t* const pTextBuffer,
    //                                                _In_ ULONG const TextBufferLength,
    //                                                _Out_ ULONG* const pTextBufferWritten) = 0;

    //HRESULT WriteConsoleInputAImpl(_In_ IConsoleInputObject* const pInContext,
    //                                       _In_reads_(InputBufferLength) const INPUT_RECORD* const pInputBuffer,
    //                                       _In_ ULONG const InputBufferLength,
    //                                       _Out_ ULONG* const pInputBufferRead) = 0;

    //HRESULT WriteConsoleInputWImpl(_In_ IConsoleInputObject* const pInContext,
    //                                       _In_reads_(InputBufferLength) const INPUT_RECORD* const pInputBuffer,
    //                                       _In_ ULONG const InputBufferLength,
    //                                       _Out_ ULONG* const pInputBufferRead) = 0;

    //HRESULT WriteConsoleOutputAImpl(_In_ IConsoleOutputObject* const pOutContext,
    //                                        _In_reads_(pTextBufferSize->X * pTextBufferSize->Y) const CHAR_INFO* const pTextBuffer,
    //                                        _In_ const COORD* const pTextBufferSize,
    //                                        _In_ const COORD* const pTextBufferSourceOrigin,
    //                                        _In_ const SMALL_RECT* const pTargetRectangle,
    //                                        _Out_ SMALL_RECT* const pAffectedRectangle) = 0;

    //HRESULT WriteConsoleOutputWImpl(_In_ IConsoleOutputObject* const pOutContext,
    //                                        _In_reads_(pTextBufferSize->X * pTextBufferSize->Y) const CHAR_INFO* const pTextBuffer,
    //                                        _In_ const COORD* const pTextBufferSize,
    //                                        _In_ const COORD* const pTextBufferSourceOrigin,
    //                                        _In_ const SMALL_RECT* const pTargetRectangle,
    //                                        _Out_ SMALL_RECT* const pAffectedRectangle) = 0;

    //HRESULT WriteConsoleOutputAttributeImpl(_In_ IConsoleOutputObject* const pOutContext,
    //                                                _In_reads_(AttributeBufferLength) const WORD* const pAttributeBuffer,
    //                                                _In_ ULONG const AttributeBufferLength,
    //                                                _In_ const COORD* const pTargetOrigin,
    //                                                _Out_ ULONG* const pAttributeBufferRead) = 0;

    //HRESULT WriteConsoleOutputCharacterAImpl(_In_ IConsoleOutputObject* const pOutContext,
    //                                                 _In_reads_(TextBufferLength) const char* const pTextBuffer,
    //                                                 _In_ ULONG const TextBufferLength,
    //                                                 _In_ const COORD* const pTargetOrigin,
    //                                                 _Out_ ULONG* const pTextBufferRead) = 0;

    //HRESULT WriteConsoleOutputCharacterWImpl(_In_ IConsoleOutputObject* const pOutContext,
    //                                                 _In_reads_(TextBufferLength) const wchar_t* const pTextBuffer,
    //                                                 _In_ ULONG const TextBufferLength,
    //                                                 _In_ const COORD* const pTargetOrigin,
    //                                                 _Out_ ULONG* const pTextBufferRead) = 0;

    //HRESULT ReadConsoleOutputA(_In_ IConsoleOutputObject* const pOutContext,
    //                                   _Out_writes_(pTextBufferSize->X * pTextBufferSize->Y) CHAR_INFO* const pTextBuffer,
    //                                   _In_ const COORD* const pTextBufferSize,
    //                                   _In_ const COORD* const pTextBufferTargetOrigin,
    //                                   _In_ const SMALL_RECT* const pSourceRectangle,
    //                                   _Out_ SMALL_RECT* const pReadRectangle) = 0;

    //HRESULT ReadConsoleOutputW(_In_ IConsoleOutputObject* const pOutContext,
    //                                   _Out_writes_(pTextBufferSize->X * pTextBufferSize->Y) CHAR_INFO* const pTextBuffer,
    //                                   _In_ const COORD* const pTextBufferSize,
    //                                   _In_ const COORD* const pTextBufferTargetOrigin,
    //                                   _In_ const SMALL_RECT* const pSourceRectangle,
    //                                   _Out_ SMALL_RECT* const pReadRectangle) = 0;

    //HRESULT GetConsoleTitleAImpl(_Out_writes_(TextBufferSize) char* const pTextBuffer,
    //                                     _In_ ULONG const TextBufferSize) = 0;

    //HRESULT GetConsoleTitleWImpl(_Out_writes_(TextBufferSize) wchar_t* const pTextBuffer,
    //                                     _In_ ULONG const TextBufferSize) = 0;

    //HRESULT GetConsoleOriginalTitleAImpl(_Out_writes_(TextBufferSize) char* const pTextBuffer,
    //                                             _In_ ULONG const TextBufferSize) = 0;

    //HRESULT GetConsoleOriginalTitleWImpl(_Out_writes_(TextBufferSize) char* const pTextBuffer,
    //                                             _In_ ULONG const TextBufferSize) = 0;

    //HRESULT SetConsoleTitleAImpl(_In_reads_(TextBufferSize) char* const pTextBuffer,
    //                                     _In_ ULONG const TextBufferSize) = 0;

    //HRESULT SetConsoleTitleWImpl(_In_reads_(TextBufferSize) wchar_t* const pTextBuffer,
    //                                     _In_ ULONG const TextBufferSize) = 0;

#pragma endregion

#pragma region L3
    //HRESULT GetNumberOfConsoleMouseButtonsImpl(_Out_ ULONG* const pButtons) = 0;

    //HRESULT GetConsoleFontSizeImpl(_In_ IConsoleOutputObject* const pOutContext,
    //                                       _In_ DWORD const FontIndex,
    //                                       _Out_ COORD* const pFontSize) = 0;

    //// driver will pare down for non-Ex method
    //HRESULT GetCurrentConsoleFontExImpl(_In_ IConsoleOutputObject* const pOutContext,
    //                                            _In_ BOOLEAN const IsForMaximumWindowSize,
    //                                            _Out_ CONSOLE_FONT_INFOEX* const pConsoleFontInfoEx) = 0;

    //HRESULT SetConsoleDisplayModeImpl(_In_ IConsoleOutputObject* const pOutContext,
    //                                          _In_ ULONG const Flags,
    //                                          _Out_ COORD* const pNewScreenBufferSize) = 0;

    //HRESULT GetConsoleDisplayModeImpl(_In_ IConsoleOutputObject* const pOutContext,
    //                                          _Out_ ULONG* const pFlags) = 0;

    //HRESULT AddConsoleAliasAImpl(_In_reads_(SourceBufferLength) const char* const pSourceBuffer,
    //                                     _In_ ULONG const SourceBufferLength,
    //                                     _In_reads_(TargetBufferLength) const char* const pTargetBuffer,
    //                                     _In_ ULONG const TargetBufferLength,
    //                                     _In_reads_(ExeNameBufferLength) const char* const pExeNameBuffer,
    //                                     _In_ ULONG const ExeNameBufferLength) = 0;

    //HRESULT AddConsoleAliasWImpl(_In_reads_(SourceBufferLength) const wchar_t* const pSourceBuffer,
    //                                     _In_ ULONG const SourceBufferLength,
    //                                     _In_reads_(TargetBufferLength) const wchar_t* const pTargetBuffer,
    //                                     _In_ ULONG const TargetBufferLength,
    //                                     _In_reads_(ExeNameBufferLength) const wchar_t* const pExeNameBuffer,
    //                                     _In_ ULONG const ExeNameBufferLength) = 0;

    //HRESULT GetConsoleAliasAImpl(_In_reads_(SourceBufferLength) const char* const pSourceBuffer,
    //                                     _In_ ULONG const SourceBufferLength,
    //                                     _Out_writes_(TargetBufferLength) char* const pTargetBuffer,
    //                                     _In_ ULONG const TargetBufferLength,
    //                                     _In_reads_(ExeNameBufferLength) const char* const pExeNameBuffer,
    //                                     _In_ ULONG const ExeNameBufferLength) = 0;

    //HRESULT GetConsoleAliasWImpl(_In_reads_(SourceBufferLength) const wchar_t* const pSourceBuffer,
    //                                     _In_ ULONG const SourceBufferLength,
    //                                     _Out_writes_(TargetBufferLength) wchar_t* const pTargetBuffer,
    //                                     _In_ ULONG const TargetBufferLength,
    //                                     _In_reads_(ExeNameBufferLength) const wchar_t* const pExeNameBuffer,
    //                                     _In_ ULONG const ExeNameBufferLength) = 0;

    //HRESULT GetConsoleAliasesLengthAImpl(_In_reads_(ExeNameBufferLength) const char* const pExeNameBuffer,
    //                                             _In_ ULONG const ExeNameBufferLength,
    //                                             _Out_ ULONG* const pAliasesBufferRequired) = 0;

    //HRESULT GetConsoleAliasesLengthWImpl(_In_reads_(ExeNameBufferLength) const wchar_t* const pExeNameBuffer,
    //                                             _In_ ULONG const ExeNameBufferLength,
    //                                             _Out_ ULONG* const pAliasesBufferRequired) = 0;

    //HRESULT GetConsoleAliasExesLengthAImpl(_Out_ ULONG* const pAliasExesBufferRequired) = 0;

    //HRESULT GetConsoleAliasExesLengthWImpl(_Out_ ULONG* const pAliasExesBufferRequired) = 0;

    //HRESULT GetConsoleAliasesAImpl(_In_reads_(ExeNameBufferLength) const char* const pExeNameBuffer,
    //                                       _In_ ULONG const ExeNameBufferLength,
    //                                       _Out_writes_(AliasBufferLength) char* const pAliasBuffer,
    //                                       _In_ ULONG const AliasBufferLength) = 0;

    //HRESULT GetConsoleAliasesWImpl(_In_reads_(ExeNameBufferLength) const wchar_t* const pExeNameBuffer,
    //                                       _In_ ULONG const ExeNameBufferLength,
    //                                       _Out_writes_(AliasBufferLength) wchar_t* const pAliasBuffer,
    //                                       _In_ ULONG const AliasBufferLength) = 0;

    //HRESULT GetConsoleAliasExesAImpl(_Out_writes_(AliasExesBufferLength) char* const pAliasExesBuffer,
    //                                         _In_ ULONG const AliasExesBufferLength) = 0;

    //HRESULT GetConsoleAliasExesWImpl(_Out_writes_(AliasExesBufferLength) wchar_t* const pAliasExesBuffer,
    //                                         _In_ ULONG const AliasExesBufferLength) = 0;

    //HRESULT GetConsoleWindowImpl(_Out_ HWND* const pHwnd) = 0;

    //HRESULT GetConsoleSelectionInfoImpl(_Out_ CONSOLE_SELECTION_INFO* const pConsoleSelectionInfo) = 0;

    //HRESULT GetConsoleHistoryInfoImpl(_Out_ CONSOLE_HISTORY_INFO* const pConsoleHistoryInfo) = 0;

    //HRESULT SetConsoleHistoryInfoImpl(_In_ const CONSOLE_HISTORY_INFO* const pConsoleHistoryInfo) = 0;

    //HRESULT SetCurrentConsoleFontExImpl(_In_ IConsoleOutputObject* const pOutContext,
    //                                            _In_ BOOLEAN const IsForMaximumWindowSize,
    //                                            _In_ const CONSOLE_FONT_INFOEX* const pConsoleFontInfoEx) = 0;

#pragma endregion
};