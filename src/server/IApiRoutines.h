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

class SCREEN_INFORMATION;
typedef SCREEN_INFORMATION IConsoleOutputObject;

struct _INPUT_INFORMATION;
typedef _INPUT_INFORMATION INPUT_INFORMATION;
typedef INPUT_INFORMATION IConsoleInputObject;

class IApiRoutines
{
public:
    IApiRoutines()
    {

    }

#pragma region ObjectManagement
    /*virtual HRESULT CreateInitialObjects(_Out_ IConsoleInputObject** const ppInputObject,
                                          _Out_ IConsoleOutputObject** const ppOutputObject) = 0;
*/

#pragma endregion

#pragma region L1
    virtual HRESULT GetConsoleInputCodePageImpl(_Out_ ULONG* const pCodePage) = 0;

    virtual HRESULT GetConsoleOutputCodePageImpl(_Out_ ULONG* const pCodePage) = 0;

    virtual HRESULT GetConsoleInputModeImpl(_In_ IConsoleInputObject* const pInContext,
                                             _Out_ ULONG* const pMode) = 0;

    virtual HRESULT GetConsoleOutputModeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                              _Out_ ULONG* const pMode) = 0;

    virtual HRESULT SetConsoleInputModeImpl(_In_ IConsoleInputObject* const pInContext,
                                             _In_ ULONG const Mode) = 0;

    virtual HRESULT SetConsoleOutputModeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                              _In_ ULONG const Mode) = 0;

    virtual HRESULT GetNumberOfConsoleInputEventsImpl(_In_ IConsoleInputObject* const pInContext,
                                                       _Out_ ULONG* const pEvents) = 0;

    virtual HRESULT PeekConsoleInputAImpl(_In_ IConsoleInputObject* const pInContext,
                                           _Out_writes_to_(InputRecordsBufferLength, *pRecordsWritten) INPUT_RECORD* const pInputRecordsBuffer,
                                           _In_ ULONG const InputRecordsBufferLength,
                                           _Out_ ULONG* const pRecordsWritten) = 0;

    virtual HRESULT PeekConsoleInputWImpl(_In_ IConsoleInputObject* const pInContext,
                                           _Out_writes_to_(InputRecordsBufferLength, *pRecordsWritten) INPUT_RECORD* const pInputRecordsBuffer,
                                           _In_ ULONG const InputRecordsBufferLength,
                                           _Out_ ULONG* const pRecordsWritten) = 0;

    virtual HRESULT ReadConsoleInputAImpl(_In_ IConsoleInputObject* const pInContext,
                                           _Out_writes_to_(InputRecordsBufferLength, *pRecordsWritten) INPUT_RECORD* const pInputRecordsBuffer,
                                           _In_ ULONG const InputRecordsBufferLength,
                                           _Out_ ULONG* const pRecordsWritten) = 0;

    virtual HRESULT ReadConsoleInputWImpl(_In_ IConsoleInputObject* const pInContext,
                                           _Out_writes_to_(InputRecordsBufferLength, *pRecordsWritten) INPUT_RECORD* const pInputRecordsBuffer,
                                           _In_ ULONG const InputRecordsBufferLength,
                                           _Out_ ULONG* const pRecordsWritten) = 0;

    virtual HRESULT ReadConsoleAImpl(_In_ IConsoleInputObject* const pInContext,
                                      _Out_writes_to_(TextBufferLength, *pTextBufferWritten) char* const pTextBuffer,
                                      _In_ ULONG const TextBufferLength,
                                      _Out_ ULONG* const pTextBufferWritten,
                                      _In_opt_ CONSOLE_READCONSOLE_CONTROL* const pReadControl) = 0;

    virtual HRESULT ReadConsoleWImpl(_In_ IConsoleInputObject* const pInContext,
                                      _Out_writes_to_(TextBufferLength, *pTextBufferWritten) wchar_t* const pTextBuffer,
                                      _In_ ULONG const TextBufferLength,
                                      _Out_ ULONG* const pTextBufferWritten,
                                      _In_opt_ CONSOLE_READCONSOLE_CONTROL* const pReadControl) = 0;

    virtual HRESULT WriteConsoleAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                       _In_reads_(TextBufferLength) char* const pTextBuffer,
                                       _In_ ULONG const TextBufferLength,
                                       _Out_ ULONG* const pTextBufferRead) = 0;

    virtual HRESULT WriteConsoleWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                       _In_reads_(TextBufferLength) wchar_t* const pTextBuffer,
                                       _In_ ULONG const TextBufferLength,
                                       _Out_ ULONG* const pTextBufferRead) = 0;

#pragma endregion

#pragma region L2

    virtual HRESULT FillConsoleOutputAttributeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                    _In_ WORD const Attribute,
                                                    _In_ DWORD const LengthToWrite,
                                                    _In_ COORD const StartingCoordinate,
                                                    _Out_ DWORD* const pCellsModified) = 0;

    virtual HRESULT FillConsoleOutputCharacterAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                     _In_ char const Character,
                                                     _In_ DWORD const LengthToWrite,
                                                     _In_ COORD const StartingCoordinate,
                                                     _Out_ DWORD* const pCellsModified) = 0;

    virtual HRESULT FillConsoleOutputCharacterWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                     _In_ wchar_t const Character,
                                                     _In_ DWORD const LengthToWrite,
                                                     _In_ COORD const StartingCoordinate,
                                                     _Out_ DWORD* const pCellsModified) = 0;

    // Process based. Restrict in protocol side?
    virtual HRESULT GenerateConsoleCtrlEventImpl(_In_ ULONG const ProcessGroupFilter,
                                                  _In_ ULONG const ControlEvent) = 0;

    virtual HRESULT SetConsoleActiveScreenBufferImpl(_In_ HANDLE const NewOutContext) = 0;

    virtual HRESULT FlushConsoleInputBuffer(_In_ IConsoleInputObject* const pInContext) = 0;

    virtual HRESULT SetConsoleInputCodePageImpl(_In_ ULONG const CodePage) = 0;

    virtual HRESULT SetConsoleOutputCodePageImpl(_In_ ULONG const CodePage) = 0;

    virtual HRESULT GetConsoleCursorInfoImpl(_In_ IConsoleOutputObject* const pOutContext,
                                              _Out_ ULONG* const pCursorSize,
                                              _Out_ BOOLEAN* const pIsVisible) = 0;

    virtual HRESULT SetConsoleCursorInfoImpl(_In_ IConsoleOutputObject* const pOutContext,
                                              _In_ ULONG const CursorSize,
                                              _In_ BOOLEAN const IsVisible) = 0;

    // driver will pare down for non-Ex method
    virtual HRESULT GetConsoleScreenBufferInfoExImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                      _Out_ CONSOLE_SCREEN_BUFFER_INFOEX* const pScreenBufferInfoEx) = 0;

    virtual HRESULT SetConsoleScreenBufferInfoExImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                      _In_ const CONSOLE_SCREEN_BUFFER_INFOEX* const pScreenBufferInfoEx) = 0;

    virtual HRESULT SetConsoleScreenBufferSizeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                    _In_ const COORD* const pSize) = 0;

    virtual HRESULT SetConsoleCursorPositionImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                  _In_ const COORD* const pCursorPosition) = 0;

    virtual HRESULT GetLargestConsoleWindowSizeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                     _Out_ COORD* const pSize) = 0;

    virtual HRESULT ScrollConsoleScreenBufferAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                    _In_ const SMALL_RECT* const pSourceRectangle,
                                                    _In_ const COORD* const pTargetOrigin,
                                                    _In_opt_ const SMALL_RECT* const pTargetClipRectangle,
                                                    _In_ const CHAR_INFO* const pFill) = 0;

    virtual HRESULT ScrollConsoleScreenBufferWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                    _In_ const SMALL_RECT* const pSourceRectangle,
                                                    _In_ const COORD* const pTargetOrigin,
                                                    _In_opt_ const SMALL_RECT* const pTargetClipRectangle,
                                                    _In_ const CHAR_INFO* const pFill) = 0;

    virtual HRESULT SetConsoleTextAttributeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                 _In_ WORD const Attribute) = 0;

    virtual HRESULT SetConsoleWindowInfoImpl(_In_ IConsoleOutputObject* const pOutContext,
                                              _In_ BOOLEAN const IsAbsoluteRectangle,
                                              _In_ const SMALL_RECT* const pWindowRectangle) = 0;

    virtual HRESULT ReadConsoleOutputAttributeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                    _In_ const COORD* const pSourceOrigin,
                                                    _Out_writes_to_(AttributeBufferLength, *pAttributeBufferWritten) WORD* const pAttributeBuffer,
                                                    _In_ ULONG const AttributeBufferLength,
                                                    _Out_ ULONG* const pAttributeBufferWritten) = 0;

    virtual HRESULT ReadConsoleOutputCharacterAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                     _In_ const COORD* const pSourceOrigin,
                                                     _Out_writes_to_(TextBufferLength, *pTextBufferWritten) char* const pTextBuffer,
                                                     _In_ ULONG const pTextBufferLength,
                                                     _Out_ ULONG* const pTextBufferWritten) = 0;

    virtual HRESULT ReadConsoleOutputCharacterWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                     _In_ const COORD* const pSourceOrigin,
                                                     _Out_writes_to_(TextBufferLength, *pTextBufferWritten) wchar_t* const pTextBuffer,
                                                     _In_ ULONG const TextBufferLength,
                                                     _Out_ ULONG* const pTextBufferWritten) = 0;

    virtual HRESULT WriteConsoleInputAImpl(_In_ IConsoleInputObject* const pInContext,
                                            _In_reads_(InputBufferLength) const INPUT_RECORD* const pInputBuffer,
                                            _In_ ULONG const InputBufferLength,
                                            _Out_ ULONG* const pInputBufferRead) = 0;

    virtual HRESULT WriteConsoleInputWImpl(_In_ IConsoleInputObject* const pInContext,
                                            _In_reads_(InputBufferLength) const INPUT_RECORD* const pInputBuffer,
                                            _In_ ULONG const InputBufferLength,
                                            _Out_ ULONG* const pInputBufferRead) = 0;

    virtual HRESULT WriteConsoleOutputAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                             _In_reads_(pTextBufferSize->X * pTextBufferSize->Y) const CHAR_INFO* const pTextBuffer,
                                             _In_ const COORD* const pTextBufferSize,
                                             _In_ const COORD* const pTextBufferSourceOrigin,
                                             _In_ const SMALL_RECT* const pTargetRectangle,
                                             _Out_ SMALL_RECT* const pAffectedRectangle) = 0;

    virtual HRESULT WriteConsoleOutputWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                             _In_reads_(pTextBufferSize->X * pTextBufferSize->Y) const CHAR_INFO* const pTextBuffer,
                                             _In_ const COORD* const pTextBufferSize,
                                             _In_ const COORD* const pTextBufferSourceOrigin,
                                             _In_ const SMALL_RECT* const pTargetRectangle,
                                             _Out_ SMALL_RECT* const pAffectedRectangle) = 0;

    virtual HRESULT WriteConsoleOutputAttributeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                     _In_reads_(AttributeBufferLength) const WORD* const pAttributeBuffer,
                                                     _In_ ULONG const AttributeBufferLength,
                                                     _In_ const COORD* const pTargetOrigin,
                                                     _Out_ ULONG* const pAttributeBufferRead) = 0;

    virtual HRESULT WriteConsoleOutputCharacterAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                      _In_reads_(TextBufferLength) const char* const pTextBuffer,
                                                      _In_ ULONG const TextBufferLength,
                                                      _In_ const COORD* const pTargetOrigin,
                                                      _Out_ ULONG* const pTextBufferRead) = 0;

    virtual HRESULT WriteConsoleOutputCharacterWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                      _In_reads_(TextBufferLength) const wchar_t* const pTextBuffer,
                                                      _In_ ULONG const TextBufferLength,
                                                      _In_ const COORD* const pTargetOrigin,
                                                      _Out_ ULONG* const pTextBufferRead) = 0;

    virtual HRESULT ReadConsoleOutputA(_In_ IConsoleOutputObject* const pOutContext,
                                        _Out_writes_(pTextBufferSize->X * pTextBufferSize->Y) CHAR_INFO* const pTextBuffer,
                                        _In_ const COORD* const pTextBufferSize,
                                        _In_ const COORD* const pTextBufferTargetOrigin,
                                        _In_ const SMALL_RECT* const pSourceRectangle,
                                        _Out_ SMALL_RECT* const pReadRectangle) = 0;

    virtual HRESULT ReadConsoleOutputW(_In_ IConsoleOutputObject* const pOutContext,
                                        _Out_writes_(pTextBufferSize->X * pTextBufferSize->Y) CHAR_INFO* const pTextBuffer,
                                        _In_ const COORD* const pTextBufferSize,
                                        _In_ const COORD* const pTextBufferTargetOrigin,
                                        _In_ const SMALL_RECT* const pSourceRectangle,
                                        _Out_ SMALL_RECT* const pReadRectangle) = 0;

    virtual HRESULT GetConsoleTitleAImpl(_Out_writes_(TextBufferSize) char* const pTextBuffer,
                                          _In_ ULONG const TextBufferSize) = 0;

    virtual HRESULT GetConsoleTitleWImpl(_Out_writes_(TextBufferSize) wchar_t* const pTextBuffer,
                                          _In_ ULONG const TextBufferSize) = 0;

    virtual HRESULT GetConsoleOriginalTitleAImpl(_Out_writes_(TextBufferSize) char* const pTextBuffer,
                                                  _In_ ULONG const TextBufferSize) = 0;

    virtual HRESULT GetConsoleOriginalTitleWImpl(_Out_writes_(TextBufferSize) char* const pTextBuffer,
                                                  _In_ ULONG const TextBufferSize) = 0;

    virtual HRESULT SetConsoleTitleAImpl(_In_reads_(TextBufferSize) char* const pTextBuffer,
                                          _In_ ULONG const TextBufferSize) = 0;

    virtual HRESULT SetConsoleTitleWImpl(_In_reads_(TextBufferSize) wchar_t* const pTextBuffer,
                                          _In_ ULONG const TextBufferSize) = 0;

#pragma endregion

#pragma region L3
    virtual HRESULT GetNumberOfConsoleMouseButtonsImpl(_Out_ ULONG* const pButtons) = 0;

    virtual HRESULT GetConsoleFontSizeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                            _In_ DWORD const FontIndex,
                                            _Out_ COORD* const pFontSize) = 0;

    // driver will pare down for non-Ex method
    virtual HRESULT GetCurrentConsoleFontExImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                 _In_ BOOLEAN const IsForMaximumWindowSize,
                                                 _Out_ CONSOLE_FONT_INFOEX* const pConsoleFontInfoEx) = 0;

    virtual HRESULT SetConsoleDisplayModeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                               _In_ ULONG const Flags,
                                               _Out_ COORD* const pNewScreenBufferSize) = 0;

    virtual HRESULT GetConsoleDisplayModeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                               _Out_ ULONG* const pFlags) = 0;

    virtual HRESULT AddConsoleAliasAImpl(_In_reads_(SourceBufferLength) const char* const pSourceBuffer,
                                          _In_ ULONG const SourceBufferLength,
                                          _In_reads_(TargetBufferLength) const char* const pTargetBuffer,
                                          _In_ ULONG const TargetBufferLength,
                                          _In_reads_(ExeNameBufferLength) const char* const pExeNameBuffer,
                                          _In_ ULONG const ExeNameBufferLength) = 0;

    virtual HRESULT AddConsoleAliasWImpl(_In_reads_(SourceBufferLength) const wchar_t* const pSourceBuffer,
                                          _In_ ULONG const SourceBufferLength,
                                          _In_reads_(TargetBufferLength) const wchar_t* const pTargetBuffer,
                                          _In_ ULONG const TargetBufferLength,
                                          _In_reads_(ExeNameBufferLength) const wchar_t* const pExeNameBuffer,
                                          _In_ ULONG const ExeNameBufferLength) = 0;

    virtual HRESULT GetConsoleAliasAImpl(_In_reads_(SourceBufferLength) const char* const pSourceBuffer,
                                          _In_ ULONG const SourceBufferLength,
                                          _Out_writes_(TargetBufferLength) char* const pTargetBuffer,
                                          _In_ ULONG const TargetBufferLength,
                                          _In_reads_(ExeNameBufferLength) const char* const pExeNameBuffer,
                                          _In_ ULONG const ExeNameBufferLength) = 0;

    virtual HRESULT GetConsoleAliasWImpl(_In_reads_(SourceBufferLength) const wchar_t* const pSourceBuffer,
                                          _In_ ULONG const SourceBufferLength,
                                          _Out_writes_(TargetBufferLength) wchar_t* const pTargetBuffer,
                                          _In_ ULONG const TargetBufferLength,
                                          _In_reads_(ExeNameBufferLength) const wchar_t* const pExeNameBuffer,
                                          _In_ ULONG const ExeNameBufferLength) = 0;

    virtual HRESULT GetConsoleAliasesLengthAImpl(_In_reads_(ExeNameBufferLength) const char* const pExeNameBuffer,
                                                  _In_ ULONG const ExeNameBufferLength,
                                                  _Out_ ULONG* const pAliasesBufferRequired) = 0;

    virtual HRESULT GetConsoleAliasesLengthWImpl(_In_reads_(ExeNameBufferLength) const wchar_t* const pExeNameBuffer,
                                                  _In_ ULONG const ExeNameBufferLength,
                                                  _Out_ ULONG* const pAliasesBufferRequired) = 0;

    virtual HRESULT GetConsoleAliasExesLengthAImpl(_Out_ ULONG* const pAliasExesBufferRequired) = 0;

    virtual HRESULT GetConsoleAliasExesLengthWImpl(_Out_ ULONG* const pAliasExesBufferRequired) = 0;

    virtual HRESULT GetConsoleAliasesAImpl(_In_reads_(ExeNameBufferLength) const char* const pExeNameBuffer,
                                            _In_ ULONG const ExeNameBufferLength,
                                            _Out_writes_(AliasBufferLength) char* const pAliasBuffer,
                                            _In_ ULONG const AliasBufferLength) = 0;

    virtual HRESULT GetConsoleAliasesWImpl(_In_reads_(ExeNameBufferLength) const wchar_t* const pExeNameBuffer,
                                            _In_ ULONG const ExeNameBufferLength,
                                            _Out_writes_(AliasBufferLength) wchar_t* const pAliasBuffer,
                                            _In_ ULONG const AliasBufferLength) = 0;

    virtual HRESULT GetConsoleAliasExesAImpl(_Out_writes_(AliasExesBufferLength) char* const pAliasExesBuffer,
                                              _In_ ULONG const AliasExesBufferLength) = 0;

    virtual HRESULT GetConsoleAliasExesWImpl(_Out_writes_(AliasExesBufferLength) wchar_t* const pAliasExesBuffer,
                                              _In_ ULONG const AliasExesBufferLength) = 0;

    virtual HRESULT GetConsoleWindowImpl(_Out_ HWND* const pHwnd) = 0;

    virtual HRESULT GetConsoleSelectionInfoImpl(_Out_ CONSOLE_SELECTION_INFO* const pConsoleSelectionInfo) = 0;

    virtual HRESULT GetConsoleHistoryInfoImpl(_Out_ CONSOLE_HISTORY_INFO* const pConsoleHistoryInfo) = 0;

    virtual HRESULT SetConsoleHistoryInfoImpl(_In_ const CONSOLE_HISTORY_INFO* const pConsoleHistoryInfo) = 0;

    virtual HRESULT SetCurrentConsoleFontExImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                 _In_ BOOLEAN const IsForMaximumWindowSize,
                                                 _In_ const CONSOLE_FONT_INFOEX* const pConsoleFontInfoEx) = 0;

#pragma endregion
};