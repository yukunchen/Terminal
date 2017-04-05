/*++
Copyright (c) Microsoft Corporation

Module Name:
- Trust.h

Abstract:
- Assert that a connecting client application's on-disk image is trusted via a
  Microsoft root CA on the local machine.

Author:
- HeGatta Mar.16.2017
--*/

#pragma once

//
// This server DLL offers input and rendering services similar in overall nature
// to NtUser and GDI on big Windows. On big Windows, when an application starts,
// it may receive input if it is the foreground application, and is allowed to
// render to the screen, where the rendering is managed by the system. This
// server DLL behaves in the same fashion, where a client application may
// receive input if it is the foreground client application, and is allowed to
// render to the screen so long as it stays the foreground application. Since
// application switchiting is handled by this server DLL, this server retains
// control of what application it sends input to, and the user of the system is
// at liberty to switch to another application at any time. Similary, this
// server DLL retains control of the screen, rendering to it only on behalf of
// its clients.
//
// Nevertheless, this server DLL is not intended to be a OneCore replacement for
// the input and rendering subsystems on big Windows. It is intended only to
// provide input and rendering services to console applications. This means that
// we would like to assert that if an application requests these services from
// us, we can at least assert that it is part of the OS and we, Microsoft,
// control it. To achieve this, we do the following when an application attempts
// to connect to us (remember that we retain the ability to reject a connection
// given the design of ALPC):
//
// 1. We open a handle to the client process with at least query privileges;
// 2. We query for the full path to the process' on-disk image;
// 3. We run WinVerifyTrust against the image. WinVerifyTrust asserts that the
//    image is signed, and that the signature chains up to a root certificate
//    authority trusted by the computer.
// 4. WinVerifyTrust exposes the certification chain it constructs as part of
//    its verification process. We pick it up and call an internal helper
//    function that asserts that the root certificate is also tied to Microsoft.
//
// As a result, WinVerifyTrust helps us assert that the executable file
// correspoding to the client process attempting to connect to us was signed
// with a certificate from which a chain of trust can be established up to a
// trusted root CA. Additionally, we assert that said root CA is a Microsoft CA.
//
// N.B.: This does not mean that the client is conhost.exe. Rather, this means
//       that the client was signed by Microsoft.
//
// N.B.: There might be ways to circumvent these checks:
//       a. There might be a way for a live process to fake its executable path;
//       b. There might be a way to redirect I/O into the on-disk executable
//          image to point to a valid Microsoft-signed executable that is not
//          the real executable the client process was launched from (this would
//          most likely require a kernel-mode driver to be injected into the
//          system, at which point the system cannot be trusted in any case).
//       c. If the client process was hijacked after launch, verifying its
//          on-disk image does us no good.
//
// N.B.: There might be a perceivable delay on first connect, when
//       WinVerifyTrust runs against an image for the first time.
//
// N.B.: MSFT: 11426025 - This check is currently not run, pending a full build
//       where the console's signing catalog exists and can be checked against.
//

BOOLEAN
TstIsFileTrusted(
    _In_reads_z_(MAX_PATH) LPCWSTR FilePath
);
