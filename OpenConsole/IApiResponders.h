#pragma once

class IApiResponders
{
public:

#pragma region L1
    virtual DWORD GetConsoleInputCodePageImpl(_In_ HANDLE const InContext,
                                              _Out_ ULONG* const pCodePage) = 0;

    virtual DWORD GetConsoleOutputCodePageImpl(_In_ HANDLE const OutContext,
                                               _Out_ ULONG* const pCodePage) = 0;

    virtual DWORD GetConsoleInputModeImpl(_In_ HANDLE const InContext,
                                          _Out_ ULONG* const pMode) = 0;

    virtual DWORD GetConsoleOutputModeImpl(_In_ HANDLE const OutContext,
                                           _Out_ ULONG* const pMode) = 0;

    virtual DWORD SetConsoleInputModeImpl(_In_ HANDLE const InContext,
                                          _In_ ULONG const Mode) = 0;

    virtual DWORD SetConsoleOutputModeImpl(_In_ HANDLE const OutContext,
                                           _In_ ULONG const Mode) = 0;

    virtual DWORD GetNumberOfConsoleInputEventsImpl(_In_ HANDLE const InContext,
                                                    _Out_ ULONG* const pEvents) = 0;

    virtual DWORD PeekConsoleInputAImpl(_In_ HANDLE const InContext,
                                        _Out_writes_to_(InputRecordsBufferLength, *pRecordsWritten) INPUT_RECORD* const pInputRecordsBuffer,
                                        _In_ ULONG const InputRecordsBufferLength,
                                        _Out_ ULONG* const pRecordsWritten) = 0;

    virtual DWORD PeekConsoleInputWImpl(_In_ HANDLE const InContext,
                                        _Out_writes_to_(InputRecordsBufferLength, *pRecordsWritten) INPUT_RECORD* const pInputRecordsBuffer,
                                        _In_ ULONG const InputRecordsBufferLength,
                                        _Out_ ULONG* const pRecordsWritten) = 0;

    virtual DWORD ReadConsoleInputAImpl(_In_ HANDLE const InContext,
                                        _Out_writes_to_(InputRecordsBufferLength, *pRecordsWritten) INPUT_RECORD* const pInputRecordsBuffer,
                                        _In_ ULONG const InputRecordsBufferLength,
                                        _Out_ ULONG* const pRecordsWritten) = 0;

    virtual DWORD ReadConsoleInputWImpl(_In_ HANDLE const InContext,
                                        _Out_writes_to_(InputRecordsBufferLength, *pRecordsWritten) INPUT_RECORD* const pInputRecordsBuffer,
                                        _In_ ULONG const InputRecordsBufferLength,
                                        _Out_ ULONG* const pRecordsWritten) = 0;

    virtual DWORD ReadConsoleAImpl(_In_ HANDLE const InContext,
                                   _Out_writes_to_(TextBufferLength, *pTextBufferWritten) char* const pTextBuffer,
                                   _In_ ULONG const TextBufferLength,
                                   _Out_ ULONG* const pTextBufferWritten,
                                   _In_opt_ CONSOLE_READCONSOLE_CONTROL* const pReadControl) = 0;

    virtual DWORD ReadConsoleWImpl(_In_ HANDLE const InContext,
                                   _Out_writes_to_(TextBufferLength, *pTextBufferWritten) wchar_t* const pTextBuffer,
                                   _In_ ULONG const TextBufferLength,
                                   _Out_ ULONG* const pTextBufferWritten,
                                   _In_opt_ CONSOLE_READCONSOLE_CONTROL* const pReadControl) = 0;

    virtual DWORD WriteConsoleAImpl(_In_ HANDLE const OutContext,
                                    _In_reads_(TextBufferLength) char* const pTextBuffer,
                                    _In_ ULONG const TextBufferLength,
                                    _Out_ ULONG* const pTextBufferRead) = 0;

    virtual DWORD WriteConsoleWImpl(_In_ HANDLE const OutContext,
                                    _In_reads_(TextBufferLength) wchar_t* const pTextBuffer,
                                    _In_ ULONG const TextBufferLength,
                                    _Out_ ULONG* const pTextBufferRead) = 0;

    // This isn't a public API but exists to help set up the thread language state with the OS. It shouldn't be exposed as such, it should just fetch output CP.
    virtual DWORD GetConsoleLangId(_In_ HANDLE const OutContext,
                                   _Out_ LANGID* const pLangId) = 0;

#pragma endregion

#pragma region L2

    virtual DWORD FillConsoleOutputAttributeImpl(_In_ HANDLE const OutContext,
                                                 _In_ WORD const Attribute,
                                                 _In_ DWORD const LengthToWrite,
                                                 _In_ COORD const StartingCoordinate,
                                                 _Out_ DWORD* const pCellsModified) = 0;

    virtual DWORD FillConsoleOutputCharacterAImpl(_In_ HANDLE const OutContext,
                                                  _In_ char const Character,
                                                  _In_ DWORD const LengthToWrite,
                                                  _In_ COORD const StartingCoordinate,
                                                  _Out_ DWORD* const pCellsModified) = 0;

    virtual DWORD FillConsoleOutputCharacterWImpl(_In_ HANDLE const OutContext,
                                                  _In_ wchar_t const Character,
                                                  _In_ DWORD const LengthToWrite,
                                                  _In_ COORD const StartingCoordinate,
                                                  _Out_ DWORD* const pCellsModified) = 0;

    // Process based. Restrict in protocol side?
    virtual DWORD GenerateConsoleCtrlEventImpl(_In_ ULONG const ProcessGroupFilter,
                                               _In_ ULONG const ControlEvent) = 0;

    virtual DWORD SetConsoleActiveScreenBufferImpl(_In_ HANDLE const NewOutContext) = 0;

    virtual DWORD FlushConsoleInputBuffer(_In_ HANDLE const InContext) = 0;

    virtual DWORD SetConsoleInputCodePageImpl(_In_ HANDLE const InContext,
                                              _In_ ULONG const CodePage) = 0;

    virtual DWORD SetConsoleOutputCodePageImpl(_In_ HANDLE const OutContext,
                                               _In_ ULONG const CodePage) = 0;

    virtual DWORD GetConsoleCursorInfoImpl(_In_ HANDLE const OutContext, 
                                           _Out_ ULONG* const pCursorSize,
                                           _Out_ BOOLEAN* const pIsVisible) = 0;

    virtual DWORD SetConsoleCursorInfoImpl(_In_ HANDLE const OutContext,
                                           _In_ ULONG const CursorSize,
                                           _In_ BOOLEAN const IsVisible) = 0;

    // driver will pare down for non-Ex method
    virtual DWORD GetConsoleScreenBufferInfoExImpl(_In_ HANDLE const OutContext,
                                                   _Out_ CONSOLE_SCREEN_BUFFER_INFOEX* const pScreenBufferInfoEx) = 0;

    virtual DWORD SetConsoleScreenBufferInfoExImpl(_In_ HANDLE const OutContext,
                                                   _In_ const CONSOLE_SCREEN_BUFFER_INFOEX* const pScreenBufferInfoEx) = 0;

    virtual DWORD SetConsoleScreenBufferSizeImpl(_In_ HANDLE const OutContext,
                                                 _In_ const COORD* const pSize) = 0;

    virtual DWORD SetConsoleCursorPositionImpl(_In_ HANDLE const OutContext,
                                               _In_ const COORD* const pCursorPosition) = 0;

    virtual DWORD GetLargestConsoleWindowSizeImpl(_In_ HANDLE const OutContext,
                                                  _Out_ COORD* const pSize) = 0;

    virtual DWORD ScrollConsoleScreenBufferAImpl(_In_ HANDLE const OutContext,
                                                 _In_ const SMALL_RECT* const pSourceRectangle,
                                                 _In_ const COORD* const pTargetOrigin,
                                                 _In_opt_ const SMALL_RECT* const pTargetClipRectangle,
                                                 _In_ const CHAR_INFO* const pFill) = 0;

    virtual DWORD ScrollConsoleScreenBufferWImpl(_In_ HANDLE const OutContext,
                                                 _In_ const SMALL_RECT* const pSourceRectangle,
                                                 _In_ const COORD* const pTargetOrigin,
                                                 _In_opt_ const SMALL_RECT* const pTargetClipRectangle,
                                                 _In_ const CHAR_INFO* const pFill) = 0;

    virtual DWORD SetConsoleTextAttributeImpl(_In_ HANDLE const OutContext,
                                              _In_ WORD const Attribute) = 0;

    virtual DWORD SetConsoleWindowInfoImpl(_In_ HANDLE const OutContext,
                                           _In_ BOOLEAN const IsAbsoluteRectangle,
                                           _In_ const SMALL_RECT* const pWindowRectangle) = 0;

    virtual DWORD ReadConsoleOutputAttributeImpl(_In_ HANDLE const OutContext,
                                                 _In_ const COORD* const pSourceOrigin,
                                                 _Out_writes_to_(AttributeBufferLength, *pAttributeBufferWritten) WORD* const pAttributeBuffer,
                                                 _In_ ULONG const AttributeBufferLength,
                                                 _Out_ ULONG* const pAttributeBufferWritten) = 0;

    virtual DWORD ReadConsoleOutputCharacterAImpl(_In_ HANDLE const OutContext,
                                                  _In_ const COORD* const pSourceOrigin,
                                                  _Out_writes_to_(TextBufferLength, *pTextBufferWritten) char* const pTextBuffer,
                                                  _In_ ULONG const pTextBufferLength,
                                                  _Out_ ULONG* const pTextBufferWritten) = 0;

    virtual DWORD ReadConsoleOutputCharacterWImpl(_In_ HANDLE const OutContext,
                                                  _In_ const COORD* const pSourceOrigin,
                                                  _Out_writes_to_(TextBufferLength, *pTextBufferWritten) wchar_t* const pTextBuffer,
                                                  _In_ ULONG const TextBufferLength,
                                                  _Out_ ULONG* const pTextBufferWritten) = 0;

    virtual DWORD WriteConsoleInputAImpl(_In_ HANDLE const InContext,
                                         _In_reads_(InputBufferLength) const INPUT_RECORD* const pInputBuffer,
                                         _In_ ULONG const InputBufferLength,
                                         _Out_ ULONG* const pInputBufferRead) = 0;

    virtual DWORD WriteConsoleInputWImpl(_In_ HANDLE const InContext,
                                         _In_reads_(InputBufferLength) const INPUT_RECORD* const pInputBuffer,
                                         _In_ ULONG const InputBufferLength,
                                         _Out_ ULONG* const pInputBufferRead) = 0;

    virtual DWORD WriteConsoleOutputAImpl(_In_ HANDLE const OutContext,
                                          _In_reads_(pTextBufferSize->X * pTextBufferSize->Y) const CHAR_INFO* const pTextBuffer,
                                          _In_ const COORD* const pTextBufferSize,
                                          _In_ const COORD* const pTextBufferSourceOrigin,
                                          _In_ const SMALL_RECT* const pTargetRectangle,
                                          _Out_ SMALL_RECT* const pAffectedRectangle) = 0;

    virtual DWORD WriteConsoleOutputWImpl(_In_ HANDLE const OutContext,
                                          _In_reads_(pTextBufferSize->X * pTextBufferSize->Y) const CHAR_INFO* const pTextBuffer,
                                          _In_ const COORD* const pTextBufferSize,
                                          _In_ const COORD* const pTextBufferSourceOrigin,
                                          _In_ const SMALL_RECT* const pTargetRectangle,
                                          _Out_ SMALL_RECT* const pAffectedRectangle) = 0;

    virtual DWORD WriteConsoleOutputAttributeImpl(_In_ HANDLE const OutContext,
                                                  _In_reads_(AttributeBufferLength) const WORD* const pAttributeBuffer,
                                                  _In_ ULONG const AttributeBufferLength,
                                                  _In_ const COORD* const pTargetOrigin,
                                                  _Out_ ULONG* const pAttributeBufferRead) = 0;

    virtual DWORD WriteConsoleOutputCharacterAImpl(_In_ HANDLE const OutContext,
                                                   _In_reads_(TextBufferLength) const char* const pTextBuffer,
                                                   _In_ ULONG const TextBufferLength,
                                                   _In_ const COORD* const pTargetOrigin,
                                                   _Out_ ULONG* const pTextBufferRead) = 0;

    virtual DWORD WriteConsoleOutputCharacterWImpl(_In_ HANDLE const OutContext,
                                                   _In_reads_(TextBufferLength) const wchar_t* const pTextBuffer,
                                                   _In_ ULONG const TextBufferLength,
                                                   _In_ const COORD* const pTargetOrigin,
                                                   _Out_ ULONG* const pTextBufferRead) = 0;

    virtual DWORD ReadConsoleOutputA(_In_ HANDLE const OutContext,
                                     _Out_writes_(pTextBufferSize->X * pTextBufferSize->Y) CHAR_INFO* const pTextBuffer,
                                     _In_ const COORD* const pTextBufferSize,
                                     _In_ const COORD* const pTextBufferTargetOrigin,
                                     _In_ const SMALL_RECT* const pSourceRectangle,
                                     _Out_ SMALL_RECT* const pReadRectangle) = 0;

    virtual DWORD ReadConsoleOutputW(_In_ HANDLE const OutContext,
                                     _Out_writes_(pTextBufferSize->X * pTextBufferSize->Y) CHAR_INFO* const pTextBuffer,
                                     _In_ const COORD* const pTextBufferSize,
                                     _In_ const COORD* const pTextBufferTargetOrigin,
                                     _In_ const SMALL_RECT* const pSourceRectangle,
                                     _Out_ SMALL_RECT* const pReadRectangle) = 0;

    virtual DWORD GetConsoleTitleAImpl(_In_ HANDLE const OutContext,
                                       _Out_writes_(TextBufferSize) char* const pTextBuffer,
                                       _In_ ULONG const TextBufferSize) = 0;

    virtual DWORD GetConsoleTitleWImpl(_In_ HANDLE const OutContext,
                                       _Out_writes_(TextBufferSize) wchar_t* const pTextBuffer,
                                       _In_ ULONG const TextBufferSize) = 0;

    virtual DWORD GetConsoleOriginalTitleAImpl(_In_ HANDLE const OutContext,
                                               _Out_writes_(TextBufferSize) char* const pTextBuffer,
                                               _In_ ULONG const TextBufferSize) = 0;

    virtual DWORD GetConsoleOriginalTitleWImpl(_In_ HANDLE const OutContext,
                                               _Out_writes_(TextBufferSize) char* const pTextBuffer,
                                               _In_ ULONG const TextBufferSize) = 0;

    virtual DWORD SetConsoleTitleAImpl(_In_ HANDLE const OutContext,
                                       _In_reads_(TextBufferSize) char* const pTextBuffer,
                                       _In_ ULONG const TextBufferSize) = 0;

    virtual DWORD SetConsoleTitleWImpl(_In_ HANDLE const OutContext,
                                       _In_reads_(TextBufferSize) wchar_t* const pTextBuffer,
                                       _In_ ULONG const TextBufferSize) = 0;

#pragma endregion

#pragma region L3
    virtual DWORD GetNumberOfConsoleMouseButtonsImpl(_Out_ ULONG* const pButtons) = 0;

    virtual DWORD GetConsoleFontSizeImpl(_In_ HANDLE const OutContext,
                                         _In_ DWORD const FontIndex,
                                         _Out_ COORD* const pFontSize) = 0;

    // driver will pare down for non-Ex method
    virtual DWORD GetCurrentConsoleFontExImpl(_In_ HANDLE const OutContext,
                                              _In_ BOOLEAN const IsForMaximumWindowSize,
                                              _Out_ CONSOLE_FONT_INFOEX* const pConsoleFontInfoEx) = 0;

    virtual DWORD SetConsoleDisplayModeImpl(_In_ HANDLE const OutContext,
                                            _In_ ULONG const Flags,
                                            _Out_ COORD* const pNewScreenBufferSize) = 0;

    virtual DWORD GetConsoleDisplayModeImpl(_In_ HANDLE const OutContext,
                                            _Out_ ULONG* const pFlags) = 0;

    virtual DWORD AddConsoleAliasAImpl(_In_reads_(SourceBufferLength) const char* const pSourceBuffer,
                                       _In_ ULONG const SourceBufferLength,
                                       _In_reads_(TargetBufferLength) const char* const pTargetBuffer,
                                       _In_ ULONG const TargetBufferLength,
                                       _In_reads_(ExeNameBufferLength) const char* const pExeNameBuffer,
                                       _In_ ULONG const ExeNameBufferLength) = 0;

    virtual DWORD AddConsoleAliasWImpl(_In_reads_(SourceBufferLength) const wchar_t* const pSourceBuffer,
                                       _In_ ULONG const SourceBufferLength,
                                       _In_reads_(TargetBufferLength) const wchar_t* const pTargetBuffer,
                                       _In_ ULONG const TargetBufferLength,
                                       _In_reads_(ExeNameBufferLength) const wchar_t* const pExeNameBuffer,
                                       _In_ ULONG const ExeNameBufferLength) = 0;

    virtual DWORD GetConsoleAliasAImpl(_In_reads_(SourceBufferLength) const char* const pSourceBuffer,
                                       _In_ ULONG const SourceBufferLength,
                                       _Out_writes_(TargetBufferLength) char* const pTargetBuffer,
                                       _In_ ULONG const TargetBufferLength,
                                       _In_reads_(ExeNameBufferLength) const char* const pExeNameBuffer,
                                       _In_ ULONG const ExeNameBufferLength) = 0;

    virtual DWORD GetConsoleAliasWImpl(_In_reads_(SourceBufferLength) const wchar_t* const pSourceBuffer,
                                       _In_ ULONG const SourceBufferLength,
                                       _Out_writes_(TargetBufferLength) wchar_t* const pTargetBuffer,
                                       _In_ ULONG const TargetBufferLength,
                                       _In_reads_(ExeNameBufferLength) const wchar_t* const pExeNameBuffer,
                                       _In_ ULONG const ExeNameBufferLength) = 0;

    virtual DWORD GetConsoleAliasesLengthAImpl(_In_reads_(ExeNameBufferLength) const char* const pExeNameBuffer,
                                               _In_ ULONG const ExeNameBufferLength,
                                               _Out_ ULONG* const pAliasesBufferRequired) = 0;

    virtual DWORD GetConsoleAliasesLengthWImpl(_In_reads_(ExeNameBufferLength) const wchar_t* const pExeNameBuffer,
                                               _In_ ULONG const ExeNameBufferLength,
                                               _Out_ ULONG* const pAliasesBufferRequired) = 0;

    virtual DWORD GetConsoleAliasExesLengthAImpl(_Out_ ULONG* const pAliasExesBufferRequired) = 0;

    virtual DWORD GetConsoleAliasExesLengthWImpl(_Out_ ULONG* const pAliasExesBufferRequired) = 0;

    virtual DWORD GetConsoleAliasesAImpl(_In_reads_(ExeNameBufferLength) const char* const pExeNameBuffer,
                                         _In_ ULONG const ExeNameBufferLength,
                                         _Out_writes_(AliasBufferLength) char* const pAliasBuffer,
                                         _In_ ULONG const AliasBufferLength) = 0;

    virtual DWORD GetConsoleAliasesWImpl(_In_reads_(ExeNameBufferLength) const wchar_t* const pExeNameBuffer,
                                         _In_ ULONG const ExeNameBufferLength,
                                         _Out_writes_(AliasBufferLength) wchar_t* const pAliasBuffer,
                                         _In_ ULONG const AliasBufferLength) = 0;

    virtual DWORD GetConsoleAliasExesAImpl(_Out_writes_(AliasExesBufferLength) char* const pAliasExesBuffer,
                                           _In_ ULONG const AliasExesBufferLength) = 0;
    
    virtual DWORD GetConsoleAliasExesWImpl(_Out_writes_(AliasExesBufferLength) wchar_t* const pAliasExesBuffer,
                                           _In_ ULONG const AliasExesBufferLength) = 0;

    virtual DWORD GetConsoleWindowImpl(_Out_ HWND* const pHwnd) = 0;

    virtual DWORD GetConsoleSelectionInfoImpl(_Out_ CONSOLE_SELECTION_INFO* const pConsoleSelectionInfo) = 0;

    virtual DWORD GetConsoleProcessListImpl(_Out_writes_(*ProcessBufferLength) DWORD* const pProcessBuffer,
                                            _Inout_ ULONG* const pProcessBufferLength) = 0;

    virtual DWORD GetConsoleHistoryInfoImpl(_Out_ CONSOLE_HISTORY_INFO* const pConsoleHistoryInfo) = 0;

    virtual DWORD SetConsoleHistoryInfoImpl(_In_ const CONSOLE_HISTORY_INFO* const pConsoleHistoryInfo) = 0;

    virtual DWORD SetCurrentConsoleFontExImpl(_In_ HANDLE const OutContext,
                                              _In_ BOOLEAN const IsForMaximumWindowSize,
                                              _In_ const CONSOLE_FONT_INFOEX* const pConsoleFontInfoEx) = 0;

#pragma endregion
};