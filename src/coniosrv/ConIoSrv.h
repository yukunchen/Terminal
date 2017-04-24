/*++
Copyright (c) Microsoft Corporation

Module Name:
- ConIoSrv.h

Abstract:
- This header defines values, types, and structures that this server and the
  OneCore interactivity library for conhost.exe share.
- Technically, conhost.exe does not need everything that is in this header, so
  we could split it in two.

Author:
- HeGatta Mar.16.2017
--*/

//
//                        !!! ------- !!!
//                        !!! WARNING !!!
//                        !!! ------- !!!
//
// Running code inside a system-critical process, as is CSRSS, means that ANY
// mistakes will immediately bugcheck the system. Also, CSRSS server DLL's
// are native binaries (not Windows nor Console) and link only against NTDLL
// (it's not a hard requirement as far as I know).
// 

#pragma once

#include <ntlpcapi.h>

//
// Full object manager path to the ALPC connection port. Clients use this to
// connect to us and send us requests.
//

#define CIS_ALPC_PORT_NAME L"\\ConsoleInputServerPort"

//
// Event and Message types (see CIS_MSG and CIS_EVENT structures below).
//

#define CIS_EVENT_TYPE_INPUT         (0)
#define CIS_EVENT_TYPE_FOCUS         (1)

#define CIS_MSG_TYPE_MAPVIRTUALKEY   (0)
#define CIS_MSG_TYPE_VKKEYSCAN       (1)
#define CIS_MSG_TYPE_GETKEYSTATE     (2)

#define CIS_MSG_TYPE_GETDISPLAYSIZE  (3)
#define CIS_MSG_TYPE_GETFONTSIZE     (4)
#define CIS_MSG_TYPE_SETCURSOR       (5)
#define CIS_MSG_TYPE_UPDATEDISPLAY   (6)

#define CIS_MSG_ATTR_FLAGS           (ALPC_FLG_MSG_CONTEXT_ATTR  | \
                                      ALPC_FLG_MSG_DATAVIEW_ATTR | \
                                      ALPC_FLG_MSG_HANDLE_ATTR)

//
// Length of the ALPC message attribute buffer.
//
// We use the following message attributes:
//
// 1. Context attribute:
//    We use this to have ALPC hand us a pointer with each message. We set this
//    pointer to be a pointer to the client context structure for the client
//    sending the request so we don't have to look it up everytime. This pointer
//    is set at connection time.
//
// 2. Data View Attribute:
//    We use this to create a shared view between us and a client to pass
//    rendering buffers since they are larger than the largest message size
//    allowed by ALPC.
//
// 3. Handle Attribute:
//    We use this to pass the pipe read handle to a new client. The client
//    receives generic read access to the handle.
//
// We have to also allocate some space for the attribute buffer descriptor,
// ALPC_MESSAGE_ATTRIBUTES.
//

#define CIS_MSG_ATTR_BUFFER_SIZE     (sizeof(ALPC_MESSAGE_ATTRIBUTES) \
                                    + sizeof(ALPC_CONTEXT_ATTR)       \
                                    + sizeof(ALPC_DATA_VIEW_ATTR)     \
                                    + sizeof(ALPC_HANDLE_ATTR))

//
// Maximum size of the view shared between server and client.
//
// N.B.: The shared view is solely used to pass console character buffers for
//       rendering. This value accomodates at least 80 characters per row. If
//       at some point we support more columns, this value may have to be
//       increased.
//
// N.B.: Ideally we'd want to compute this value dynamically once we detect the
//       display size (in characters). Then, we'd set that as the maximum view
//       size for the server communication port for each client. However, the
//       requested view size is rounded up by the memory manager when we ask
//       ALPC to map the view. Thus, the maximum allowed view size will be
//       smaller than what we actually request, and the mapping will fail.
//

#define CIS_SHARED_VIEW_SIZE         (0x10000)

//
// The CIS_CLIENT structure holds the information needed to deliver events to
// any one client application. Client applications are instances of conhost.exe
// running on OneCore editions to which this CSRSS server DLL delivers keyboard
// input as well as notifications letting them know when they have become the
// foreground console application, or whether they are now in the background.
//
// N.B.: The LIST_ENTRY member must be in the first position so that a pointer
//       to this structure can be cast into PLIST_ENTRY, if only for mere
//       convenience.
//

typedef struct {
    
    //
    // This member must be first so that the structure can be addressed as a
    // LIST_ENTRY.
    //

    LIST_ENTRY List;

    //
    // ProcessHandle is the handle to the process object, while UniqueProcessId
    // is a number that uniquely represents the client process with ALPC and
    // is, as far as I know, entirely unrelated to the actual process object.
    // I believe the idea is to separate the process handle itself, which is a
    // securable object and can't just be passed around without access checks,
    // and some way to unique identify processes (and threads) without
    // compromising security.
    //
    // ProcessHandle is useful when registering a client, while UniqueProcessId
    // is useful as a key into the list since all ALPC messages carry it.
    //

    HANDLE ProcessHandle;
    HANDLE UniqueProcessId;
    HANDLE ServerCommunicationPort;

    HANDLE PipeReadHandle;
    HANDLE PipeWriteHandle;

    BOOLEAN IsNextDisplayUpdateFocusRedraw;
    
    struct {
        HANDLE Handle;
        SIZE_T Size;
        PVOID ViewBase;
    } SharedSection;
    
} CIS_CLIENT, *PCIS_CLIENT;

//
// The CIS_CONTEXT structure holds global information used by this CSRSS server
// DLL. Instead of having several, unrelated global variables, we only have
// this one. It is allocated in the heap upon server DLL initialiation. All
// access should be appropriately serialized via the ContextLock memeber, as
// this structure may be used by multiple threads simultaneouly. Since we are
// using a Slim Reader/Writer lock, SRW semantics apply (i.e. acquire exclusive
// only when necessary).
//

typedef struct {
    HANDLE ServerPort;

    PCIS_CLIENT ActiveClient;  // Doubly-linked List
    
    struct {
        HANDLE Handle;
        ULONG Height;
        ULONG Width;
    } Display;
    
    RTL_SRWLOCK ContextLock;
} CIS_CONTEXT, *PCIS_CONTEXT;

//
// This Server DLL and each of its clients (instances of conhost.exe)
// communicate in two different ways:
//
// 1. An ALPC API Port;
// 2. An Anonymous Pipe.
//
// When this server DLL starts, it creates an ALPC API port and listens for
// connection requests. When conhost.exe starts under OneCore, it tries to
// connect to that API port. If this server determines that it should accept the
// connection, the connection is established. This server DLL keeps a record of
// this new client in the server context global. When this server processes the
// connection request, it creates an anonymous pipe and passes the pipe read
// handle to the client, while keeping the write handle to itself. ALPC provides
// a handle passing and duplication mechanism via message attributes, and the
// passing of the handle is trivial.
//
// As part of the determination of whether to accept a connection from a client,
// this server DLL asserts that the client connecting is indeed conhost.exe. For
// details on this verification, see Trust.h.
//
// When the client makes an API request to this server via the ALPC API port, it
// sends a message whose structure is CIS_MSG, defined below. Because we limit
// the maximum message length, both for security and convenience reasons (it's
// easier to deal with fixed-sized buffers), we define a single structure that
// has enough room for every type of message (i.e. every type of API request).
// This is done via a union of all message types. One such message type may be
// requesting this server to render to the screen on behalf of a client, or a
// user32 keyboard-related API that we implement. The message type is indicated
// via the Type member, and the API parameters are specified in the
// corresponding inner structure, embedded in the union. Using a big union as
// ALPC messages is the method used by CSRSS, WER (Windows Error Reporting), and
// others.
//
// Note that the API port is shared by all clients, while each client gets its
// own pipe. Also, this server DLL only listens to the port on a single thread,
// although multithreading with ALPC is possible, albeit somewhat terse. This
// means that having dozens of console applications running may cause
// performance issues. This isn't a big deal for Nano Server, and shouldn't be
// generally, but it should be kept in mind.
//
// The pipe, on the other hand, is used by this server to send keyboard input
// and focus/unfocus events to clients. Whenever an input event arrives from the
// keyboard on the raw input thread via an APC callback, it looks up the
// currently active client, if any, and dispatches a message via the pipe to
// said client (see CIS_EVENT structure below). The same occurs when the user
// Alt+Tabs to another client and we need to let it know to render. We also
// dispatch a focus lost event to the previously active client to ask it to no
// longer issue render calls. Note though that every render call by a client
// that is not the currently active one is ignored.
//
// |---------------------|         |-----------------------------|
// |  CSRSS              |         |  conhost.exe #1             |
// |                     |         |                             |
// |    ConIoSrv.dll     |    |--->|  Client Communication Port  |
// |           API Port  |<---| |->|  Pipe Read Handle           |
// |                     |    | |  |-----------------------------|
// |            Pipe #1  |----^-|
// |                ...  |    |
// |                ...  |    |    |-----------------------------|
// |            Pipe #N  |--| |    |  conhost.exe #N             |
// |---------------------|  | |    |                             |
//                          | |--->|  Client Communication Port  |
//                          |----->|  Pipe Read Handle           |
//                                 |-----------------------------|
//

typedef struct {
    PORT_MESSAGE AlpcHeader;

    //
    // Type of message we have just received.
    //

    UCHAR Type;

    //
    // We set this if the API was successfully invoked.
    //
    // N.B.: This value is set regardless of whether the actual API
    //       call succeeded or failed. This value merely means that the
    //       client requested a valid API, and the API was called. In
    //       other words, if the client asks for an invalid API, the
    //       reply message will have this set to FALSE, and TRUE otherwise.
    //

    BOOL IsSuccessful;

    //
    // The 'Request' union holds the parameter lists for each of the
    // privileged functions that this server implements. On Windows
    // editions that have user32.dll, these functions are implemented
    // there. Since these functions require access to hardware state, on
    // editions with user32.dll, these are actually implemented in kernel
    // mode (userk). For us who don't have user32.dll and userk, the
    // hardware state information is held by us, and thus we must implement
    // these functions ourselves. The client provides the parameters and we
    // fill in the return value, then reply.
    //
    // N.B.: None of the parameters are _Out_.
    //
    // Direction: Client -> Server (Always ; Server always replies).
    //

    union {
        
        //
        // Windows API's.
        //

        struct {

            //
            // WINUSERAPI
            // UINT
            // WINAPI
            // MapVirtualKeyW(
            //     _In_ UINT uCode,
            //     _In_ UINT uMapType);
            //

            UINT Code;
            UINT MapType;

            UINT ReturnValue;
        } MapVirtualKeyParams;

        struct {

            //
            // WINUSERAPI
            // SHORT
            // WINAPI
            // VkKeyScanW(
            //    _In_ WCHAR ch);
            //

            WCHAR Character;

            SHORT ReturnValue;
        } VkKeyScanParams;

        struct {

            //
            // WINUSERAPI
            // SHORT
            // WINAPI
            // GetKeyState(
            //    _In_ int nVirtKey);
            //

            int VirtualKey;

            SHORT ReturnValue;
        } GetKeyStateParams;
        
        //
        // Console-related API's.
        //

        struct {
            CD_IO_DISPLAY_SIZE DisplaySize;
                
            NTSTATUS ReturnValue;
        } GetDisplaySizeParams;

        struct {
            CD_IO_FONT_SIZE FontSize;
                
            NTSTATUS ReturnValue;
        } GetFontSizeParams;
            
        struct {
            CD_IO_CURSOR_INFORMATION CursorInformation;
                
            NTSTATUS ReturnValue;
        } SetCursorParams;
            
        struct {
            SHORT RowIndex;

            NTSTATUS ReturnValue;
        } UpdateDisplayParams;
    };
} CIS_MSG, *PCIS_MSG;

typedef struct {

    UCHAR Type;

    //
    // The 'Event' union contains data that the server sends to the client
    // when either an input event ('InputEvent' structure) arrives, or when
    // the user has switched console applications via Alt+Tab ('FocusEvent'
    // structure).
    //
    // Direction: Server -> Client (Always ; Client never replies).
    //

    union {

        //
        // A keyboard event was captured by the server and is being sent to
        // the currently active client.
        //

        struct {
            INPUT_RECORD Record;
        } InputEvent;

        //
        // The user has Alt+Tabbed to a different console application. A
        // pair of focus events is sent: one to the previously active
        // application to let it know to let go of the display
        // (IsActive = FALSE), and one to the newly active client application
        // to let it know it must start rendering (IsActive = TRUE).
        //

        struct {
            BOOLEAN IsActive;
        } FocusEvent;
    };
} CIS_EVENT, *PCIS_EVENT;

//
// Defined at the top of Server.c.
//

extern PCIS_CONTEXT g_CisContext;

//
// Convenience macros.
//

#define NT_ALLOCSUCCESS(x) (((x) == NULL) ? STATUS_NO_MEMORY : STATUS_SUCCESS)
