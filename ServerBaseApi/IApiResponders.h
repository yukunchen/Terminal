#pragma once

#include "IApiService.h"
#include "IConsoleObjects.h"

typedef long NTSTATUS;

class IApiResponders
{
public:
    IApiResponders()
    {

    }

    void SetApiService(_In_ IApiService* const pService)
    {
        _pService = pService;
    }

protected:
    IApiService* _pService;

public:

#pragma region ObjectManagement
    virtual NTSTATUS CreateInitialObjects(_Out_ IConsoleInputObject** const ppInputObject,
                                          _Out_ IConsoleOutputObject** const ppOutputObject) = 0;


#pragma endregion

#pragma region L1
    virtual NTSTATUS GetConsoleInputCodePageImpl(_Out_ ULONG* const pCodePage) = 0;

    virtual NTSTATUS GetConsoleOutputCodePageImpl(_Out_ ULONG* const pCodePage) = 0;

    virtual NTSTATUS GetConsoleInputModeImpl(_In_ IConsoleInputObject* const pInContext,
                                             _Out_ ULONG* const pMode) = 0;

    virtual NTSTATUS GetConsoleOutputModeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                              _Out_ ULONG* const pMode) = 0;

    virtual NTSTATUS SetConsoleInputModeImpl(_In_ IConsoleInputObject* const pInContext,
                                             _In_ ULONG const Mode) = 0;

    virtual NTSTATUS SetConsoleOutputModeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                              _In_ ULONG const Mode) = 0;

    virtual NTSTATUS GetNumberOfConsoleInputEventsImpl(_In_ IConsoleInputObject* const pInContext,
                                                       _Out_ ULONG* const pEvents) = 0;

    virtual NTSTATUS PeekConsoleInputAImpl(_In_ IConsoleInputObject* const pInContext,
                                           _Out_writes_to_(InputRecordsBufferLength, *pRecordsWritten) INPUT_RECORD* const pInputRecordsBuffer,
                                           _In_ ULONG const InputRecordsBufferLength,
                                           _Out_ ULONG* const pRecordsWritten) = 0;

    virtual NTSTATUS PeekConsoleInputWImpl(_In_ IConsoleInputObject* const pInContext,
                                           _Out_writes_to_(InputRecordsBufferLength, *pRecordsWritten) INPUT_RECORD* const pInputRecordsBuffer,
                                           _In_ ULONG const InputRecordsBufferLength,
                                           _Out_ ULONG* const pRecordsWritten) = 0;

    virtual NTSTATUS ReadConsoleInputAImpl(_In_ IConsoleInputObject* const pInContext,
                                           _Out_writes_to_(InputRecordsBufferLength, *pRecordsWritten) INPUT_RECORD* const pInputRecordsBuffer,
                                           _In_ ULONG const InputRecordsBufferLength,
                                           _Out_ ULONG* const pRecordsWritten) = 0;

    virtual NTSTATUS ReadConsoleInputWImpl(_In_ IConsoleInputObject* const pInContext,
                                           _Out_writes_to_(InputRecordsBufferLength, *pRecordsWritten) INPUT_RECORD* const pInputRecordsBuffer,
                                           _In_ ULONG const InputRecordsBufferLength,
                                           _Out_ ULONG* const pRecordsWritten) = 0;

    virtual NTSTATUS ReadConsoleAImpl(_In_ IConsoleInputObject* const pInContext,
                                      _Out_writes_to_(TextBufferLength, *pTextBufferWritten) char* const pTextBuffer,
                                      _In_ ULONG const TextBufferLength,
                                      _Out_ ULONG* const pTextBufferWritten,
                                      _In_opt_ CONSOLE_READCONSOLE_CONTROL* const pReadControl) = 0;

    virtual NTSTATUS ReadConsoleWImpl(_In_ IConsoleInputObject* const pInContext,
                                      _Out_writes_to_(TextBufferLength, *pTextBufferWritten) wchar_t* const pTextBuffer,
                                      _In_ ULONG const TextBufferLength,
                                      _Out_ ULONG* const pTextBufferWritten,
                                      _In_opt_ CONSOLE_READCONSOLE_CONTROL* const pReadControl) = 0;

    virtual NTSTATUS WriteConsoleAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                       _In_reads_(TextBufferLength) char* const pTextBuffer,
                                       _In_ ULONG const TextBufferLength,
                                       _Out_ ULONG* const pTextBufferRead) = 0;

    virtual NTSTATUS WriteConsoleWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                       _In_reads_(TextBufferLength) wchar_t* const pTextBuffer,
                                       _In_ ULONG const TextBufferLength,
                                       _Out_ ULONG* const pTextBufferRead) = 0;

#pragma endregion

#pragma region L2

    virtual NTSTATUS FillConsoleOutputAttributeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                    _In_ WORD const Attribute,
                                                    _In_ DWORD const LengthToWrite,
                                                    _In_ COORD const StartingCoordinate,
                                                    _Out_ DWORD* const pCellsModified) = 0;

    virtual NTSTATUS FillConsoleOutputCharacterAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                     _In_ char const Character,
                                                     _In_ DWORD const LengthToWrite,
                                                     _In_ COORD const StartingCoordinate,
                                                     _Out_ DWORD* const pCellsModified) = 0;

    virtual NTSTATUS FillConsoleOutputCharacterWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                     _In_ wchar_t const Character,
                                                     _In_ DWORD const LengthToWrite,
                                                     _In_ COORD const StartingCoordinate,
                                                     _Out_ DWORD* const pCellsModified) = 0;

    // Process based. Restrict in protocol side?
    virtual NTSTATUS GenerateConsoleCtrlEventImpl(_In_ ULONG const ProcessGroupFilter,
                                                  _In_ ULONG const ControlEvent) = 0;

    virtual NTSTATUS SetConsoleActiveScreenBufferImpl(_In_ HANDLE const NewOutContext) = 0;

    virtual NTSTATUS FlushConsoleInputBuffer(_In_ IConsoleInputObject* const pInContext) = 0;

    virtual NTSTATUS SetConsoleInputCodePageImpl(_In_ ULONG const CodePage) = 0;

    virtual NTSTATUS SetConsoleOutputCodePageImpl(_In_ ULONG const CodePage) = 0;

    virtual NTSTATUS GetConsoleCursorInfoImpl(_In_ IConsoleOutputObject* const pOutContext,
                                              _Out_ ULONG* const pCursorSize,
                                              _Out_ BOOLEAN* const pIsVisible) = 0;

    virtual NTSTATUS SetConsoleCursorInfoImpl(_In_ IConsoleOutputObject* const pOutContext,
                                              _In_ ULONG const CursorSize,
                                              _In_ BOOLEAN const IsVisible) = 0;

    // driver will pare down for non-Ex method
    virtual NTSTATUS GetConsoleScreenBufferInfoExImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                      _Out_ CONSOLE_SCREEN_BUFFER_INFOEX* const pScreenBufferInfoEx) = 0;

    virtual NTSTATUS SetConsoleScreenBufferInfoExImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                      _In_ const CONSOLE_SCREEN_BUFFER_INFOEX* const pScreenBufferInfoEx) = 0;

    virtual NTSTATUS SetConsoleScreenBufferSizeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                    _In_ const COORD* const pSize) = 0;

    virtual NTSTATUS SetConsoleCursorPositionImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                  _In_ const COORD* const pCursorPosition) = 0;

    virtual NTSTATUS GetLargestConsoleWindowSizeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                     _Out_ COORD* const pSize) = 0;

    virtual NTSTATUS ScrollConsoleScreenBufferAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                    _In_ const SMALL_RECT* const pSourceRectangle,
                                                    _In_ const COORD* const pTargetOrigin,
                                                    _In_opt_ const SMALL_RECT* const pTargetClipRectangle,
                                                    _In_ const CHAR_INFO* const pFill) = 0;

    virtual NTSTATUS ScrollConsoleScreenBufferWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                    _In_ const SMALL_RECT* const pSourceRectangle,
                                                    _In_ const COORD* const pTargetOrigin,
                                                    _In_opt_ const SMALL_RECT* const pTargetClipRectangle,
                                                    _In_ const CHAR_INFO* const pFill) = 0;

    virtual NTSTATUS SetConsoleTextAttributeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                 _In_ WORD const Attribute) = 0;

    virtual NTSTATUS SetConsoleWindowInfoImpl(_In_ IConsoleOutputObject* const pOutContext,
                                              _In_ BOOLEAN const IsAbsoluteRectangle,
                                              _In_ const SMALL_RECT* const pWindowRectangle) = 0;

    virtual NTSTATUS ReadConsoleOutputAttributeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                    _In_ const COORD* const pSourceOrigin,
                                                    _Out_writes_to_(AttributeBufferLength, *pAttributeBufferWritten) WORD* const pAttributeBuffer,
                                                    _In_ ULONG const AttributeBufferLength,
                                                    _Out_ ULONG* const pAttributeBufferWritten) = 0;

    virtual NTSTATUS ReadConsoleOutputCharacterAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                     _In_ const COORD* const pSourceOrigin,
                                                     _Out_writes_to_(TextBufferLength, *pTextBufferWritten) char* const pTextBuffer,
                                                     _In_ ULONG const pTextBufferLength,
                                                     _Out_ ULONG* const pTextBufferWritten) = 0;

    virtual NTSTATUS ReadConsoleOutputCharacterWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                     _In_ const COORD* const pSourceOrigin,
                                                     _Out_writes_to_(TextBufferLength, *pTextBufferWritten) wchar_t* const pTextBuffer,
                                                     _In_ ULONG const TextBufferLength,
                                                     _Out_ ULONG* const pTextBufferWritten) = 0;

    virtual NTSTATUS WriteConsoleInputAImpl(_In_ IConsoleInputObject* const pInContext,
                                            _In_reads_(InputBufferLength) const INPUT_RECORD* const pInputBuffer,
                                            _In_ ULONG const InputBufferLength,
                                            _Out_ ULONG* const pInputBufferRead) = 0;

    virtual NTSTATUS WriteConsoleInputWImpl(_In_ IConsoleInputObject* const pInContext,
                                            _In_reads_(InputBufferLength) const INPUT_RECORD* const pInputBuffer,
                                            _In_ ULONG const InputBufferLength,
                                            _Out_ ULONG* const pInputBufferRead) = 0;

    virtual NTSTATUS WriteConsoleOutputAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                             _In_reads_(pTextBufferSize->X * pTextBufferSize->Y) const CHAR_INFO* const pTextBuffer,
                                             _In_ const COORD* const pTextBufferSize,
                                             _In_ const COORD* const pTextBufferSourceOrigin,
                                             _In_ const SMALL_RECT* const pTargetRectangle,
                                             _Out_ SMALL_RECT* const pAffectedRectangle) = 0;

    virtual NTSTATUS WriteConsoleOutputWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                             _In_reads_(pTextBufferSize->X * pTextBufferSize->Y) const CHAR_INFO* const pTextBuffer,
                                             _In_ const COORD* const pTextBufferSize,
                                             _In_ const COORD* const pTextBufferSourceOrigin,
                                             _In_ const SMALL_RECT* const pTargetRectangle,
                                             _Out_ SMALL_RECT* const pAffectedRectangle) = 0;

    virtual NTSTATUS WriteConsoleOutputAttributeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                     _In_reads_(AttributeBufferLength) const WORD* const pAttributeBuffer,
                                                     _In_ ULONG const AttributeBufferLength,
                                                     _In_ const COORD* const pTargetOrigin,
                                                     _Out_ ULONG* const pAttributeBufferRead) = 0;

    virtual NTSTATUS WriteConsoleOutputCharacterAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                      _In_reads_(TextBufferLength) const char* const pTextBuffer,
                                                      _In_ ULONG const TextBufferLength,
                                                      _In_ const COORD* const pTargetOrigin,
                                                      _Out_ ULONG* const pTextBufferRead) = 0;

    virtual NTSTATUS WriteConsoleOutputCharacterWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                      _In_reads_(TextBufferLength) const wchar_t* const pTextBuffer,
                                                      _In_ ULONG const TextBufferLength,
                                                      _In_ const COORD* const pTargetOrigin,
                                                      _Out_ ULONG* const pTextBufferRead) = 0;

    virtual NTSTATUS ReadConsoleOutputA(_In_ IConsoleOutputObject* const pOutContext,
                                        _Out_writes_(pTextBufferSize->X * pTextBufferSize->Y) CHAR_INFO* const pTextBuffer,
                                        _In_ const COORD* const pTextBufferSize,
                                        _In_ const COORD* const pTextBufferTargetOrigin,
                                        _In_ const SMALL_RECT* const pSourceRectangle,
                                        _Out_ SMALL_RECT* const pReadRectangle) = 0;

    virtual NTSTATUS ReadConsoleOutputW(_In_ IConsoleOutputObject* const pOutContext,
                                        _Out_writes_(pTextBufferSize->X * pTextBufferSize->Y) CHAR_INFO* const pTextBuffer,
                                        _In_ const COORD* const pTextBufferSize,
                                        _In_ const COORD* const pTextBufferTargetOrigin,
                                        _In_ const SMALL_RECT* const pSourceRectangle,
                                        _Out_ SMALL_RECT* const pReadRectangle) = 0;

    virtual NTSTATUS GetConsoleTitleAImpl(_Out_writes_(TextBufferSize) char* const pTextBuffer,
                                          _In_ ULONG const TextBufferSize) = 0;

    virtual NTSTATUS GetConsoleTitleWImpl(_Out_writes_(TextBufferSize) wchar_t* const pTextBuffer,
                                          _In_ ULONG const TextBufferSize) = 0;

    virtual NTSTATUS GetConsoleOriginalTitleAImpl(_Out_writes_(TextBufferSize) char* const pTextBuffer,
                                                  _In_ ULONG const TextBufferSize) = 0;

    virtual NTSTATUS GetConsoleOriginalTitleWImpl(_Out_writes_(TextBufferSize) char* const pTextBuffer,
                                                  _In_ ULONG const TextBufferSize) = 0;

    virtual NTSTATUS SetConsoleTitleAImpl(_In_reads_(TextBufferSize) char* const pTextBuffer,
                                          _In_ ULONG const TextBufferSize) = 0;

    virtual NTSTATUS SetConsoleTitleWImpl(_In_reads_(TextBufferSize) wchar_t* const pTextBuffer,
                                          _In_ ULONG const TextBufferSize) = 0;

#pragma endregion

#pragma region L3
    virtual NTSTATUS GetNumberOfConsoleMouseButtonsImpl(_Out_ ULONG* const pButtons) = 0;

    virtual NTSTATUS GetConsoleFontSizeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                            _In_ DWORD const FontIndex,
                                            _Out_ COORD* const pFontSize) = 0;

    // driver will pare down for non-Ex method
    virtual NTSTATUS GetCurrentConsoleFontExImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                 _In_ BOOLEAN const IsForMaximumWindowSize,
                                                 _Out_ CONSOLE_FONT_INFOEX* const pConsoleFontInfoEx) = 0;

    virtual NTSTATUS SetConsoleDisplayModeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                               _In_ ULONG const Flags,
                                               _Out_ COORD* const pNewScreenBufferSize) = 0;

    virtual NTSTATUS GetConsoleDisplayModeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                               _Out_ ULONG* const pFlags) = 0;

    virtual NTSTATUS AddConsoleAliasAImpl(_In_reads_(SourceBufferLength) const char* const pSourceBuffer,
                                          _In_ ULONG const SourceBufferLength,
                                          _In_reads_(TargetBufferLength) const char* const pTargetBuffer,
                                          _In_ ULONG const TargetBufferLength,
                                          _In_reads_(ExeNameBufferLength) const char* const pExeNameBuffer,
                                          _In_ ULONG const ExeNameBufferLength) = 0;

    virtual NTSTATUS AddConsoleAliasWImpl(_In_reads_(SourceBufferLength) const wchar_t* const pSourceBuffer,
                                          _In_ ULONG const SourceBufferLength,
                                          _In_reads_(TargetBufferLength) const wchar_t* const pTargetBuffer,
                                          _In_ ULONG const TargetBufferLength,
                                          _In_reads_(ExeNameBufferLength) const wchar_t* const pExeNameBuffer,
                                          _In_ ULONG const ExeNameBufferLength) = 0;

    virtual NTSTATUS GetConsoleAliasAImpl(_In_reads_(SourceBufferLength) const char* const pSourceBuffer,
                                          _In_ ULONG const SourceBufferLength,
                                          _Out_writes_(TargetBufferLength) char* const pTargetBuffer,
                                          _In_ ULONG const TargetBufferLength,
                                          _In_reads_(ExeNameBufferLength) const char* const pExeNameBuffer,
                                          _In_ ULONG const ExeNameBufferLength) = 0;

    virtual NTSTATUS GetConsoleAliasWImpl(_In_reads_(SourceBufferLength) const wchar_t* const pSourceBuffer,
                                          _In_ ULONG const SourceBufferLength,
                                          _Out_writes_(TargetBufferLength) wchar_t* const pTargetBuffer,
                                          _In_ ULONG const TargetBufferLength,
                                          _In_reads_(ExeNameBufferLength) const wchar_t* const pExeNameBuffer,
                                          _In_ ULONG const ExeNameBufferLength) = 0;

    virtual NTSTATUS GetConsoleAliasesLengthAImpl(_In_reads_(ExeNameBufferLength) const char* const pExeNameBuffer,
                                                  _In_ ULONG const ExeNameBufferLength,
                                                  _Out_ ULONG* const pAliasesBufferRequired) = 0;

    virtual NTSTATUS GetConsoleAliasesLengthWImpl(_In_reads_(ExeNameBufferLength) const wchar_t* const pExeNameBuffer,
                                                  _In_ ULONG const ExeNameBufferLength,
                                                  _Out_ ULONG* const pAliasesBufferRequired) = 0;

    virtual NTSTATUS GetConsoleAliasExesLengthAImpl(_Out_ ULONG* const pAliasExesBufferRequired) = 0;

    virtual NTSTATUS GetConsoleAliasExesLengthWImpl(_Out_ ULONG* const pAliasExesBufferRequired) = 0;

    virtual NTSTATUS GetConsoleAliasesAImpl(_In_reads_(ExeNameBufferLength) const char* const pExeNameBuffer,
                                            _In_ ULONG const ExeNameBufferLength,
                                            _Out_writes_(AliasBufferLength) char* const pAliasBuffer,
                                            _In_ ULONG const AliasBufferLength) = 0;

    virtual NTSTATUS GetConsoleAliasesWImpl(_In_reads_(ExeNameBufferLength) const wchar_t* const pExeNameBuffer,
                                            _In_ ULONG const ExeNameBufferLength,
                                            _Out_writes_(AliasBufferLength) wchar_t* const pAliasBuffer,
                                            _In_ ULONG const AliasBufferLength) = 0;

    virtual NTSTATUS GetConsoleAliasExesAImpl(_Out_writes_(AliasExesBufferLength) char* const pAliasExesBuffer,
                                              _In_ ULONG const AliasExesBufferLength) = 0;

    virtual NTSTATUS GetConsoleAliasExesWImpl(_Out_writes_(AliasExesBufferLength) wchar_t* const pAliasExesBuffer,
                                              _In_ ULONG const AliasExesBufferLength) = 0;

    virtual NTSTATUS GetConsoleWindowImpl(_Out_ HWND* const pHwnd) = 0;

    virtual NTSTATUS GetConsoleSelectionInfoImpl(_Out_ CONSOLE_SELECTION_INFO* const pConsoleSelectionInfo) = 0;

    virtual NTSTATUS GetConsoleHistoryInfoImpl(_Out_ CONSOLE_HISTORY_INFO* const pConsoleHistoryInfo) = 0;

    virtual NTSTATUS SetConsoleHistoryInfoImpl(_In_ const CONSOLE_HISTORY_INFO* const pConsoleHistoryInfo) = 0;

    virtual NTSTATUS SetCurrentConsoleFontExImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                 _In_ BOOLEAN const IsForMaximumWindowSize,
                                                 _In_ const CONSOLE_FONT_INFOEX* const pConsoleFontInfoEx) = 0;

#pragma endregion
};