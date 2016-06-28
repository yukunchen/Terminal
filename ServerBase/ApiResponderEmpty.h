#pragma once

#include "IApiResponders.h"

class ApiResponderEmpty : public IApiResponders
{
public:
    ApiResponderEmpty();
    ~ApiResponderEmpty();

#pragma region ObjectManagement
    DWORD CreateInitialObjects(_Out_ IConsoleInputObject** const ppInputObject,
                               _Out_ IConsoleOutputObject** const ppOutputObject);
#pragma endregion

#pragma region L1
    DWORD GetConsoleInputCodePageImpl(_Out_ ULONG* const pCodePage);

    DWORD GetConsoleOutputCodePageImpl(_Out_ ULONG* const pCodePage);

    DWORD GetConsoleInputModeImpl(_In_ IConsoleInputObject* const pInContext,
                                  _Out_ ULONG* const pMode);

    DWORD GetConsoleOutputModeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                   _Out_ ULONG* const pMode);

    DWORD SetConsoleInputModeImpl(_In_ IConsoleInputObject* const pInContext,
                                  _In_ ULONG const Mode);

    DWORD SetConsoleOutputModeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                   _In_ ULONG const Mode);

    DWORD GetNumberOfConsoleInputEventsImpl(_In_ IConsoleInputObject* const pInContext,
                                            _Out_ ULONG* const pEvents);

    DWORD PeekConsoleInputAImpl(_In_ IConsoleInputObject* const pInContext,
                                _Out_writes_to_(InputRecordsBufferLength, *pRecordsWritten) INPUT_RECORD* const pInputRecordsBuffer,
                                _In_ ULONG const InputRecordsBufferLength,
                                _Out_ ULONG* const pRecordsWritten);

    DWORD PeekConsoleInputWImpl(_In_ IConsoleInputObject* const pInContext,
                                _Out_writes_to_(InputRecordsBufferLength, *pRecordsWritten) INPUT_RECORD* const pInputRecordsBuffer,
                                _In_ ULONG const InputRecordsBufferLength,
                                _Out_ ULONG* const pRecordsWritten);

    DWORD ReadConsoleInputAImpl(_In_ IConsoleInputObject* const pInContext,
                                _Out_writes_to_(InputRecordsBufferLength, *pRecordsWritten) INPUT_RECORD* const pInputRecordsBuffer,
                                _In_ ULONG const InputRecordsBufferLength,
                                _Out_ ULONG* const pRecordsWritten);

    DWORD ReadConsoleInputWImpl(_In_ IConsoleInputObject* const pInContext,
                                _Out_writes_to_(InputRecordsBufferLength, *pRecordsWritten) INPUT_RECORD* const pInputRecordsBuffer,
                                _In_ ULONG const InputRecordsBufferLength,
                                _Out_ ULONG* const pRecordsWritten);

    DWORD ReadConsoleAImpl(_In_ IConsoleInputObject* const pInContext,
                           _Out_writes_to_(TextBufferLength, *pTextBufferWritten) char* const pTextBuffer,
                           _In_ ULONG const TextBufferLength,
                           _Out_ ULONG* const pTextBufferWritten,
                           _In_opt_ CONSOLE_READCONSOLE_CONTROL* const pReadControl);

    DWORD ReadConsoleWImpl(_In_ IConsoleInputObject* const pInContext,
                           _Out_writes_to_(TextBufferLength, *pTextBufferWritten) wchar_t* const pTextBuffer,
                           _In_ ULONG const TextBufferLength,
                           _Out_ ULONG* const pTextBufferWritten,
                           _In_opt_ CONSOLE_READCONSOLE_CONTROL* const pReadControl);

    DWORD WriteConsoleAImpl(_In_ IConsoleOutputObject* const pOutContext,
                            _In_reads_(TextBufferLength) char* const pTextBuffer,
                            _In_ ULONG const TextBufferLength,
                            _Out_ ULONG* const pTextBufferRead);

    DWORD WriteConsoleWImpl(_In_ IConsoleOutputObject* const pOutContext,
                            _In_reads_(TextBufferLength) wchar_t* const pTextBuffer,
                            _In_ ULONG const TextBufferLength,
                            _Out_ ULONG* const pTextBufferRead);

#pragma endregion

#pragma region L2

    DWORD FillConsoleOutputAttributeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                         _In_ WORD const Attribute,
                                         _In_ DWORD const LengthToWrite,
                                         _In_ COORD const StartingCoordinate,
                                         _Out_ DWORD* const pCellsModified);

    DWORD FillConsoleOutputCharacterAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                          _In_ char const Character,
                                          _In_ DWORD const LengthToWrite,
                                          _In_ COORD const StartingCoordinate,
                                          _Out_ DWORD* const pCellsModified);

    DWORD FillConsoleOutputCharacterWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                          _In_ wchar_t const Character,
                                          _In_ DWORD const LengthToWrite,
                                          _In_ COORD const StartingCoordinate,
                                          _Out_ DWORD* const pCellsModified);

    // Process based. Restrict in protocol side?
    DWORD GenerateConsoleCtrlEventImpl(_In_ ULONG const ProcessGroupFilter,
                                       _In_ ULONG const ControlEvent);

    DWORD SetConsoleActiveScreenBufferImpl(_In_ HANDLE const NewOutContext);

    DWORD FlushConsoleInputBuffer(_In_ IConsoleInputObject* const pInContext);

    DWORD SetConsoleInputCodePageImpl(_In_ ULONG const CodePage);

    DWORD SetConsoleOutputCodePageImpl(_In_ ULONG const CodePage);

    DWORD GetConsoleCursorInfoImpl(_In_ IConsoleOutputObject* const pOutContext,
                                   _Out_ ULONG* const pCursorSize,
                                   _Out_ BOOLEAN* const pIsVisible);

    DWORD SetConsoleCursorInfoImpl(_In_ IConsoleOutputObject* const pOutContext,
                                   _In_ ULONG const CursorSize,
                                   _In_ BOOLEAN const IsVisible);

    // driver will pare down for non-Ex method
    DWORD GetConsoleScreenBufferInfoExImpl(_In_ IConsoleOutputObject* const pOutContext,
                                           _Out_ CONSOLE_SCREEN_BUFFER_INFOEX* const pScreenBufferInfoEx);

    DWORD SetConsoleScreenBufferInfoExImpl(_In_ IConsoleOutputObject* const pOutContext,
                                           _In_ const CONSOLE_SCREEN_BUFFER_INFOEX* const pScreenBufferInfoEx);

    DWORD SetConsoleScreenBufferSizeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                         _In_ const COORD* const pSize);

    DWORD SetConsoleCursorPositionImpl(_In_ IConsoleOutputObject* const pOutContext,
                                       _In_ const COORD* const pCursorPosition);

    DWORD GetLargestConsoleWindowSizeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                          _Out_ COORD* const pSize);

    DWORD ScrollConsoleScreenBufferAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                         _In_ const SMALL_RECT* const pSourceRectangle,
                                         _In_ const COORD* const pTargetOrigin,
                                         _In_opt_ const SMALL_RECT* const pTargetClipRectangle,
                                         _In_ const CHAR_INFO* const pFill);

    DWORD ScrollConsoleScreenBufferWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                         _In_ const SMALL_RECT* const pSourceRectangle,
                                         _In_ const COORD* const pTargetOrigin,
                                         _In_opt_ const SMALL_RECT* const pTargetClipRectangle,
                                         _In_ const CHAR_INFO* const pFill);

    DWORD SetConsoleTextAttributeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                      _In_ WORD const Attribute);

    DWORD SetConsoleWindowInfoImpl(_In_ IConsoleOutputObject* const pOutContext,
                                   _In_ BOOLEAN const IsAbsoluteRectangle,
                                   _In_ const SMALL_RECT* const pWindowRectangle);

    DWORD ReadConsoleOutputAttributeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                         _In_ const COORD* const pSourceOrigin,
                                         _Out_writes_to_(AttributeBufferLength, *pAttributeBufferWritten) WORD* const pAttributeBuffer,
                                         _In_ ULONG const AttributeBufferLength,
                                         _Out_ ULONG* const pAttributeBufferWritten);

    DWORD ReadConsoleOutputCharacterAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                          _In_ const COORD* const pSourceOrigin,
                                          _Out_writes_to_(TextBufferLength, *pTextBufferWritten) char* const pTextBuffer,
                                          _In_ ULONG const pTextBufferLength,
                                          _Out_ ULONG* const pTextBufferWritten);

    DWORD ReadConsoleOutputCharacterWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                          _In_ const COORD* const pSourceOrigin,
                                          _Out_writes_to_(TextBufferLength, *pTextBufferWritten) wchar_t* const pTextBuffer,
                                          _In_ ULONG const TextBufferLength,
                                          _Out_ ULONG* const pTextBufferWritten);

    DWORD WriteConsoleInputAImpl(_In_ IConsoleInputObject* const pInContext,
                                 _In_reads_(InputBufferLength) const INPUT_RECORD* const pInputBuffer,
                                 _In_ ULONG const InputBufferLength,
                                 _Out_ ULONG* const pInputBufferRead);

    DWORD WriteConsoleInputWImpl(_In_ IConsoleInputObject* const pInContext,
                                 _In_reads_(InputBufferLength) const INPUT_RECORD* const pInputBuffer,
                                 _In_ ULONG const InputBufferLength,
                                 _Out_ ULONG* const pInputBufferRead);

    DWORD WriteConsoleOutputAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                  _In_reads_(pTextBufferSize->X * pTextBufferSize->Y) const CHAR_INFO* const pTextBuffer,
                                  _In_ const COORD* const pTextBufferSize,
                                  _In_ const COORD* const pTextBufferSourceOrigin,
                                  _In_ const SMALL_RECT* const pTargetRectangle,
                                  _Out_ SMALL_RECT* const pAffectedRectangle);

    DWORD WriteConsoleOutputWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                  _In_reads_(pTextBufferSize->X * pTextBufferSize->Y) const CHAR_INFO* const pTextBuffer,
                                  _In_ const COORD* const pTextBufferSize,
                                  _In_ const COORD* const pTextBufferSourceOrigin,
                                  _In_ const SMALL_RECT* const pTargetRectangle,
                                  _Out_ SMALL_RECT* const pAffectedRectangle);

    DWORD WriteConsoleOutputAttributeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                          _In_reads_(AttributeBufferLength) const WORD* const pAttributeBuffer,
                                          _In_ ULONG const AttributeBufferLength,
                                          _In_ const COORD* const pTargetOrigin,
                                          _Out_ ULONG* const pAttributeBufferRead);

    DWORD WriteConsoleOutputCharacterAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                           _In_reads_(TextBufferLength) const char* const pTextBuffer,
                                           _In_ ULONG const TextBufferLength,
                                           _In_ const COORD* const pTargetOrigin,
                                           _Out_ ULONG* const pTextBufferRead);

    DWORD WriteConsoleOutputCharacterWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                           _In_reads_(TextBufferLength) const wchar_t* const pTextBuffer,
                                           _In_ ULONG const TextBufferLength,
                                           _In_ const COORD* const pTargetOrigin,
                                           _Out_ ULONG* const pTextBufferRead);

    DWORD ReadConsoleOutputA(_In_ IConsoleOutputObject* const pOutContext,
                             _Out_writes_(pTextBufferSize->X * pTextBufferSize->Y) CHAR_INFO* const pTextBuffer,
                             _In_ const COORD* const pTextBufferSize,
                             _In_ const COORD* const pTextBufferTargetOrigin,
                             _In_ const SMALL_RECT* const pSourceRectangle,
                             _Out_ SMALL_RECT* const pReadRectangle);

    DWORD ReadConsoleOutputW(_In_ IConsoleOutputObject* const pOutContext,
                             _Out_writes_(pTextBufferSize->X * pTextBufferSize->Y) CHAR_INFO* const pTextBuffer,
                             _In_ const COORD* const pTextBufferSize,
                             _In_ const COORD* const pTextBufferTargetOrigin,
                             _In_ const SMALL_RECT* const pSourceRectangle,
                             _Out_ SMALL_RECT* const pReadRectangle);

    DWORD GetConsoleTitleAImpl(_Out_writes_(TextBufferSize) char* const pTextBuffer,
                               _In_ ULONG const TextBufferSize);

    DWORD GetConsoleTitleWImpl(_Out_writes_(TextBufferSize) wchar_t* const pTextBuffer,
                               _In_ ULONG const TextBufferSize);

    DWORD GetConsoleOriginalTitleAImpl(_Out_writes_(TextBufferSize) char* const pTextBuffer,
                                       _In_ ULONG const TextBufferSize);

    DWORD GetConsoleOriginalTitleWImpl(_Out_writes_(TextBufferSize) char* const pTextBuffer,
                                       _In_ ULONG const TextBufferSize);

    DWORD SetConsoleTitleAImpl(_In_reads_(TextBufferSize) char* const pTextBuffer,
                               _In_ ULONG const TextBufferSize);

    DWORD SetConsoleTitleWImpl(_In_reads_(TextBufferSize) wchar_t* const pTextBuffer,
                               _In_ ULONG const TextBufferSize);

#pragma endregion

#pragma region L3
    DWORD GetNumberOfConsoleMouseButtonsImpl(_Out_ ULONG* const pButtons);

    DWORD GetConsoleFontSizeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                 _In_ DWORD const FontIndex,
                                 _Out_ COORD* const pFontSize);

    // driver will pare down for non-Ex method
    DWORD GetCurrentConsoleFontExImpl(_In_ IConsoleOutputObject* const pOutContext,
                                      _In_ BOOLEAN const IsForMaximumWindowSize,
                                      _Out_ CONSOLE_FONT_INFOEX* const pConsoleFontInfoEx);

    DWORD SetConsoleDisplayModeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                    _In_ ULONG const Flags,
                                    _Out_ COORD* const pNewScreenBufferSize);

    DWORD GetConsoleDisplayModeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                    _Out_ ULONG* const pFlags);

    DWORD AddConsoleAliasAImpl(_In_reads_(SourceBufferLength) const char* const pSourceBuffer,
                               _In_ ULONG const SourceBufferLength,
                               _In_reads_(TargetBufferLength) const char* const pTargetBuffer,
                               _In_ ULONG const TargetBufferLength,
                               _In_reads_(ExeNameBufferLength) const char* const pExeNameBuffer,
                               _In_ ULONG const ExeNameBufferLength);

    DWORD AddConsoleAliasWImpl(_In_reads_(SourceBufferLength) const wchar_t* const pSourceBuffer,
                               _In_ ULONG const SourceBufferLength,
                               _In_reads_(TargetBufferLength) const wchar_t* const pTargetBuffer,
                               _In_ ULONG const TargetBufferLength,
                               _In_reads_(ExeNameBufferLength) const wchar_t* const pExeNameBuffer,
                               _In_ ULONG const ExeNameBufferLength);

    DWORD GetConsoleAliasAImpl(_In_reads_(SourceBufferLength) const char* const pSourceBuffer,
                               _In_ ULONG const SourceBufferLength,
                               _Out_writes_(TargetBufferLength) char* const pTargetBuffer,
                               _In_ ULONG const TargetBufferLength,
                               _In_reads_(ExeNameBufferLength) const char* const pExeNameBuffer,
                               _In_ ULONG const ExeNameBufferLength);

    DWORD GetConsoleAliasWImpl(_In_reads_(SourceBufferLength) const wchar_t* const pSourceBuffer,
                               _In_ ULONG const SourceBufferLength,
                               _Out_writes_(TargetBufferLength) wchar_t* const pTargetBuffer,
                               _In_ ULONG const TargetBufferLength,
                               _In_reads_(ExeNameBufferLength) const wchar_t* const pExeNameBuffer,
                               _In_ ULONG const ExeNameBufferLength);

    DWORD GetConsoleAliasesLengthAImpl(_In_reads_(ExeNameBufferLength) const char* const pExeNameBuffer,
                                       _In_ ULONG const ExeNameBufferLength,
                                       _Out_ ULONG* const pAliasesBufferRequired);

    DWORD GetConsoleAliasesLengthWImpl(_In_reads_(ExeNameBufferLength) const wchar_t* const pExeNameBuffer,
                                       _In_ ULONG const ExeNameBufferLength,
                                       _Out_ ULONG* const pAliasesBufferRequired);

    DWORD GetConsoleAliasExesLengthAImpl(_Out_ ULONG* const pAliasExesBufferRequired);

    DWORD GetConsoleAliasExesLengthWImpl(_Out_ ULONG* const pAliasExesBufferRequired);

    DWORD GetConsoleAliasesAImpl(_In_reads_(ExeNameBufferLength) const char* const pExeNameBuffer,
                                 _In_ ULONG const ExeNameBufferLength,
                                 _Out_writes_(AliasBufferLength) char* const pAliasBuffer,
                                 _In_ ULONG const AliasBufferLength);

    DWORD GetConsoleAliasesWImpl(_In_reads_(ExeNameBufferLength) const wchar_t* const pExeNameBuffer,
                                 _In_ ULONG const ExeNameBufferLength,
                                 _Out_writes_(AliasBufferLength) wchar_t* const pAliasBuffer,
                                 _In_ ULONG const AliasBufferLength);

    DWORD GetConsoleAliasExesAImpl(_Out_writes_(AliasExesBufferLength) char* const pAliasExesBuffer,
                                   _In_ ULONG const AliasExesBufferLength);

    DWORD GetConsoleAliasExesWImpl(_Out_writes_(AliasExesBufferLength) wchar_t* const pAliasExesBuffer,
                                   _In_ ULONG const AliasExesBufferLength);

    DWORD GetConsoleWindowImpl(_Out_ HWND* const pHwnd);

    DWORD GetConsoleSelectionInfoImpl(_Out_ CONSOLE_SELECTION_INFO* const pConsoleSelectionInfo);

    DWORD GetConsoleProcessListImpl(_Out_writes_(*ProcessBufferLength) DWORD* const pProcessBuffer,
                                    _Inout_ ULONG* const pProcessBufferLength);

    DWORD GetConsoleHistoryInfoImpl(_Out_ CONSOLE_HISTORY_INFO* const pConsoleHistoryInfo);

    DWORD SetConsoleHistoryInfoImpl(_In_ const CONSOLE_HISTORY_INFO* const pConsoleHistoryInfo);

    DWORD SetCurrentConsoleFontExImpl(_In_ IConsoleOutputObject* const pOutContext,
                                      _In_ BOOLEAN const IsForMaximumWindowSize,
                                      _In_ const CONSOLE_FONT_INFOEX* const pConsoleFontInfoEx);

#pragma endregion
};

