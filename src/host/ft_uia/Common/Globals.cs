//----------------------------------------------------------------------------------------------------------------------
// <copyright file="Globals.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>Console UI Automation global settings</summary>
//----------------------------------------------------------------------------------------------------------------------
namespace Conhost.UIA.Tests.Common
{
    using System;

    public static class Globals
    {
        public const int Timeout = 500; // in milliseconds
        public const int AppCreateTimeout = 3000; // in milliseconds

        public static void WaitForTimeout()
        {
            System.Threading.Thread.Sleep(Globals.Timeout);
        }
    }

    public enum OpenTarget
    {
        Defaults,
        Specifics
    }

    public enum CreateType
    {
        ProcessOnly,
        ShortcutFile
    }
}
