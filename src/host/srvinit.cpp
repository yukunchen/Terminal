/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "srvinit.h"

#include "dbcs.h"
#include "directio.h"
#include "getset.h"
#include "globals.h"
#include "handle.h"
#include "icon.hpp"
#include "misc.h"
#include "output.h"
#include "registry.hpp"
#include "stream.h"
#include "renderFontDefaults.hpp"
#include "windowdpiapi.hpp"
#include "userprivapi.hpp"

#pragma hdrstop

#define CONSOLE_API_STRUCT(Routine, Struct) { Routine, sizeof(Struct) }
#define CONSOLE_API_NO_PARAMETER(Routine) { Routine, 0 }

typedef NTSTATUS(*PCONSOLE_API_ROUTINE) (_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL ReplyPending);

typedef struct _CONSOLE_API_DESCRIPTOR
{
    PCONSOLE_API_ROUTINE Routine;
    ULONG RequiredSize;
} CONSOLE_API_DESCRIPTOR, *PCONSOLE_API_DESCRIPTOR;

typedef struct _CONSOLE_API_LAYER_DESCRIPTOR
{
    const CONSOLE_API_DESCRIPTOR *Descriptor;
    ULONG Count;
} CONSOLE_API_LAYER_DESCRIPTOR, *PCONSOLE_API_LAYER_DESCRIPTOR;

NTSTATUS SrvDeprecatedAPI(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL ReplyPending);

const CONSOLE_API_DESCRIPTOR ConsoleApiLayer1[] = {
    CONSOLE_API_STRUCT(SrvGetConsoleCP, CONSOLE_GETCP_MSG),
    CONSOLE_API_STRUCT(SrvGetConsoleMode, CONSOLE_MODE_MSG),
    CONSOLE_API_STRUCT(SrvSetConsoleMode, CONSOLE_MODE_MSG),
    CONSOLE_API_STRUCT(SrvGetConsoleNumberOfInputEvents, CONSOLE_GETNUMBEROFINPUTEVENTS_MSG),
    CONSOLE_API_STRUCT(SrvGetConsoleInput, CONSOLE_GETCONSOLEINPUT_MSG),
    CONSOLE_API_STRUCT(SrvReadConsole, CONSOLE_READCONSOLE_MSG),
    CONSOLE_API_STRUCT(SrvWriteConsole, CONSOLE_WRITECONSOLE_MSG),
    CONSOLE_API_NO_PARAMETER(SrvDeprecatedAPI), // SrvConsoleNotifyLastClose
    CONSOLE_API_STRUCT(SrvGetConsoleLangId, CONSOLE_LANGID_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_MAPBITMAP_MSG),
};

const CONSOLE_API_DESCRIPTOR ConsoleApiLayer2[] = {
    CONSOLE_API_STRUCT(SrvFillConsoleOutput, CONSOLE_FILLCONSOLEOUTPUT_MSG),
    CONSOLE_API_STRUCT(SrvGenerateConsoleCtrlEvent, CONSOLE_CTRLEVENT_MSG),
    CONSOLE_API_NO_PARAMETER(SrvSetConsoleActiveScreenBuffer),
    CONSOLE_API_NO_PARAMETER(SrvFlushConsoleInputBuffer),
    CONSOLE_API_STRUCT(SrvSetConsoleCP, CONSOLE_SETCP_MSG),
    CONSOLE_API_STRUCT(SrvGetConsoleCursorInfo, CONSOLE_GETCURSORINFO_MSG),
    CONSOLE_API_STRUCT(SrvSetConsoleCursorInfo, CONSOLE_SETCURSORINFO_MSG),
    CONSOLE_API_STRUCT(SrvGetConsoleScreenBufferInfo, CONSOLE_SCREENBUFFERINFO_MSG),
    CONSOLE_API_STRUCT(SrvSetScreenBufferInfo, CONSOLE_SCREENBUFFERINFO_MSG),
    CONSOLE_API_STRUCT(SrvSetConsoleScreenBufferSize, CONSOLE_SETSCREENBUFFERSIZE_MSG),
    CONSOLE_API_STRUCT(SrvSetConsoleCursorPosition, CONSOLE_SETCURSORPOSITION_MSG),
    CONSOLE_API_STRUCT(SrvGetLargestConsoleWindowSize, CONSOLE_GETLARGESTWINDOWSIZE_MSG),
    CONSOLE_API_STRUCT(SrvScrollConsoleScreenBuffer, CONSOLE_SCROLLSCREENBUFFER_MSG),
    CONSOLE_API_STRUCT(SrvSetConsoleTextAttribute, CONSOLE_SETTEXTATTRIBUTE_MSG),
    CONSOLE_API_STRUCT(SrvSetConsoleWindowInfo, CONSOLE_SETWINDOWINFO_MSG),
    CONSOLE_API_STRUCT(SrvReadConsoleOutputString, CONSOLE_READCONSOLEOUTPUTSTRING_MSG),
    CONSOLE_API_STRUCT(SrvWriteConsoleInput, CONSOLE_WRITECONSOLEINPUT_MSG),
    CONSOLE_API_STRUCT(SrvWriteConsoleOutput, CONSOLE_WRITECONSOLEOUTPUT_MSG),
    CONSOLE_API_STRUCT(SrvWriteConsoleOutputString, CONSOLE_WRITECONSOLEOUTPUTSTRING_MSG),
    CONSOLE_API_STRUCT(SrvReadConsoleOutput, CONSOLE_READCONSOLEOUTPUT_MSG),
    CONSOLE_API_STRUCT(SrvGetConsoleTitle, CONSOLE_GETTITLE_MSG),
    CONSOLE_API_STRUCT(SrvSetConsoleTitle, CONSOLE_SETTITLE_MSG),
};

const CONSOLE_API_DESCRIPTOR ConsoleApiLayer3[] = {
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_GETNUMBEROFFONTS_MSG),
    CONSOLE_API_STRUCT(SrvGetConsoleMouseInfo, CONSOLE_GETMOUSEINFO_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_GETFONTINFO_MSG),
    CONSOLE_API_STRUCT(SrvGetConsoleFontSize, CONSOLE_GETFONTSIZE_MSG),
    CONSOLE_API_STRUCT(SrvGetConsoleCurrentFont, CONSOLE_CURRENTFONT_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_SETFONT_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_SETICON_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_INVALIDATERECT_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_VDM_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_SETCURSOR_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_SHOWCURSOR_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_MENUCONTROL_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_SETPALETTE_MSG),
    CONSOLE_API_STRUCT(SrvSetConsoleDisplayMode, CONSOLE_SETDISPLAYMODE_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_REGISTERVDM_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_GETHARDWARESTATE_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_SETHARDWARESTATE_MSG),
    CONSOLE_API_STRUCT(SrvGetConsoleDisplayMode, CONSOLE_GETDISPLAYMODE_MSG),
    CONSOLE_API_STRUCT(SrvAddConsoleAlias, CONSOLE_ADDALIAS_MSG),
    CONSOLE_API_STRUCT(SrvGetConsoleAlias, CONSOLE_GETALIAS_MSG),
    CONSOLE_API_STRUCT(SrvGetConsoleAliasesLength, CONSOLE_GETALIASESLENGTH_MSG),
    CONSOLE_API_STRUCT(SrvGetConsoleAliasExesLength, CONSOLE_GETALIASEXESLENGTH_MSG),
    CONSOLE_API_STRUCT(SrvGetConsoleAliases, CONSOLE_GETALIASES_MSG),
    CONSOLE_API_STRUCT(SrvGetConsoleAliasExes, CONSOLE_GETALIASEXES_MSG),
    CONSOLE_API_STRUCT(SrvExpungeConsoleCommandHistory, CONSOLE_EXPUNGECOMMANDHISTORY_MSG),
    CONSOLE_API_STRUCT(SrvSetConsoleNumberOfCommands, CONSOLE_SETNUMBEROFCOMMANDS_MSG),
    CONSOLE_API_STRUCT(SrvGetConsoleCommandHistoryLength, CONSOLE_GETCOMMANDHISTORYLENGTH_MSG),
    CONSOLE_API_STRUCT(SrvGetConsoleCommandHistory, CONSOLE_GETCOMMANDHISTORY_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_SETKEYSHORTCUTS_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_SETMENUCLOSE_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_GETKEYBOARDLAYOUTNAME_MSG),
    CONSOLE_API_STRUCT(SrvGetConsoleWindow, CONSOLE_GETCONSOLEWINDOW_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_CHAR_TYPE_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_LOCAL_EUDC_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_CURSOR_MODE_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_CURSOR_MODE_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_REGISTEROS2_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_SETOS2OEMFORMAT_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_NLS_MODE_MSG),
    CONSOLE_API_STRUCT(SrvDeprecatedAPI, CONSOLE_NLS_MODE_MSG),
    CONSOLE_API_STRUCT(SrvGetConsoleSelectionInfo, CONSOLE_GETSELECTIONINFO_MSG),
    CONSOLE_API_STRUCT(SrvGetConsoleProcessList, CONSOLE_GETCONSOLEPROCESSLIST_MSG),
    CONSOLE_API_STRUCT(SrvGetConsoleHistory, CONSOLE_HISTORY_MSG),
    CONSOLE_API_STRUCT(SrvSetConsoleHistory, CONSOLE_HISTORY_MSG),
    CONSOLE_API_STRUCT(SrvSetConsoleCurrentFont, CONSOLE_CURRENTFONT_MSG),
};

const CONSOLE_API_LAYER_DESCRIPTOR ConsoleApiLayerTable[] = {
    {ConsoleApiLayer1, RTL_NUMBER_OF(ConsoleApiLayer1)},
    {ConsoleApiLayer2, RTL_NUMBER_OF(ConsoleApiLayer2)},
    {ConsoleApiLayer3, RTL_NUMBER_OF(ConsoleApiLayer3)},
};

const UINT CONSOLE_EVENT_FAILURE_ID = 21790;
const UINT CONSOLE_LPC_PORT_FAILURE_ID = 21791;

void LoadLinkInfo(_Inout_ Settings* pLinkSettings,
                  _Inout_updates_bytes_(*pdwTitleLength) LPWSTR pwszTitle,
                  _Inout_ PDWORD pdwTitleLength,
                  _In_ PCWSTR pwszCurrDir,
                  _In_ PCWSTR pwszAppName)
{
    WCHAR wszIconLocation[MAX_PATH] = {0};
    int iIconIndex = 0;

    pLinkSettings->SetCodePage(g_uiOEMCP);

    // Did we get started from a link?
    if (pLinkSettings->GetStartupFlags() & STARTF_TITLEISLINKNAME)
    {
        if (SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)))
        {
            const size_t cbTitle = (*pdwTitleLength + 1) * sizeof(WCHAR);
            g_ciConsoleInformation.LinkTitle = (PWSTR) new BYTE[cbTitle];

            NTSTATUS Status = NT_TESTNULL(g_ciConsoleInformation.LinkTitle);
            if (NT_SUCCESS(Status))
            {
                if (FAILED(StringCbCopyNW(g_ciConsoleInformation.LinkTitle, cbTitle, pwszTitle, *pdwTitleLength)))
                {
                    Status = STATUS_UNSUCCESSFUL;
                }

                if (NT_SUCCESS(Status))
                {
                    CONSOLE_STATE_INFO csi = {0};
                    csi.LinkTitle = g_ciConsoleInformation.LinkTitle;
                    WCHAR wszShortcutTitle[MAX_PATH];
                    BOOL fReadConsoleProperties;
                    WORD wShowWindow = pLinkSettings->GetShowWindow();
                    DWORD dwHotKey = pLinkSettings->GetHotKey();
                    Status = ShortcutSerialization::s_GetLinkValues(&csi,
                                                                    &fReadConsoleProperties,
                                                                    wszShortcutTitle,
                                                                    ARRAYSIZE(wszShortcutTitle),
                                                                    wszIconLocation,
                                                                    ARRAYSIZE(wszIconLocation),
                                                                    &iIconIndex,
                                                                    (int*) &(wShowWindow),
                                                                    (WORD*)&(dwHotKey));
                    pLinkSettings->SetShowWindow(wShowWindow);
                    pLinkSettings->SetHotKey(dwHotKey);
                    // if we got a title, use it. even on overall link value load failure, the title will be correct if
                    // filled out.
                    if (wszShortcutTitle[0] != L'\0')
                    {
                        // guarantee null termination to make OACR happy.
                        wszShortcutTitle[ARRAYSIZE(wszShortcutTitle)-1] = L'\0';
                        StringCbCopyW(pwszTitle, *pdwTitleLength, wszShortcutTitle);

                        // OACR complains about the use of a DWORD here, so roundtrip through a size_t
                        size_t cbTitleLength;
                        if (SUCCEEDED(StringCbLengthW(pwszTitle, *pdwTitleLength, &cbTitleLength)))
                        {
                            // don't care about return result -- the buffer is guaranteed null terminated to at least
                            // the length of Title
                            (void)SizeTToDWord(cbTitleLength, pdwTitleLength);
                        }
                    }

                    if (NT_SUCCESS(Status) && fReadConsoleProperties)
                    {
                        // copy settings
                        pLinkSettings->InitFromStateInfo(&csi);

                        // since we were launched via shortcut, make sure we don't let the invoker's STARTUPINFO pollute the
                        // shortcut's settings
                        pLinkSettings->UnsetStartupFlag(STARTF_USESIZE | STARTF_USECOUNTCHARS);
                    }
                    else
                    {
                        // if we didn't find any console properties, or otherwise failed to load link properties, pretend
                        // like we weren't launched from a shortcut -- this allows us to at least try to find registry
                        // settings based on title.
                        pLinkSettings->UnsetStartupFlag(STARTF_TITLEISLINKNAME);
                    }
                }
            }
            CoUninitialize();
        }
    }

    // Go get the icon
    if (wszIconLocation[0] == L'\0')
    {
        // search for the application along the path so that we can load its icons (if we didn't find one explicitly in
        // the shortcut)
        const DWORD dwLinkLen = SearchPathW(pwszCurrDir, pwszAppName, nullptr, ARRAYSIZE(wszIconLocation), wszIconLocation, nullptr);
        if (dwLinkLen <= 0 || dwLinkLen > sizeof(wszIconLocation))
        {
            StringCchCopyW(wszIconLocation, ARRAYSIZE(wszIconLocation), pwszAppName);
        }
    }

    if (wszIconLocation[0] != L'\0')
    {
        Icon::Instance().LoadIconsFromPath(wszIconLocation, iIconIndex);
    }

    if (!IsValidCodePage(pLinkSettings->GetCodePage()))
    {
        // make sure we don't leave this function with an invalid codepage
        pLinkSettings->SetCodePage(g_uiOEMCP);
    }
}

NTSTATUS ConsoleServerInitialization(_In_ HANDLE Server)
{
    g_ciConsoleInformation.Server = Server;

    g_uiOEMCP = GetOEMCP();

    InitializeListHead(&g_ciConsoleInformation.ProcessHandleList);
    InitializeListHead(&g_ciConsoleInformation.CommandHistoryList);
    InitializeListHead(&g_ciConsoleInformation.OutputQueue);
    InitializeListHead(&g_ciConsoleInformation.ExeAliasList);
    InitializeListHead(&g_ciConsoleInformation.MessageQueue);

    g_pFontDefaultList = new RenderFontDefaults();
    FontInfo::s_SetFontDefaultList(g_pFontDefaultList);

    // Removed allocation of scroll buffer here.
    return STATUS_SUCCESS;
}

// Calling FindProcessInList(nullptr) means you want the root process.
PCONSOLE_PROCESS_HANDLE FindProcessInList(_In_opt_ HANDLE hProcess)
{
    PLIST_ENTRY const ListHead = &g_ciConsoleInformation.ProcessHandleList;
    PLIST_ENTRY ListNext = ListHead->Flink;

    while (ListNext != ListHead)
    {
        PCONSOLE_PROCESS_HANDLE const ProcessHandleRecord = CONTAINING_RECORD(ListNext, CONSOLE_PROCESS_HANDLE, ListLink);
        if (0 != hProcess)
        {
            if (ProcessHandleRecord->ClientId.UniqueProcess == hProcess)
            {
                return ProcessHandleRecord;
            }
        }
        else
        {
            if (ProcessHandleRecord->RootProcess)
            {
                return ProcessHandleRecord;
            }
        }

        ListNext = ListNext->Flink;
    }

    return nullptr;
}

NTSTATUS SetUpConsole(_Inout_ Settings* pStartupSettings,
                      _In_ DWORD TitleLength,
                      _In_reads_bytes_(TitleLength) LPWSTR Title,
                      _In_ LPCWSTR CurDir,
                      _In_ LPCWSTR AppName)
{
    // We will find and locate all relevant preference settings and then create the console here.
    // The precedence order for settings is:
    // 1. STARTUPINFO settings
    // 2a. Shortcut/Link settings
    // 2b. Registry specific settings
    // 3. Registry default settings
    // 4. Hardcoded default settings
    // To establish this hierarchy, we will need to load the settings and apply them in reverse order.

    // 4. Initializing Settings will establish hardcoded defaults.
    // Set to reference of global console information since that's the only place we need to hold the settings.
    Settings& settings = g_ciConsoleInformation;

    // 3. Read the default registry values.
    Registry reg(&settings);
    reg.LoadGlobalsFromRegistry();
    reg.LoadDefaultFromRegistry();

    // 2. Read specific settings

    // Link is expecting the flags from the process to be in already, so apply that first
    settings.SetStartupFlags(pStartupSettings->GetStartupFlags());

    // We need to see if we were spawned from a link. If we were, we need to
    // call back into the shell to try to get all the console information from the link.
    LoadLinkInfo(&settings, Title, &TitleLength, CurDir, AppName);

    // If we weren't started from a link, this will already be set.
    // If LoadLinkInfo couldn't find anything, it will remove the flag so we can dig in the registry.
    if (!(settings.IsStartupFlagSet(STARTF_TITLEISLINKNAME)))
    {
        reg.LoadFromRegistry(Title);
    }

    // 1. The settings we were passed contains STARTUPINFO structure settings to be applied last.
    settings.ApplyStartupInfo(pStartupSettings);

    // Validate all applied settings for correctness against final rules.
    settings.Validate();

    // As of the graphics refactoring to library based, all fonts are now DPI aware. Scaling is performed at the Blt time for raster fonts.
    // Note that we can only declare our DPI awareness once per process launch.
    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

    // Allow child dialogs (i.e. Properties and Find) to scale automatically based on DPI if we're currently DPI aware.
    WindowDpiApi::s_EnablePerMonitorDialogScaling();

    //Save initial font name for comparison on exit. We want telemetry when the font has changed
    if (settings.IsFaceNameSet())
    {
        settings.SetLaunchFaceName(settings.GetFaceName(), LF_FACESIZE);
    }

    // Now we need to actually create the console using the settings given.
    #pragma prefast(suppress:26018, "PREfast can't detect null termination status of Title.")

    // Allocate console will read the global g_ciConsoleInformation for the settings we just set.
    NTSTATUS Status = AllocateConsole(Title, TitleLength);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    return STATUS_SUCCESS;
}

VOID FreeConsoleIMEStuff()
{
    PCONVERSIONAREA_INFORMATION ConvAreaInfo, ConvAreaInfoNext;

    ConvAreaInfo = g_ciConsoleInformation.ConsoleIme.ConvAreaRoot;
    while (ConvAreaInfo)
    {
        ConvAreaInfoNext = ConvAreaInfo->ConvAreaNext;
        FreeConvAreaScreenBuffer(ConvAreaInfo->ScreenBuffer);
        delete ConvAreaInfo;
        ConvAreaInfo = ConvAreaInfoNext;
    }

    if (g_ciConsoleInformation.ConsoleIme.CompStrData)
    {
        delete[] g_ciConsoleInformation.ConsoleIme.CompStrData;
    }
}

NTSTATUS RemoveConsole(_In_ PCONSOLE_PROCESS_HANDLE ProcessData)
{
    NTSTATUS Status;
    CONSOLE_INFORMATION *Console;
    BOOL fRecomputeOwner;

#pragma prefast(suppress:28931, "Status is not unused. Used by assertions.")
    Status = RevalidateConsole(&Console);
    ASSERT(NT_SUCCESS(Status));

    FreeCommandHistory((HANDLE) ProcessData);

    fRecomputeOwner = ProcessData->RootProcess;
    FreeProcessData(ProcessData);

    if (fRecomputeOwner)
    {
        SetConsoleWindowOwner(g_ciConsoleInformation.hWnd, nullptr);
    }

    UnlockConsole();

    return STATUS_SUCCESS;
}

// Routine Description:
// - This routine is called when a process is destroyed. It closes the process's handles and frees the console if it's the last reference.
// Arguments:
// - hProcess - Process ID.
// Return Value:
// - <none>
VOID ConsoleClientDisconnectRoutine(PCONSOLE_PROCESS_HANDLE ProcessData)
{
    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::FreeConsole);

    NotifyWinEvent(EVENT_CONSOLE_END_APPLICATION, g_ciConsoleInformation.hWnd, HandleToULong(ProcessData->ClientId.UniqueProcess), 0);

    RemoveConsole(ProcessData);
}

DWORD ConsoleIoThread(_In_ HANDLE Server);

DWORD ConsoleInputThread(LPVOID lpParameter);

void ConsoleCheckDebug()
{
#ifdef DBG
    HKEY hCurrentUser;
    HKEY hConsole;
    NTSTATUS status = RegistrySerialization::s_OpenConsoleKey(&hCurrentUser, &hConsole);

    if (NT_SUCCESS(status))
    {
        DWORD dwData = 0;
        status = RegistrySerialization::s_QueryValue(hConsole, L"DebugLaunch", sizeof(dwData), (BYTE*)&dwData, nullptr);

        if (NT_SUCCESS(status))
        {
            if (dwData != 0)
            {
                DebugBreak();
            }
        }

        RegCloseKey(hConsole);
        RegCloseKey(hCurrentUser);
    }
#endif
}

NTSTATUS ConsoleCreateIoThread(_In_ HANDLE Server)
{
    ConsoleCheckDebug();

    HANDLE hThread;
    NTSTATUS Status;

    Status = ConsoleServerInitialization(Server);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    g_hConsoleInputInitEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);

    if (g_hConsoleInputInitEvent == nullptr)
    {
        Status = NTSTATUS_FROM_WIN32(GetLastError());
    }

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    hThread = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)ConsoleIoThread, Server, 0, nullptr);
    if (hThread == nullptr)
    {
        return STATUS_NO_MEMORY;
    }

    CloseHandle(hThread);

    return STATUS_SUCCESS;
}

#define SYSTEM_ROOT         (L"%SystemRoot%")
#define SYSTEM_ROOT_LENGTH  (sizeof(SYSTEM_ROOT) - sizeof(WCHAR))

// Routine Description:
// - This routine translates path characters into '_' characters because the NT registry apis do not allow the creation of keys with
//   names that contain path characters. It also converts absolute paths into %SystemRoot% relative ones. As an example, if both behaviors were
//   specified it would convert a title like C:\WINNT\System32\cmd.exe to %SystemRoot%_System32_cmd.exe.
// Arguments:
// - ConsoleTitle - Pointer to string to translate.
// - Unexpand - Convert absolute path to %SystemRoot% relative one.
// - Substitute - Whether string-substitution ('_' for '\') should occur.
// Return Value:
// - Pointer to translated title or nullptr.
// Note:
// - This routine allocates a buffer that must be freed.
PWSTR TranslateConsoleTitle(_In_ PCWSTR pwszConsoleTitle, _In_ const BOOL fUnexpand, _In_ const BOOL fSubstitute)
{
    LPWSTR Tmp = nullptr;

    size_t cbConsoleTitle;
    size_t cbSystemRoot;

    LPWSTR pwszSysRoot = new wchar_t[MAX_PATH];
    if (nullptr != pwszSysRoot)
    {
        if (0 != GetSystemDirectoryW(pwszSysRoot, MAX_PATH))
        {
            if (SUCCEEDED(StringCbLengthW(pwszConsoleTitle, STRSAFE_MAX_CCH, &cbConsoleTitle)) &&
                SUCCEEDED(StringCbLengthW(pwszSysRoot, STRSAFE_MAX_CCH, &cbSystemRoot)))
            {
                int const cchSystemRoot = (int)(cbSystemRoot / sizeof(WCHAR));
                int const cchConsoleTitle = (int)(cbConsoleTitle / sizeof(WCHAR));
                cbConsoleTitle += sizeof(WCHAR); // account for nullptr terminator

                if (fUnexpand &&
                    cchConsoleTitle >= cchSystemRoot &&
#pragma prefast(suppress:26018, "We've guaranteed that cchSystemRoot is equal to or smaller than cchConsoleTitle in size.")
                    (CSTR_EQUAL == CompareStringOrdinal(pwszConsoleTitle, cchSystemRoot, pwszSysRoot, cchSystemRoot, TRUE)))
                {
                    cbConsoleTitle -= cbSystemRoot;
                    pwszConsoleTitle += cchSystemRoot;
                    cbSystemRoot = SYSTEM_ROOT_LENGTH;
                }
                else
                {
                    cbSystemRoot = 0;
                }

                LPWSTR TranslatedConsoleTitle;
                Tmp = TranslatedConsoleTitle = (PWSTR)new BYTE[cbSystemRoot + cbConsoleTitle];
                if (TranslatedConsoleTitle == nullptr)
                {
                    return nullptr;
                }

                memmove(TranslatedConsoleTitle, SYSTEM_ROOT, cbSystemRoot);
                TranslatedConsoleTitle += (cbSystemRoot / sizeof(WCHAR));   // skip by characters -- not bytes

                for (UINT i = 0; i < cbConsoleTitle; i += sizeof(WCHAR))
                {
#pragma prefast(suppress:26018, "We are reading the null portion of the buffer on purpose and will escape on reaching it below.")
                    if (fSubstitute && *pwszConsoleTitle == '\\')
                    {
#pragma prefast(suppress:26019, "Console title must contain system root if this path was followed.")
                        *TranslatedConsoleTitle++ = (WCHAR)'_';
                    }
                    else
                    {
                        *TranslatedConsoleTitle++ = *pwszConsoleTitle;
                        if (*pwszConsoleTitle == L'\0')
                        {
                            break;
                        }
                    }

                    pwszConsoleTitle++;
                }
            }
        }
        delete[] pwszSysRoot;
    }

    return Tmp;
}

NTSTATUS GetConsoleLangId(_In_ const UINT uiOutputCP, _Out_ LANGID * const pLangId)
{
    NTSTATUS Status = STATUS_NOT_SUPPORTED;

    if (pLangId != nullptr)
    {
        switch (uiOutputCP)
        {
            case CP_JAPANESE:
                *pLangId = MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT);
                break;
            case CP_KOREAN:
                *pLangId = MAKELANGID(LANG_KOREAN, SUBLANG_KOREAN);
                break;
            case CP_CHINESE_SIMPLIFIED:
                *pLangId = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED);
                break;
            case CP_CHINESE_TRADITIONAL:
                *pLangId = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL);
                break;
            default:
                *pLangId = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
                break;
        }
    }
    Status = STATUS_SUCCESS;

    return Status;
}

NTSTATUS SrvGetConsoleLangId(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL /*ReplyPending*/)
{
    PCONSOLE_LANGID_MSG const a = &m->u.consoleMsgL1.GetConsoleLangId;

    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::GetConsoleLangId);

    CONSOLE_INFORMATION *Console;
    NTSTATUS Status = RevalidateConsole(&Console);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = GetConsoleLangId(g_ciConsoleInformation.OutputCP, &a->LangId);

    UnlockConsole();

    return Status;
}

// Routine Description:
// - This routine reads the connection information from a 'connect' IO, validates it and stores them in an internal format.
// - N.B. The internal informat contains information not sent by clients in their connect IOs and intialized by other routines.
// Arguments:
// - Server - Supplies a handle to the console server.
// - Message - Supplies the message representing the connect IO.
// - Cac - Receives the connection information.
// Return Value:
// - NTSTATUS indicating if the connection information was successfully initialized.
NTSTATUS ConsoleInitializeConnectInfo(_In_ PCONSOLE_API_MSG Message, _Out_ PCONSOLE_API_CONNECTINFO Cac)
{
    CONSOLE_SERVER_MSG Data = { 0 };

    // Try to receive the data sent by the client.
    NTSTATUS Status = ReadMessageInput(Message, 0, &Data, sizeof Data);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    // Validate that strings are within the buffers and null-terminated.
    if ((Data.ApplicationNameLength > (sizeof(Data.ApplicationName) - sizeof(WCHAR))) ||
        (Data.TitleLength > (sizeof(Data.Title) - sizeof(WCHAR))) ||
        (Data.CurrentDirectoryLength > (sizeof(Data.CurrentDirectory) - sizeof(WCHAR))) ||
        (Data.ApplicationName[Data.ApplicationNameLength / sizeof(WCHAR)] != UNICODE_NULL) ||
        (Data.Title[Data.TitleLength / sizeof(WCHAR)] != UNICODE_NULL) || (Data.CurrentDirectory[Data.CurrentDirectoryLength / sizeof(WCHAR)] != UNICODE_NULL))
    {
        return STATUS_INVALID_BUFFER_SIZE;
    }

    // Initialize (partially) the connect info with the received data.
    ASSERT(sizeof(Cac->AppName) == sizeof(Data.ApplicationName));
    ASSERT(sizeof(Cac->Title) == sizeof(Data.Title));
    ASSERT(sizeof(Cac->CurDir) == sizeof(Data.CurrentDirectory));

    // unused(Data.IconId)
    Cac->ConsoleInfo.SetHotKey(Data.HotKey);
    Cac->ConsoleInfo.SetStartupFlags(Data.StartupFlags);
    Cac->ConsoleInfo.SetFillAttribute(Data.FillAttribute);
    Cac->ConsoleInfo.SetShowWindow(Data.ShowWindow);
    Cac->ConsoleInfo.SetScreenBufferSize(Data.ScreenBufferSize);
    Cac->ConsoleInfo.SetWindowSize(Data.WindowSize);
    Cac->ConsoleInfo.SetWindowOrigin(Data.WindowOrigin);
    Cac->ProcessGroupId = Data.ProcessGroupId;
    Cac->ConsoleApp = Data.ConsoleApp;
    Cac->WindowVisible = Data.WindowVisible;
    Cac->TitleLength = Data.TitleLength;
    Cac->AppNameLength = Data.ApplicationNameLength;
    Cac->CurDirLength = Data.CurrentDirectoryLength;

    memmove(Cac->AppName, Data.ApplicationName, sizeof(Cac->AppName));
    memmove(Cac->Title, Data.Title, sizeof(Cac->Title));
    memmove(Cac->CurDir, Data.CurrentDirectory, sizeof(Cac->CurDir));

    return STATUS_SUCCESS;
}

NTSTATUS ConsoleAllocateConsole(PCONSOLE_API_CONNECTINFO p)
{
    // AllocConsole is outside our codebase, but we should be able to mostly track the call here.
    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::AllocConsole);

    NTSTATUS Status = SetUpConsole(&p->ConsoleInfo, p->TitleLength, p->Title, p->CurDir, p->AppName);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    if (NT_SUCCESS(Status) && p->WindowVisible)
    {
        HANDLE Thread;

        Thread = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)ConsoleInputThread, nullptr, 0, &g_dwInputThreadId);
        if (Thread == nullptr)
        {
            Status = STATUS_NO_MEMORY;
        }
        else
        {
            // The ConsoleInputThread needs to lock the console so we must first unlock it ourselves.
            UnlockConsole();
            WaitForSingleObject(g_hConsoleInputInitEvent, INFINITE);
            LockConsole();

            CloseHandle(Thread);
            CloseHandle(g_hConsoleInputInitEvent);
            g_hConsoleInputInitEvent = nullptr;

            if (!NT_SUCCESS(g_ntstatusConsoleInputInitStatus))
            {
                Status = g_ntstatusConsoleInputInitStatus;
            }
            else
            {
                Status = STATUS_SUCCESS;
            }

            /*
             * Tell driver to allow clients with UIAccess to connect
             * to this server even if the security descriptor doesn't
             * allow it.
             *
             * N.B. This allows applications like narrator.exe to have
             *      access to the console. This is ok because they already
             *      have access to the console window anyway - this function
             *      is only called when a window is created.
             */

            IoControlFile(g_ciConsoleInformation.Server,
                          IOCTL_CONDRV_ALLOW_VIA_UIACCESS,
                          nullptr,
                          0,
                          nullptr,
                          0);
        }
    }
    else
    {
        g_ciConsoleInformation.Flags |= CONSOLE_NO_WINDOW;
    }

    return Status;
}

PCONSOLE_API_MSG ConsoleHandleConnectionRequest(_In_ HANDLE Server, _Inout_ PCONSOLE_API_MSG ReceiveMsg)
{
    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::AttachConsole);

    PCONSOLE_PROCESS_HANDLE ProcessData = nullptr;

    LockConsole();

    CLIENT_ID ClientId;
    ClientId.UniqueProcess = (HANDLE) ReceiveMsg->Descriptor.Process;
    ClientId.UniqueThread = (HANDLE) ReceiveMsg->Descriptor.Object;

    CONSOLE_API_CONNECTINFO Cac;
    NTSTATUS Status = ConsoleInitializeConnectInfo(ReceiveMsg, &Cac);
    if (!NT_SUCCESS(Status))
    {
        goto Error;
    }

    ProcessData = AllocProcessData(&ClientId, Cac.ProcessGroupId, nullptr);
    if (ProcessData == nullptr)
    {
        Status = STATUS_UNSUCCESSFUL;
        goto Error;
    }

    ProcessData->RootProcess = ((g_ciConsoleInformation.Flags & CONSOLE_INITIALIZED) == 0);

    // ConsoleApp will be false in the AttachConsole case.
    if (Cac.ConsoleApp)
    {
        CONSOLE_PROCESS_INFO cpi;

        cpi.dwProcessID = HandleToUlong(ClientId.UniqueProcess);
        cpi.dwFlags = CPI_NEWPROCESSWINDOW;
        UserPrivApi::s_ConsoleControl(UserPrivApi::CONSOLECONTROL::ConsoleNotifyConsoleApplication, &cpi, sizeof(CONSOLE_PROCESS_INFO));
    }

    if (g_ciConsoleInformation.hWnd)
    {
        NotifyWinEvent(EVENT_CONSOLE_START_APPLICATION, g_ciConsoleInformation.hWnd, HandleToULong(ClientId.UniqueProcess), 0);
    }

    if ((g_ciConsoleInformation.Flags & CONSOLE_INITIALIZED) == 0)
    {
        Status = ConsoleAllocateConsole(&Cac);
        if (!NT_SUCCESS(Status))
        {
            goto Error;
        }

        g_ciConsoleInformation.Flags |= CONSOLE_INITIALIZED;
    }

    AllocateCommandHistory(Cac.AppName, Cac.AppNameLength, (HANDLE)ProcessData);

    if (ProcessData->ProcessHandle != nullptr)
    {
        SetProcessForegroundRights(ProcessData->ProcessHandle, g_ciConsoleInformation.Flags & CONSOLE_HAS_FOCUS);
    }

    // Create the handles.
    Status = AllocateIoHandle(CONSOLE_INPUT_HANDLE,
                              &ProcessData->InputHandle,
                              &g_ciConsoleInformation.pInputBuffer->Header,
                              GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE);

    if (!NT_SUCCESS(Status))
    {
        goto Error;
    }

    Status = AllocateIoHandle(CONSOLE_OUTPUT_HANDLE,
                              &ProcessData->OutputHandle,
                              &g_ciConsoleInformation.CurrentScreenBuffer->GetMainBuffer()->Header,
                              GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE);

    if (!NT_SUCCESS(Status))
    {
        goto Error;
    }

    // Complete the request.
    SetReplyStatus(ReceiveMsg, STATUS_SUCCESS);
    SetReplyInformation(ReceiveMsg, sizeof(CD_CONNECTION_INFORMATION));

    CD_CONNECTION_INFORMATION ConnectionInformation;
    ReceiveMsg->Complete.Write.Data = &ConnectionInformation;
    ReceiveMsg->Complete.Write.Size = sizeof(CD_CONNECTION_INFORMATION);

    ConnectionInformation.Process = (ULONG_PTR) ProcessData;
    ConnectionInformation.Input = (ULONG_PTR) ProcessData->InputHandle;
    ConnectionInformation.Output = (ULONG_PTR) ProcessData->OutputHandle;

    Status = ConsoleComplete(Server, &ReceiveMsg->Complete);
    if (!NT_SUCCESS(Status))
    {
        FreeCommandHistory((HANDLE) ProcessData);
        FreeProcessData(ProcessData);
    }

    UnlockConsole();

    return nullptr;

Error:

    ASSERT(!NT_SUCCESS(Status));

    if (ProcessData != nullptr)
    {
        FreeCommandHistory((HANDLE) ProcessData);
        FreeProcessData(ProcessData);
    }

    UnlockConsole();

    SetReplyStatus(ReceiveMsg, Status);

    return ReceiveMsg;
}

// Routine Description:
// - This routine handles IO requests to create new objects. It validates the request, creates the object and a "handle" to it.
// Arguments:
// - Message - Supplies the message representing the create IO.
// - Console - Supplies the console whose object will be created.
// Return Value:
// - A pointer to the reply message, if this message is to be completed inline; nullptr if this message will pend now and complete later.
PCONSOLE_API_MSG ConsoleCreateObject(_In_ PCONSOLE_API_MSG Message, _Inout_ CONSOLE_INFORMATION *Console)
{
    NTSTATUS Status;

    PCD_CREATE_OBJECT_INFORMATION const CreateInformation = &Message->CreateObject;

    LockConsole();

    // If a generic object was requested, use the desired access to determine which type of object the caller is expecting.
    if (CreateInformation->ObjectType == CD_IO_OBJECT_TYPE_GENERIC)
    {
        if ((CreateInformation->DesiredAccess & (GENERIC_READ | GENERIC_WRITE)) == GENERIC_READ)
        {
            CreateInformation->ObjectType = CD_IO_OBJECT_TYPE_CURRENT_INPUT;

        }
        else if ((CreateInformation->DesiredAccess & (GENERIC_READ | GENERIC_WRITE)) == GENERIC_WRITE)
        {
            CreateInformation->ObjectType = CD_IO_OBJECT_TYPE_CURRENT_OUTPUT;
        }
    }

    HANDLE Handle = nullptr;
    // Check the requested type.
    switch (CreateInformation->ObjectType)
    {
        case CD_IO_OBJECT_TYPE_CURRENT_INPUT:
            Status = AllocateIoHandle(CONSOLE_INPUT_HANDLE, &Handle, &Console->pInputBuffer->Header, CreateInformation->DesiredAccess, CreateInformation->ShareMode);
            break;

        case CD_IO_OBJECT_TYPE_CURRENT_OUTPUT:
        {
            PSCREEN_INFORMATION const ScreenInformation = g_ciConsoleInformation.CurrentScreenBuffer->GetMainBuffer();
            if (ScreenInformation == nullptr)
            {
                Status = STATUS_OBJECT_NAME_NOT_FOUND;

            }
            else
            {
                Status = AllocateIoHandle(CONSOLE_OUTPUT_HANDLE, &Handle, &ScreenInformation->Header, CreateInformation->DesiredAccess, CreateInformation->ShareMode);
            }
            break;
        }
        case CD_IO_OBJECT_TYPE_NEW_OUTPUT:
            Status = ConsoleCreateScreenBuffer(&Handle, Message, CreateInformation, &Message->CreateScreenBuffer);
            break;

        default:
            Status = STATUS_INVALID_PARAMETER;
    }

    if (!NT_SUCCESS(Status))
    {
        goto Error;
    }

    // Complete the request.
    SetReplyStatus(Message, STATUS_SUCCESS);
    SetReplyInformation(Message, (ULONG_PTR) Handle);

    Status = ConsoleComplete(Console->Server, &Message->Complete);
    if (!NT_SUCCESS(Status))
    {
        ConsoleCloseHandle(Handle);
    }

    UnlockConsole();

    return nullptr;

Error:

    ASSERT(!NT_SUCCESS(Status));

    UnlockConsole();

    SetReplyStatus(Message, Status);

    return Message;
}

// Routine Description:
// - This routine validates a user IO and dispatches it to the appropriate worker routine.
// Arguments:
// - Message - Supplies the message representing the user IO.
// Return Value:
// - A pointer to the reply message, if this message is to be completed inline; nullptr if this message will pend now and complete later.
PCONSOLE_API_MSG ConsoleDispatchRequest(_Inout_ PCONSOLE_API_MSG Message)
{
    // Make sure the indices are valid and retrieve the API descriptor.
    ULONG const LayerNumber = (Message->msgHeader.ApiNumber >> 24) - 1;
    ULONG const ApiNumber = Message->msgHeader.ApiNumber & 0xffffff;

    NTSTATUS Status;
    if ((LayerNumber >= RTL_NUMBER_OF(ConsoleApiLayerTable)) || (ApiNumber >= ConsoleApiLayerTable[LayerNumber].Count))
    {
        Status = STATUS_ILLEGAL_FUNCTION;
        goto Complete;
    }

    CONSOLE_API_DESCRIPTOR const *Descriptor = &ConsoleApiLayerTable[LayerNumber].Descriptor[ApiNumber];

    // Validate the argument size and call the API.
    if ((Message->Descriptor.InputSize < sizeof(CONSOLE_MSG_HEADER)) ||
        (Message->msgHeader.ApiDescriptorSize > sizeof(Message->u)) ||
        (Message->msgHeader.ApiDescriptorSize > Message->Descriptor.InputSize - sizeof(CONSOLE_MSG_HEADER)) || (Message->msgHeader.ApiDescriptorSize < Descriptor->RequiredSize))
    {
        Status = STATUS_ILLEGAL_FUNCTION;
        goto Complete;
    }

    BOOL ReplyPending = FALSE;
    Message->Complete.Write.Data = &Message->u;
    Message->Complete.Write.Size = Message->msgHeader.ApiDescriptorSize;
    Message->State.WriteOffset = Message->msgHeader.ApiDescriptorSize;
    Message->State.ReadOffset = Message->msgHeader.ApiDescriptorSize + sizeof(CONSOLE_MSG_HEADER);

    Status = (*Descriptor->Routine) (Message, &ReplyPending);

    if (!ReplyPending)
    {
        goto Complete;
    }

    return nullptr;

Complete:

    SetReplyStatus(Message, Status);

    return Message;
}

// Routine Description:
// - This routine is the main one in the console server IO thread.
// - It reads IO requests submitted by clients through the driver, services and completes them in a loop.
// Arguments:
// - Server - Supplies the console server handle.
// Return Value:
// - This routine never returns. The process exits when no more references or clients exist.
DWORD ConsoleIoThread(_In_ HANDLE Server)
{
    CONSOLE_API_MSG ReceiveMsg;
    PCONSOLE_API_MSG ReplyMsg = nullptr;
    BOOL ReplyPending = FALSE;
    NTSTATUS Status;

    bool fShouldExit = false;
    while (!fShouldExit)
    {
        if (ReplyMsg != nullptr)
        {
            ReleaseMessageBuffers(ReplyMsg);
        }

        Status = IoControlFile(Server,
                               IOCTL_CONDRV_READ_IO,
                               (ReplyMsg == nullptr) ? nullptr : &ReplyMsg->Complete,
                               (ReplyMsg == nullptr) ? 0 : sizeof ReplyMsg->Complete,
                               &ReceiveMsg.Descriptor,
                               sizeof(CONSOLE_API_MSG) - FIELD_OFFSET(CONSOLE_API_MSG, Descriptor));

        if (Status == STATUS_PENDING)
        {
            WaitForSingleObject(Server, INFINITE);
            Status = ReceiveMsg.IoStatus.Status;
        }

        if (!NT_SUCCESS(Status))
        {
            if (Status == STATUS_PIPE_DISCONNECTED ||
                Status == ERROR_PIPE_NOT_CONNECTED ||
                Status == NTSTATUS_FROM_WIN32(ERROR_PIPE_NOT_CONNECTED))
            {
                fShouldExit = true;

                // This will not return. Terminate immediately when disconnected.
                TerminateProcess(GetCurrentProcess(), STATUS_SUCCESS);
            }
            RIPMSG1(RIP_WARNING, "NtReplyWaitReceivePort failed with Status 0x%x", Status);
            ReplyMsg = nullptr;
            continue;
        }

        ZeroMemory(&ReceiveMsg.State, sizeof(ReceiveMsg.State));
        ZeroMemory(&ReceiveMsg.Complete, sizeof(CD_IO_COMPLETE));

        ReceiveMsg.Complete.Identifier = ReceiveMsg.Descriptor.Identifier;

        switch (ReceiveMsg.Descriptor.Function)
        {
            case CONSOLE_IO_USER_DEFINED:
                ReplyMsg = ConsoleDispatchRequest(&ReceiveMsg);
                break;

            case CONSOLE_IO_CONNECT:
                ReplyMsg = ConsoleHandleConnectionRequest(Server, &ReceiveMsg);
                break;

            case CONSOLE_IO_DISCONNECT:
                ConsoleClientDisconnectRoutine(GetMessageProcess(&ReceiveMsg));
                SetReplyStatus(&ReceiveMsg, STATUS_SUCCESS);
                ReplyMsg = &ReceiveMsg;
                break;

            case CONSOLE_IO_CREATE_OBJECT:
                ReplyMsg = ConsoleCreateObject(&ReceiveMsg, &g_ciConsoleInformation);
                break;

            case CONSOLE_IO_CLOSE_OBJECT:
                SrvCloseHandle(&ReceiveMsg);
                SetReplyStatus(&ReceiveMsg, STATUS_SUCCESS);
                ReplyMsg = &ReceiveMsg;
                break;

            case CONSOLE_IO_RAW_WRITE:
                ZeroMemory(&ReceiveMsg.u.consoleMsgL1.WriteConsole, sizeof(CONSOLE_WRITECONSOLE_MSG));

                ReplyPending = FALSE;
                Status = SrvWriteConsole(&ReceiveMsg, &ReplyPending);
                if (ReplyPending)
                {
                    ReplyMsg = nullptr;

                }
                else
                {
                    SetReplyStatus(&ReceiveMsg, Status);
                    ReplyMsg = &ReceiveMsg;
                }
                break;

            case CONSOLE_IO_RAW_READ:
                ZeroMemory(&ReceiveMsg.u.consoleMsgL1.ReadConsole, sizeof(CONSOLE_READCONSOLE_MSG));
                ReceiveMsg.u.consoleMsgL1.ReadConsole.ProcessControlZ = TRUE;
                ReplyPending = FALSE;
                Status = SrvReadConsole(&ReceiveMsg, &ReplyPending);
                if (ReplyPending)
                {
                    ReplyMsg = nullptr;

                }
                else
                {
                    SetReplyStatus(&ReceiveMsg, Status);
                    ReplyMsg = &ReceiveMsg;
                }
                break;

            case CONSOLE_IO_RAW_FLUSH:
                ReplyPending = FALSE;
                Status = SrvFlushConsoleInputBuffer(&ReceiveMsg, &ReplyPending);
                ASSERT(!ReplyPending);
                SetReplyStatus(&ReceiveMsg, Status);
                ReplyMsg = &ReceiveMsg;
                break;

            default:
                SetReplyStatus(&ReceiveMsg, STATUS_UNSUCCESSFUL);
                ReplyMsg = &ReceiveMsg;
        }
    }

    return 0;
}

NTSTATUS SrvDeprecatedAPI(_Inout_ PCONSOLE_API_MSG /*m*/, _Inout_ PBOOL /*ReplyPending*/)
{
    // assert if we hit a deprecated API.
    ASSERT(TRUE);

    // One common aspect of the functions we deprecate is that they all RevalidateConsole and then UnlockConsole at the
    // end. Here we do the same thing to more closely emulate the old functions.
    CONSOLE_INFORMATION *Console;
    NTSTATUS Status = RevalidateConsole(&Console);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    UnlockConsole();
    return STATUS_UNSUCCESSFUL;
}
