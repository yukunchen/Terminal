#pragma once

#include "..\ServerBaseApi\IApiResponders.h"

class ApiResponderEmpty : public IApiResponders
{
public:
    ApiResponderEmpty();
    ~ApiResponderEmpty();

    void NotifyInput();

#pragma region ObjectManagement
    NTSTATUS CreateInitialObjects(_Out_ IConsoleInputObject** const ppInputObject,
                                  _Out_ IConsoleOutputObject** const ppOutputObject);
#pragma endregion

#pragma region L1
    NTSTATUS GetConsoleInputCodePageImpl(_Out_ ULONG* const pCodePage);

    NTSTATUS GetConsoleOutputCodePageImpl(_Out_ ULONG* const pCodePage);

    NTSTATUS GetConsoleInputModeImpl(_In_ IConsoleInputObject* const pInContext,
                                     _Out_ ULONG* const pMode);

    NTSTATUS GetConsoleOutputModeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                      _Out_ ULONG* const pMode);

    NTSTATUS SetConsoleInputModeImpl(_In_ IConsoleInputObject* const pInContext,
                                     _In_ ULONG const Mode);

    NTSTATUS SetConsoleOutputModeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                      _In_ ULONG const Mode);

    NTSTATUS GetNumberOfConsoleInputEventsImpl(_In_ IConsoleInputObject* const pInContext,
                                               _Out_ ULONG* const pEvents);

    NTSTATUS PeekConsoleInputAImpl(_In_ IConsoleInputObject* const pInContext,
                                   _Out_writes_to_(InputRecordsBufferLength, *pRecordsWritten) INPUT_RECORD* const pInputRecordsBuffer,
                                   _In_ ULONG const InputRecordsBufferLength,
                                   _Out_ ULONG* const pRecordsWritten);

    NTSTATUS PeekConsoleInputWImpl(_In_ IConsoleInputObject* const pInContext,
                                   _Out_writes_to_(InputRecordsBufferLength, *pRecordsWritten) INPUT_RECORD* const pInputRecordsBuffer,
                                   _In_ ULONG const InputRecordsBufferLength,
                                   _Out_ ULONG* const pRecordsWritten);

    NTSTATUS ReadConsoleInputAImpl(_In_ IConsoleInputObject* const pInContext,
                                   _Out_writes_to_(InputRecordsBufferLength, *pRecordsWritten) INPUT_RECORD* const pInputRecordsBuffer,
                                   _In_ ULONG const InputRecordsBufferLength,
                                   _Out_ ULONG* const pRecordsWritten);

    NTSTATUS ReadConsoleInputWImpl(_In_ IConsoleInputObject* const pInContext,
                                   _Out_writes_to_(InputRecordsBufferLength, *pRecordsWritten) INPUT_RECORD* const pInputRecordsBuffer,
                                   _In_ ULONG const InputRecordsBufferLength,
                                   _Out_ ULONG* const pRecordsWritten);

    NTSTATUS ReadConsoleAImpl(_In_ IConsoleInputObject* const pInContext,
                              _Out_writes_to_(TextBufferLength, *pTextBufferWritten) char* const pTextBuffer,
                              _In_ ULONG const TextBufferLength,
                              _Out_ ULONG* const pTextBufferWritten,
                              _In_opt_ CONSOLE_READCONSOLE_CONTROL* const pReadControl);

    NTSTATUS ReadConsoleWImpl(_In_ IConsoleInputObject* const pInContext,
                              _Out_writes_to_(TextBufferLength, *pTextBufferWritten) wchar_t* const pTextBuffer,
                              _In_ ULONG const TextBufferLength,
                              _Out_ ULONG* const pTextBufferWritten,
                              _In_opt_ CONSOLE_READCONSOLE_CONTROL* const pReadControl);

    NTSTATUS WriteConsoleAImpl(_In_ IConsoleOutputObject* const pOutContext,
                               _In_reads_(TextBufferLength) char* const pTextBuffer,
                               _In_ ULONG const TextBufferLength,
                               _Out_ ULONG* const pTextBufferRead);

    NTSTATUS WriteConsoleWImpl(_In_ IConsoleOutputObject* const pOutContext,
                               _In_reads_(TextBufferLength) wchar_t* const pTextBuffer,
                               _In_ ULONG const TextBufferLength,
                               _Out_ ULONG* const pTextBufferRead);

#pragma endregion

#pragma region L2

    NTSTATUS FillConsoleOutputAttributeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                            _In_ WORD const Attribute,
                                            _In_ DWORD const LengthToWrite,
                                            _In_ COORD const StartingCoordinate,
                                            _Out_ DWORD* const pCellsModified);

    NTSTATUS FillConsoleOutputCharacterAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                             _In_ char const Character,
                                             _In_ DWORD const LengthToWrite,
                                             _In_ COORD const StartingCoordinate,
                                             _Out_ DWORD* const pCellsModified);

    NTSTATUS FillConsoleOutputCharacterWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                             _In_ wchar_t const Character,
                                             _In_ DWORD const LengthToWrite,
                                             _In_ COORD const StartingCoordinate,
                                             _Out_ DWORD* const pCellsModified);

    // Process based. Restrict in protocol side?
    NTSTATUS GenerateConsoleCtrlEventImpl(_In_ ULONG const ProcessGroupFilter,
                                          _In_ ULONG const ControlEvent);

    NTSTATUS SetConsoleActiveScreenBufferImpl(_In_ HANDLE const NewOutContext);

    NTSTATUS FlushConsoleInputBuffer(_In_ IConsoleInputObject* const pInContext);

    NTSTATUS SetConsoleInputCodePageImpl(_In_ ULONG const CodePage);

    NTSTATUS SetConsoleOutputCodePageImpl(_In_ ULONG const CodePage);

    NTSTATUS GetConsoleCursorInfoImpl(_In_ IConsoleOutputObject* const pOutContext,
                                      _Out_ ULONG* const pCursorSize,
                                      _Out_ BOOLEAN* const pIsVisible);

    NTSTATUS SetConsoleCursorInfoImpl(_In_ IConsoleOutputObject* const pOutContext,
                                      _In_ ULONG const CursorSize,
                                      _In_ BOOLEAN const IsVisible);

    // driver will pare down for non-Ex method
    NTSTATUS GetConsoleScreenBufferInfoExImpl(_In_ IConsoleOutputObject* const pOutContext,
                                              _Out_ CONSOLE_SCREEN_BUFFER_INFOEX* const pScreenBufferInfoEx);

    NTSTATUS SetConsoleScreenBufferInfoExImpl(_In_ IConsoleOutputObject* const pOutContext,
                                              _In_ const CONSOLE_SCREEN_BUFFER_INFOEX* const pScreenBufferInfoEx);

    NTSTATUS SetConsoleScreenBufferSizeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                            _In_ const COORD* const pSize);

    NTSTATUS SetConsoleCursorPositionImpl(_In_ IConsoleOutputObject* const pOutContext,
                                          _In_ const COORD* const pCursorPosition);

    NTSTATUS GetLargestConsoleWindowSizeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                             _Out_ COORD* const pSize);

    NTSTATUS ScrollConsoleScreenBufferAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                            _In_ const SMALL_RECT* const pSourceRectangle,
                                            _In_ const COORD* const pTargetOrigin,
                                            _In_opt_ const SMALL_RECT* const pTargetClipRectangle,
                                            _In_ const CHAR_INFO* const pFill);

    NTSTATUS ScrollConsoleScreenBufferWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                            _In_ const SMALL_RECT* const pSourceRectangle,
                                            _In_ const COORD* const pTargetOrigin,
                                            _In_opt_ const SMALL_RECT* const pTargetClipRectangle,
                                            _In_ const CHAR_INFO* const pFill);

    NTSTATUS SetConsoleTextAttributeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                         _In_ WORD const Attribute);

    NTSTATUS SetConsoleWindowInfoImpl(_In_ IConsoleOutputObject* const pOutContext,
                                      _In_ BOOLEAN const IsAbsoluteRectangle,
                                      _In_ const SMALL_RECT* const pWindowRectangle);

    NTSTATUS ReadConsoleOutputAttributeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                            _In_ const COORD* const pSourceOrigin,
                                            _Out_writes_to_(AttributeBufferLength, *pAttributeBufferWritten) WORD* const pAttributeBuffer,
                                            _In_ ULONG const AttributeBufferLength,
                                            _Out_ ULONG* const pAttributeBufferWritten);

    NTSTATUS ReadConsoleOutputCharacterAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                             _In_ const COORD* const pSourceOrigin,
                                             _Out_writes_to_(TextBufferLength, *pTextBufferWritten) char* const pTextBuffer,
                                             _In_ ULONG const pTextBufferLength,
                                             _Out_ ULONG* const pTextBufferWritten);

    NTSTATUS ReadConsoleOutputCharacterWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                             _In_ const COORD* const pSourceOrigin,
                                             _Out_writes_to_(TextBufferLength, *pTextBufferWritten) wchar_t* const pTextBuffer,
                                             _In_ ULONG const TextBufferLength,
                                             _Out_ ULONG* const pTextBufferWritten);

    NTSTATUS WriteConsoleInputAImpl(_In_ IConsoleInputObject* const pInContext,
                                    _In_reads_(InputBufferLength) const INPUT_RECORD* const pInputBuffer,
                                    _In_ ULONG const InputBufferLength,
                                    _Out_ ULONG* const pInputBufferRead);

    NTSTATUS WriteConsoleInputWImpl(_In_ IConsoleInputObject* const pInContext,
                                    _In_reads_(InputBufferLength) const INPUT_RECORD* const pInputBuffer,
                                    _In_ ULONG const InputBufferLength,
                                    _Out_ ULONG* const pInputBufferRead);

    NTSTATUS WriteConsoleOutputAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                     _In_reads_(pTextBufferSize->X * pTextBufferSize->Y) const CHAR_INFO* const pTextBuffer,
                                     _In_ const COORD* const pTextBufferSize,
                                     _In_ const COORD* const pTextBufferSourceOrigin,
                                     _In_ const SMALL_RECT* const pTargetRectangle,
                                     _Out_ SMALL_RECT* const pAffectedRectangle);

    NTSTATUS WriteConsoleOutputWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                     _In_reads_(pTextBufferSize->X * pTextBufferSize->Y) const CHAR_INFO* const pTextBuffer,
                                     _In_ const COORD* const pTextBufferSize,
                                     _In_ const COORD* const pTextBufferSourceOrigin,
                                     _In_ const SMALL_RECT* const pTargetRectangle,
                                     _Out_ SMALL_RECT* const pAffectedRectangle);

    NTSTATUS WriteConsoleOutputAttributeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                             _In_reads_(AttributeBufferLength) const WORD* const pAttributeBuffer,
                                             _In_ ULONG const AttributeBufferLength,
                                             _In_ const COORD* const pTargetOrigin,
                                             _Out_ ULONG* const pAttributeBufferRead);

    NTSTATUS WriteConsoleOutputCharacterAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                              _In_reads_(TextBufferLength) const char* const pTextBuffer,
                                              _In_ ULONG const TextBufferLength,
                                              _In_ const COORD* const pTargetOrigin,
                                              _Out_ ULONG* const pTextBufferRead);

    NTSTATUS WriteConsoleOutputCharacterWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                              _In_reads_(TextBufferLength) const wchar_t* const pTextBuffer,
                                              _In_ ULONG const TextBufferLength,
                                              _In_ const COORD* const pTargetOrigin,
                                              _Out_ ULONG* const pTextBufferRead);

    NTSTATUS ReadConsoleOutputA(_In_ IConsoleOutputObject* const pOutContext,
                                _Out_writes_(pTextBufferSize->X * pTextBufferSize->Y) CHAR_INFO* const pTextBuffer,
                                _In_ const COORD* const pTextBufferSize,
                                _In_ const COORD* const pTextBufferTargetOrigin,
                                _In_ const SMALL_RECT* const pSourceRectangle,
                                _Out_ SMALL_RECT* const pReadRectangle);

    NTSTATUS ReadConsoleOutputW(_In_ IConsoleOutputObject* const pOutContext,
                                _Out_writes_(pTextBufferSize->X * pTextBufferSize->Y) CHAR_INFO* const pTextBuffer,
                                _In_ const COORD* const pTextBufferSize,
                                _In_ const COORD* const pTextBufferTargetOrigin,
                                _In_ const SMALL_RECT* const pSourceRectangle,
                                _Out_ SMALL_RECT* const pReadRectangle);

    NTSTATUS GetConsoleTitleAImpl(_Out_writes_(TextBufferSize) char* const pTextBuffer,
                                  _In_ ULONG const TextBufferSize);

    NTSTATUS GetConsoleTitleWImpl(_Out_writes_(TextBufferSize) wchar_t* const pTextBuffer,
                                  _In_ ULONG const TextBufferSize);

    NTSTATUS GetConsoleOriginalTitleAImpl(_Out_writes_(TextBufferSize) char* const pTextBuffer,
                                          _In_ ULONG const TextBufferSize);

    NTSTATUS GetConsoleOriginalTitleWImpl(_Out_writes_(TextBufferSize) char* const pTextBuffer,
                                          _In_ ULONG const TextBufferSize);

    NTSTATUS SetConsoleTitleAImpl(_In_reads_(TextBufferSize) char* const pTextBuffer,
                                  _In_ ULONG const TextBufferSize);

    NTSTATUS SetConsoleTitleWImpl(_In_reads_(TextBufferSize) wchar_t* const pTextBuffer,
                                  _In_ ULONG const TextBufferSize);

#pragma endregion

#pragma region L3
    NTSTATUS GetNumberOfConsoleMouseButtonsImpl(_Out_ ULONG* const pButtons);

    NTSTATUS GetConsoleFontSizeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                    _In_ DWORD const FontIndex,
                                    _Out_ COORD* const pFontSize);

    // driver will pare down for non-Ex method
    NTSTATUS GetCurrentConsoleFontExImpl(_In_ IConsoleOutputObject* const pOutContext,
                                         _In_ BOOLEAN const IsForMaximumWindowSize,
                                         _Out_ CONSOLE_FONT_INFOEX* const pConsoleFontInfoEx);

    NTSTATUS SetConsoleDisplayModeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                       _In_ ULONG const Flags,
                                       _Out_ COORD* const pNewScreenBufferSize);

    NTSTATUS GetConsoleDisplayModeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                       _Out_ ULONG* const pFlags);

    NTSTATUS AddConsoleAliasAImpl(_In_reads_(SourceBufferLength) const char* const pSourceBuffer,
                                  _In_ ULONG const SourceBufferLength,
                                  _In_reads_(TargetBufferLength) const char* const pTargetBuffer,
                                  _In_ ULONG const TargetBufferLength,
                                  _In_reads_(ExeNameBufferLength) const char* const pExeNameBuffer,
                                  _In_ ULONG const ExeNameBufferLength);

    NTSTATUS AddConsoleAliasWImpl(_In_reads_(SourceBufferLength) const wchar_t* const pSourceBuffer,
                                  _In_ ULONG const SourceBufferLength,
                                  _In_reads_(TargetBufferLength) const wchar_t* const pTargetBuffer,
                                  _In_ ULONG const TargetBufferLength,
                                  _In_reads_(ExeNameBufferLength) const wchar_t* const pExeNameBuffer,
                                  _In_ ULONG const ExeNameBufferLength);

    NTSTATUS GetConsoleAliasAImpl(_In_reads_(SourceBufferLength) const char* const pSourceBuffer,
                                  _In_ ULONG const SourceBufferLength,
                                  _Out_writes_(TargetBufferLength) char* const pTargetBuffer,
                                  _In_ ULONG const TargetBufferLength,
                                  _In_reads_(ExeNameBufferLength) const char* const pExeNameBuffer,
                                  _In_ ULONG const ExeNameBufferLength);

    NTSTATUS GetConsoleAliasWImpl(_In_reads_(SourceBufferLength) const wchar_t* const pSourceBuffer,
                                  _In_ ULONG const SourceBufferLength,
                                  _Out_writes_(TargetBufferLength) wchar_t* const pTargetBuffer,
                                  _In_ ULONG const TargetBufferLength,
                                  _In_reads_(ExeNameBufferLength) const wchar_t* const pExeNameBuffer,
                                  _In_ ULONG const ExeNameBufferLength);

    NTSTATUS GetConsoleAliasesLengthAImpl(_In_reads_(ExeNameBufferLength) const char* const pExeNameBuffer,
                                          _In_ ULONG const ExeNameBufferLength,
                                          _Out_ ULONG* const pAliasesBufferRequired);

    NTSTATUS GetConsoleAliasesLengthWImpl(_In_reads_(ExeNameBufferLength) const wchar_t* const pExeNameBuffer,
                                          _In_ ULONG const ExeNameBufferLength,
                                          _Out_ ULONG* const pAliasesBufferRequired);

    NTSTATUS GetConsoleAliasExesLengthAImpl(_Out_ ULONG* const pAliasExesBufferRequired);

    NTSTATUS GetConsoleAliasExesLengthWImpl(_Out_ ULONG* const pAliasExesBufferRequired);

    NTSTATUS GetConsoleAliasesAImpl(_In_reads_(ExeNameBufferLength) const char* const pExeNameBuffer,
                                    _In_ ULONG const ExeNameBufferLength,
                                    _Out_writes_(AliasBufferLength) char* const pAliasBuffer,
                                    _In_ ULONG const AliasBufferLength);

    NTSTATUS GetConsoleAliasesWImpl(_In_reads_(ExeNameBufferLength) const wchar_t* const pExeNameBuffer,
                                    _In_ ULONG const ExeNameBufferLength,
                                    _Out_writes_(AliasBufferLength) wchar_t* const pAliasBuffer,
                                    _In_ ULONG const AliasBufferLength);

    NTSTATUS GetConsoleAliasExesAImpl(_Out_writes_(AliasExesBufferLength) char* const pAliasExesBuffer,
                                      _In_ ULONG const AliasExesBufferLength);

    NTSTATUS GetConsoleAliasExesWImpl(_Out_writes_(AliasExesBufferLength) wchar_t* const pAliasExesBuffer,
                                      _In_ ULONG const AliasExesBufferLength);

    NTSTATUS GetConsoleWindowImpl(_Out_ HWND* const pHwnd);

    NTSTATUS GetConsoleSelectionInfoImpl(_Out_ CONSOLE_SELECTION_INFO* const pConsoleSelectionInfo);

    NTSTATUS GetConsoleHistoryInfoImpl(_Out_ CONSOLE_HISTORY_INFO* const pConsoleHistoryInfo);

    NTSTATUS SetConsoleHistoryInfoImpl(_In_ const CONSOLE_HISTORY_INFO* const pConsoleHistoryInfo);

    NTSTATUS SetCurrentConsoleFontExImpl(_In_ IConsoleOutputObject* const pOutContext,
                                         _In_ BOOLEAN const IsForMaximumWindowSize,
                                         _In_ const CONSOLE_FONT_INFOEX* const pConsoleFontInfoEx);

#pragma endregion
};

