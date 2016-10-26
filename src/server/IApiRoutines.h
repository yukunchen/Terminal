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
                                          _Out_ IConsoleOutputObject** const ppOutputObject);
*/

#pragma endregion

#pragma region L1
    virtual HRESULT GetConsoleInputCodePageImpl(_Out_ ULONG* const pCodePage);

    virtual HRESULT GetConsoleOutputCodePageImpl(_Out_ ULONG* const pCodePage);

    virtual HRESULT GetConsoleInputModeImpl(_In_ IConsoleInputObject* const pInContext,
                                            _Out_ ULONG* const pMode);

    virtual HRESULT GetConsoleOutputModeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                             _Out_ ULONG* const pMode);

    virtual HRESULT SetConsoleInputModeImpl(_In_ IConsoleInputObject* const pInContext,
                                            _In_ ULONG const Mode);

    virtual HRESULT SetConsoleOutputModeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                             _In_ ULONG const Mode);

    virtual HRESULT GetNumberOfConsoleInputEventsImpl(_In_ IConsoleInputObject* const pInContext,
                                                      _Out_ ULONG* const pEvents);

    virtual HRESULT PeekConsoleInputAImpl(_In_ IConsoleInputObject* const pInContext,
                                          _Out_writes_to_(InputRecordsBufferLength, *pRecordsWritten) INPUT_RECORD* const pInputRecordsBuffer,
                                          _In_ ULONG const InputRecordsBufferLength,
                                          _Out_ ULONG* const pRecordsWritten);

    virtual HRESULT PeekConsoleInputWImpl(_In_ IConsoleInputObject* const pInContext,
                                          _Out_writes_to_(InputRecordsBufferLength, *pRecordsWritten) INPUT_RECORD* const pInputRecordsBuffer,
                                          _In_ ULONG const InputRecordsBufferLength,
                                          _Out_ ULONG* const pRecordsWritten);

    virtual HRESULT ReadConsoleInputAImpl(_In_ IConsoleInputObject* const pInContext,
                                          _Out_writes_to_(InputRecordsBufferLength, *pRecordsWritten) INPUT_RECORD* const pInputRecordsBuffer,
                                          _In_ ULONG const InputRecordsBufferLength,
                                          _Out_ ULONG* const pRecordsWritten);

    virtual HRESULT ReadConsoleInputWImpl(_In_ IConsoleInputObject* const pInContext,
                                          _Out_writes_to_(InputRecordsBufferLength, *pRecordsWritten) INPUT_RECORD* const pInputRecordsBuffer,
                                          _In_ ULONG const InputRecordsBufferLength,
                                          _Out_ ULONG* const pRecordsWritten);

    virtual HRESULT ReadConsoleAImpl(_In_ IConsoleInputObject* const pInContext,
                                     _Out_writes_to_(TextBufferLength, *pTextBufferWritten) char* const pTextBuffer,
                                     _In_ ULONG const TextBufferLength,
                                     _Out_ ULONG* const pTextBufferWritten,
                                     _In_opt_ CONSOLE_READCONSOLE_CONTROL* const pReadControl);

    virtual HRESULT ReadConsoleWImpl(_In_ IConsoleInputObject* const pInContext,
                                     _Out_writes_to_(TextBufferLength, *pTextBufferWritten) wchar_t* const pTextBuffer,
                                     _In_ ULONG const TextBufferLength,
                                     _Out_ ULONG* const pTextBufferWritten,
                                     _In_opt_ CONSOLE_READCONSOLE_CONTROL* const pReadControl);

    virtual HRESULT WriteConsoleAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                      _In_reads_(TextBufferLength) char* const pTextBuffer,
                                      _In_ ULONG const TextBufferLength,
                                      _Out_ ULONG* const pTextBufferRead);

    virtual HRESULT WriteConsoleWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                      _In_reads_(TextBufferLength) wchar_t* const pTextBuffer,
                                      _In_ ULONG const TextBufferLength,
                                      _Out_ ULONG* const pTextBufferRead);

#pragma endregion

#pragma region L2

    virtual HRESULT FillConsoleOutputAttributeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                   _In_ WORD const Attribute,
                                                   _In_ DWORD const LengthToWrite,
                                                   _In_ COORD const StartingCoordinate,
                                                   _Out_ DWORD* const pCellsModified);

    virtual HRESULT FillConsoleOutputCharacterAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                    _In_ char const Character,
                                                    _In_ DWORD const LengthToWrite,
                                                    _In_ COORD const StartingCoordinate,
                                                    _Out_ DWORD* const pCellsModified);

    virtual HRESULT FillConsoleOutputCharacterWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                    _In_ wchar_t const Character,
                                                    _In_ DWORD const LengthToWrite,
                                                    _In_ COORD const StartingCoordinate,
                                                    _Out_ DWORD* const pCellsModified);

    // Process based. Restrict in protocol side?
    virtual HRESULT GenerateConsoleCtrlEventImpl(_In_ ULONG const ProcessGroupFilter,
                                                 _In_ ULONG const ControlEvent);

    virtual HRESULT SetConsoleActiveScreenBufferImpl(_In_ HANDLE const NewOutContext);

    virtual HRESULT FlushConsoleInputBuffer(_In_ IConsoleInputObject* const pInContext);

    virtual HRESULT SetConsoleInputCodePageImpl(_In_ ULONG const CodePage);

    virtual HRESULT SetConsoleOutputCodePageImpl(_In_ ULONG const CodePage);

    virtual HRESULT GetConsoleCursorInfoImpl(_In_ IConsoleOutputObject* const pOutContext,
                                             _Out_ ULONG* const pCursorSize,
                                             _Out_ BOOLEAN* const pIsVisible);

    virtual HRESULT SetConsoleCursorInfoImpl(_In_ IConsoleOutputObject* const pOutContext,
                                             _In_ ULONG const CursorSize,
                                             _In_ BOOLEAN const IsVisible);

    // driver will pare down for non-Ex method
    virtual HRESULT GetConsoleScreenBufferInfoExImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                     _Out_ CONSOLE_SCREEN_BUFFER_INFOEX* const pScreenBufferInfoEx);

    virtual HRESULT SetConsoleScreenBufferInfoExImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                     _In_ const CONSOLE_SCREEN_BUFFER_INFOEX* const pScreenBufferInfoEx);

    virtual HRESULT SetConsoleScreenBufferSizeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                   _In_ const COORD* const pSize);

    virtual HRESULT SetConsoleCursorPositionImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                 _In_ const COORD* const pCursorPosition);

    virtual HRESULT GetLargestConsoleWindowSizeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                    _Out_ COORD* const pSize);

    virtual HRESULT ScrollConsoleScreenBufferAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                   _In_ const SMALL_RECT* const pSourceRectangle,
                                                   _In_ const COORD* const pTargetOrigin,
                                                   _In_opt_ const SMALL_RECT* const pTargetClipRectangle,
                                                   _In_ char const chFill,
                                                   _In_ WORD const attrFill);

    virtual HRESULT ScrollConsoleScreenBufferWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                   _In_ const SMALL_RECT* const pSourceRectangle,
                                                   _In_ const COORD* const pTargetOrigin,
                                                   _In_opt_ const SMALL_RECT* const pTargetClipRectangle,
                                                   _In_ wchar_t const wchFill,
                                                   _In_ WORD const attrFill);

    virtual HRESULT SetConsoleTextAttributeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                _In_ WORD const Attribute);

    virtual HRESULT SetConsoleWindowInfoImpl(_In_ IConsoleOutputObject* const pOutContext,
                                             _In_ BOOLEAN const IsAbsoluteRectangle,
                                             _In_ const SMALL_RECT* const pWindowRectangle);

    virtual HRESULT ReadConsoleOutputAttributeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                   _In_ const COORD* const pSourceOrigin,
                                                   _Out_writes_to_(AttributeBufferLength, *pAttributeBufferWritten) WORD* const pAttributeBuffer,
                                                   _In_ ULONG const AttributeBufferLength,
                                                   _Out_ ULONG* const pAttributeBufferWritten);

    virtual HRESULT ReadConsoleOutputCharacterAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                    _In_ const COORD* const pSourceOrigin,
                                                    _Out_writes_to_(TextBufferLength, *pTextBufferWritten) char* const pTextBuffer,
                                                    _In_ ULONG const pTextBufferLength,
                                                    _Out_ ULONG* const pTextBufferWritten);

    virtual HRESULT ReadConsoleOutputCharacterWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                    _In_ const COORD* const pSourceOrigin,
                                                    _Out_writes_to_(TextBufferLength, *pTextBufferWritten) wchar_t* const pTextBuffer,
                                                    _In_ ULONG const TextBufferLength,
                                                    _Out_ ULONG* const pTextBufferWritten);

    virtual HRESULT WriteConsoleInputAImpl(_In_ IConsoleInputObject* const pInContext,
                                           _In_reads_(InputBufferLength) const INPUT_RECORD* const pInputBuffer,
                                           _In_ ULONG const InputBufferLength,
                                           _Out_ ULONG* const pInputBufferRead);

    virtual HRESULT WriteConsoleInputWImpl(_In_ IConsoleInputObject* const pInContext,
                                           _In_reads_(InputBufferLength) const INPUT_RECORD* const pInputBuffer,
                                           _In_ ULONG const InputBufferLength,
                                           _Out_ ULONG* const pInputBufferRead);

    virtual HRESULT WriteConsoleOutputAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                            _In_reads_(pTextBufferSize->X * pTextBufferSize->Y) const CHAR_INFO* const pTextBuffer,
                                            _In_ const COORD* const pTextBufferSize,
                                            _In_ const COORD* const pTextBufferSourceOrigin,
                                            _In_ const SMALL_RECT* const pTargetRectangle,
                                            _Out_ SMALL_RECT* const pAffectedRectangle);

    virtual HRESULT WriteConsoleOutputWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                            _In_reads_(pTextBufferSize->X * pTextBufferSize->Y) const CHAR_INFO* const pTextBuffer,
                                            _In_ const COORD* const pTextBufferSize,
                                            _In_ const COORD* const pTextBufferSourceOrigin,
                                            _In_ const SMALL_RECT* const pTargetRectangle,
                                            _Out_ SMALL_RECT* const pAffectedRectangle);

    virtual HRESULT WriteConsoleOutputAttributeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                    _In_reads_(AttributeBufferLength) const WORD* const pAttributeBuffer,
                                                    _In_ ULONG const AttributeBufferLength,
                                                    _In_ const COORD* const pTargetOrigin,
                                                    _Out_ ULONG* const pAttributeBufferRead);

    virtual HRESULT WriteConsoleOutputCharacterAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                     _In_reads_(TextBufferLength) const char* const pTextBuffer,
                                                     _In_ ULONG const TextBufferLength,
                                                     _In_ const COORD* const pTargetOrigin,
                                                     _Out_ ULONG* const pTextBufferRead);

    virtual HRESULT WriteConsoleOutputCharacterWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                     _In_reads_(TextBufferLength) const wchar_t* const pTextBuffer,
                                                     _In_ ULONG const TextBufferLength,
                                                     _In_ const COORD* const pTargetOrigin,
                                                     _Out_ ULONG* const pTextBufferRead);

    virtual HRESULT ReadConsoleOutputA(_In_ IConsoleOutputObject* const pOutContext,
                                       _Out_writes_(pTextBufferSize->X * pTextBufferSize->Y) CHAR_INFO* const pTextBuffer,
                                       _In_ const COORD* const pTextBufferSize,
                                       _In_ const COORD* const pTextBufferTargetOrigin,
                                       _In_ const SMALL_RECT* const pSourceRectangle,
                                       _Out_ SMALL_RECT* const pReadRectangle);

    virtual HRESULT ReadConsoleOutputW(_In_ IConsoleOutputObject* const pOutContext,
                                       _Out_writes_(pTextBufferSize->X * pTextBufferSize->Y) CHAR_INFO* const pTextBuffer,
                                       _In_ const COORD* const pTextBufferSize,
                                       _In_ const COORD* const pTextBufferTargetOrigin,
                                       _In_ const SMALL_RECT* const pSourceRectangle,
                                       _Out_ SMALL_RECT* const pReadRectangle);

    virtual HRESULT GetConsoleTitleAImpl(_Out_writes_(*pcchTitleBufferSize) char* const psTitleBuffer,
                                         _Inout_ ULONG* const pcchTitleBufferSize) = 0;

    virtual HRESULT GetConsoleTitleWImpl(_Out_writes_(*pcchTitleBufferSize) wchar_t* const pwsTitleBuffer,
                                         _Inout_ ULONG* const pcchTitleBufferSize) = 0;

    virtual HRESULT GetConsoleOriginalTitleAImpl(_Out_writes_(*pcchTitleBufferSize) char* const psTitleBuffer,
                                                 _Inout_ ULONG* const pcchTitleBufferSize) = 0;

    virtual HRESULT GetConsoleOriginalTitleWImpl(_Out_writes_(*pcchTitleBufferSize) wchar_t* const pwsTitleBuffer,
                                                 _Inout_ ULONG* const pcchTitleBufferSize) = 0;

    virtual HRESULT SetConsoleTitleAImpl(_In_reads_bytes_(cbTitleBufferSize) char* const psTitleBuffer,
                                         _In_ ULONG const cbTitleBufferSize);

    virtual HRESULT SetConsoleTitleWImpl(_In_reads_bytes_(cbTitleBufferSize) wchar_t* const pwsTitleBuffer,
                                         _In_ ULONG const cbTitleBufferSize);

#pragma endregion

#pragma region L3
    virtual HRESULT GetNumberOfConsoleMouseButtonsImpl(_Out_ ULONG* const pButtons);

    virtual HRESULT GetConsoleFontSizeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                           _In_ DWORD const FontIndex,
                                           _Out_ COORD* const pFontSize);

    // driver will pare down for non-Ex method
    virtual HRESULT GetCurrentConsoleFontExImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                _In_ BOOLEAN const IsForMaximumWindowSize,
                                                _Out_ CONSOLE_FONT_INFOEX* const pConsoleFontInfoEx);

    virtual HRESULT SetConsoleDisplayModeImpl(_In_ SCREEN_INFORMATION* const pContext,
                                              _In_ ULONG const Flags,
                                              _Out_ COORD* const pNewScreenBufferSize);

    virtual HRESULT GetConsoleDisplayModeImpl(_In_ SCREEN_INFORMATION* const pContext,
                                              _Out_ ULONG* const pFlags);

    virtual HRESULT AddConsoleAliasAImpl(_In_reads_bytes_(cbSourceBufferLength) const char* const psSourceBuffer,
                                         _In_ ULONG const cbSourceBufferLength,
                                         _In_reads_bytes_(cbTargetBufferLength) const char* const psTargetBuffer,
                                         _In_ ULONG const cbTargetBufferLength,
                                         _In_reads_bytes_(cbExeNameBufferLength) const char* const psExeNameBuffer,
                                         _In_ ULONG const cbExeNameBufferLength) = 0;

    virtual HRESULT AddConsoleAliasWImpl(_In_reads_bytes_(cbSourceBufferLength) const wchar_t* const pwsSourceBuffer,
                                         _In_ ULONG const cbSourceBufferLength,
                                         _In_reads_bytes_(cbTargetBufferLength) const wchar_t* const pwsTargetBuffer,
                                         _In_ ULONG const cbTargetBufferLength,
                                         _In_reads_bytes_(cbExeNameBufferLength) const wchar_t* const pwsExeNameBuffer,
                                         _In_ ULONG const cbExeNameBufferLength) = 0;

    virtual HRESULT GetConsoleAliasAImpl(_In_reads_bytes_(cbSourceBufferLength) const char* const psSourceBuffer,
                                         _In_ ULONG const cbSourceBufferLength,
                                         _Out_writes_bytes_(*pcbTargetBufferLength) char* const psTargetBuffer,
                                         _Inout_ ULONG* const pcbTargetBufferLength,
                                         _In_reads_bytes_(cbExeNameBufferLength) const char* const psExeNameBuffer,
                                         _In_ ULONG const cbExeNameBufferLength) = 0;

    virtual HRESULT GetConsoleAliasWImpl(_In_reads_bytes_(cbSourceBufferLength) const wchar_t* const pwsSourceBuffer,
                                         _In_ ULONG const cbSourceBufferLength,
                                         _Out_writes_bytes_(*pcbTargetBufferLength) wchar_t* const pwsTargetBuffer,
                                         _Inout_ ULONG* const pcbTargetBufferLength,
                                         _In_reads_bytes_(cbExeNameBufferLength) const wchar_t* const pwsExeNameBuffer,
                                         _In_ ULONG const cbExeNameBufferLength) = 0;

    virtual HRESULT GetConsoleAliasesLengthAImpl(_In_reads_bytes_(cbExeNameBufferLength) const char* const psExeNameBuffer,
                                                 _In_ ULONG const cbExeNameBufferLength,
                                                 _Out_ ULONG* const pcbAliasesBufferRequired);

    virtual HRESULT GetConsoleAliasesLengthWImpl(_In_reads_bytes_(cbExeNameBufferLength) const wchar_t* const pwsExeNameBuffer,
                                                 _In_ ULONG const cbExeNameBufferLength,
                                                 _Out_ ULONG* const pcbAliasesBufferRequired);

    virtual HRESULT GetConsoleAliasExesLengthAImpl(_Out_ ULONG* const pcbAliasExesBufferRequired);

    virtual HRESULT GetConsoleAliasExesLengthWImpl(_Out_ ULONG* const pcbAliasExesBufferRequired);

    virtual HRESULT GetConsoleAliasesAImpl(_In_reads_bytes_(cbExeNameBufferLength) const char* const psExeNameBuffer,
                                           _In_ ULONG const cbExeNameBufferLength,
                                           _Out_writes_bytes_(*pcbAliasBufferLength) char* const psAliasBuffer,
                                           _Inout_ ULONG* const pcbAliasBufferLength) = 0;

    virtual HRESULT GetConsoleAliasesWImpl(_In_reads_(cbExeNameBufferLength) const wchar_t* const pwsExeNameBuffer,
                                           _In_ ULONG const cbExeNameBufferLength,
                                           _Out_writes_bytes_(*pcbAliasBufferLength) wchar_t* const pwsAliasBuffer,
                                           _Inout_ ULONG* const pcbAliasBufferLength) = 0;

    virtual HRESULT GetConsoleAliasExesAImpl(_Out_writes_bytes_(*pcbAliasExesBufferLength) char* const psAliasExesBuffer,
                                             _Inout_ ULONG* const pcbAliasExesBufferLength) = 0;

    virtual HRESULT GetConsoleAliasExesWImpl(_Out_writes_bytes_(*pcbAliasExesBufferLength) wchar_t* const pwsAliasExesBuffer,
                                             _Inout_ ULONG* const pcbAliasExesBufferLength) = 0;

#pragma region CMDext Private API

    virtual HRESULT ExpungeConsoleCommandHistoryAImpl(_In_reads_bytes_(cbExeNameBufferLength) const char* const psExeNameBuffer,
                                                      _In_ ULONG const cbExeNameBufferLength) = 0;

    virtual HRESULT ExpungeConsoleCommandHistoryWImpl(_In_reads_bytes_(cbExeNameBufferLength) const wchar_t* const pwsExeNameBuffer,
                                                      _In_ ULONG const cbExeNameBufferLength) = 0;

    virtual HRESULT SetConsoleNumberOfCommandsAImpl(_In_reads_bytes_(cbExeNameBufferLength) const char* const psExeNameBuffer,
                                                    _In_ ULONG const cbExeNameBufferLength,
                                                    _In_ ULONG const NumberOfCommands) = 0;

    virtual HRESULT SetConsoleNumberOfCommandsWImpl(_In_reads_bytes_(cbExeNameBufferLength) const wchar_t* const pwsExeNameBuffer,
                                                    _In_ ULONG const cbExeNameBufferLength,
                                                    _In_ ULONG const NumberOfCommands) = 0;

    virtual HRESULT GetConsoleCommandHistoryLengthAImpl(_In_reads_bytes_(cbExeNameBufferLength) const char* const psExeNameBuffer,
                                                        _In_ ULONG const cbExeNameBufferLength,
                                                        _Out_ ULONG* const pCommandHistoryLength) = 0;

    virtual HRESULT GetConsoleCommandHistoryLengthWImpl(_In_reads_bytes_(cbExeNameBufferLength) const wchar_t* const pwsExeNameBuffer,
                                                        _In_ ULONG const cbExeNameBufferLength,
                                                        _Out_ ULONG* const pCommandHistoryLength) = 0;

    virtual HRESULT GetConsoleCommandHistoryAImpl(_In_reads_bytes_(cbExeNameBufferLength) const char* const psExeNameBuffer,
                                                  _In_ ULONG const cbExeNameBufferLength,
                                                  _Out_writes_bytes_(*pcbCommandHistoryBufferLength) char* const psCommandHistoryBuffer,
                                                  _Inout_ ULONG* const pcbCommandHistoryBufferLength) = 0;

    virtual HRESULT GetConsoleCommandHistoryWImpl(_In_reads_bytes_(cbExeNameBufferLength) const wchar_t* const pwsExeNameBuffer,
                                                  _In_ ULONG const cbExeNameBufferLength,
                                                  _Out_writes_bytes_(*pcbCommandHistoryBufferLength) wchar_t* const pwsCommandHistoryBuffer,
                                                  _Inout_ ULONG* const pcbCommandHistoryBufferLength) = 0;

#pragma endregion

    virtual HRESULT GetConsoleWindowImpl(_Out_ HWND* const pHwnd);

    virtual HRESULT GetConsoleSelectionInfoImpl(_Out_ CONSOLE_SELECTION_INFO* const pConsoleSelectionInfo);

    virtual HRESULT GetConsoleHistoryInfoImpl(_Out_ CONSOLE_HISTORY_INFO* const pConsoleHistoryInfo);

    virtual HRESULT SetConsoleHistoryInfoImpl(_In_ const CONSOLE_HISTORY_INFO* const pConsoleHistoryInfo);

    virtual HRESULT SetCurrentConsoleFontExImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                _In_ BOOLEAN const IsForMaximumWindowSize,
                                                _In_ const CONSOLE_FONT_INFOEX* const pConsoleFontInfoEx);

#pragma endregion
};