//----------------------------------------------------------------------------------------------------------------------
// <copyright file="CmdApp.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>Helper and wrapper for generating the base test application and its UI Root node.</summary>
//----------------------------------------------------------------------------------------------------------------------
namespace Conhost.UIA.Tests.Elements
{
    using System;
    using System.Diagnostics;
    using System.IO;

    using Conhost.UIA.Tests.Common;
    using Conhost.UIA.Tests.Common.NativeMethods;

    using OpenQA.Selenium;
    using OpenQA.Selenium.Remote;
    using OpenQA.Selenium.Appium;
    using OpenQA.Selenium.Appium.iOS;
    using OpenQA.Selenium.Interactions;

    using WEX.Logging.Interop;
    using WEX.TestExecution;

    using System.Runtime.InteropServices;
    using System.Drawing;
    using System.Text;

    public class CmdApp : IDisposable
    {
        private static readonly string binPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.System), "cmd.exe");
        private static readonly string linkPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData),
                                                               @"Microsoft\Windows\Start Menu\Programs\System Tools\Command Prompt.lnk");

        protected const string AppDriverUrl = "http://127.0.0.1:4723";

        private Process commandProcess;
        public IOSDriver<IOSElement> Session { get; private set; }
        public Actions Actions { get; private set; }
        public AppiumWebElement UIRoot { get; private set; }

        private IntPtr hStdOut = IntPtr.Zero;
        private bool isDisposed = false;

        public CmdApp(CreateType type)
        {
            this.CreateCmdProcess(type);
        }

        public CmdApp(string path)
        {
            this.CreateCmdProcess(path);
        }

        ~CmdApp()
        {
            this.Dispose(false);
        }

        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        public AppiumWebElement GetTitleBar()
        {
            return this.UIRoot.FindElementByAccessibilityId("TitleBar");
        }

        public IntPtr GetStdOutHandle()
        {
            return hStdOut;
        }

        public bool IsVirtualTerminalEnabled(IntPtr hConsoleOutput)
        {
            WinCon.CONSOLE_OUTPUT_MODES modes;

            NativeMethods.Win32BoolHelper(WinCon.GetConsoleMode(hConsoleOutput, out modes), "Retrieve console output mode flags.");

            return (modes & WinCon.CONSOLE_OUTPUT_MODES.ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
        }

        public string GetWindowTitle()
        {
            int size = 100;
            StringBuilder builder = new StringBuilder(size);

            WinCon.GetConsoleTitle(builder, size);

            return builder.ToString();
        }

        public IntPtr GetWindowHandle()
        {
            return WinCon.GetConsoleWindow();
        }

        public WinCon.COORD GetCursorPosition(IntPtr hConsole)
        {
            WinCon.CONSOLE_SCREEN_BUFFER_INFO_EX sbiex = GetScreenBufferInfo(hConsole);

            return sbiex.dwCursorPosition;
        }

        public void FillCursorPosition(IntPtr hConsole, ref Point pt)
        {
            WinCon.COORD coord = GetCursorPosition(hConsole);
            pt.X = coord.X;
            pt.Y = coord.Y;
        }

        public bool IsCursorVisible(IntPtr hConsole)
        {
            WinCon.CONSOLE_CURSOR_INFO cursorInfo = GetCursorInfo(hConsole);
            return cursorInfo.bVisible;
        }

        public WinCon.CONSOLE_ATTRIBUTES GetCurrentAttributes(IntPtr hConsole)
        {
            WinCon.CONSOLE_SCREEN_BUFFER_INFO_EX sbiex = GetScreenBufferInfo(hConsole);
            return sbiex.wAttributes;
        }

        public WinCon.SMALL_RECT GetViewport(IntPtr hConsole)
        {
            WinCon.CONSOLE_SCREEN_BUFFER_INFO_EX sbiex = GetScreenBufferInfo(hConsole);

            return sbiex.srWindow;
        }

        public WinCon.CONSOLE_CURSOR_INFO GetCursorInfo(IntPtr hConsole)
        {
            WinCon.CONSOLE_CURSOR_INFO cursorInfo = new WinCon.CONSOLE_CURSOR_INFO();
            NativeMethods.Win32BoolHelper(WinCon.GetConsoleCursorInfo(hConsole, out cursorInfo), "Get cursor display information.");
            return cursorInfo;
        }

        public WinCon.CONSOLE_SCREEN_BUFFER_INFO_EX GetScreenBufferInfo(IntPtr hConsole)
        {
            WinCon.CONSOLE_SCREEN_BUFFER_INFO_EX sbiex = new WinCon.CONSOLE_SCREEN_BUFFER_INFO_EX();
            sbiex.cbSize = (uint)Marshal.SizeOf(sbiex);
            NativeMethods.Win32BoolHelper(WinCon.GetConsoleScreenBufferInfoEx(hConsole, ref sbiex), "Get screen buffer info for cursor position.");
            return sbiex;
        }

        protected virtual void Dispose(bool disposing)
        {
            if (!this.isDisposed)
            {
                // ensure we're exited when this is destroyed or disposed of explicitly
                this.ExitCmdProcess();
                this.isDisposed = true;
            }
        }

        private void CreateCmdProcess(string path)
        {
            //WindowOpenedWaiter waiter = new WindowOpenedWaiter(UICondition.CreateFromClassName("ConsoleWindowClass"));

            Log.Comment("Attempting to launch command-line application at '{0}'", path);

            this.commandProcess = Process.Start(path, "/k \"TITLE test window\"");

            Globals.WaitForTimeout();

            int pid = this.commandProcess.Id;

            // Free any attached consoles and attach to the console we just created.
            // The driver will bind our calls to the Console APIs into the child process.
            // This will allow us to use the APIs to get/set the console state of the test window.
            NativeMethods.Win32BoolHelper(WinCon.FreeConsole(), "Free existing console bindings.");
            NativeMethods.Win32BoolHelper(WinCon.AttachConsole((uint)pid), "Bind to the new PID for console APIs.");

            DesiredCapabilities appCapabilities = new DesiredCapabilities();
            appCapabilities.SetCapability("app", @"Root");
            Session = new IOSDriver<IOSElement>(new Uri(AppDriverUrl), appCapabilities);
            Session.Manage().Timeouts().ImplicitlyWait(TimeSpan.FromSeconds(15));
            Verify.IsNotNull(Session);
            Actions = new Actions(Session);
            Verify.IsNotNull(Session);

            Globals.WaitForTimeout();
            Globals.WaitForTimeout();
            Globals.WaitForTimeout();

            this.UIRoot = Session.FindElementByName("test window");
            this.hStdOut = WinCon.GetStdHandle(WinCon.CONSOLE_STD_HANDLE.STD_OUTPUT_HANDLE);
            Verify.IsNotNull(this.hStdOut, "Ensure output handle is valid.");
        }

        private void CreateCmdProcess(CreateType type)
        {
            string path = string.Empty;

            switch (type)
            {
                case CreateType.ProcessOnly:
                    path = binPath;
                    break;
                case CreateType.ShortcutFile:
                    path = linkPath;
                    break;
                default:
                    throw new NotImplementedException(AutoHelpers.FormatInvariant("CreateType '{0}' not implemented.", type.ToString()));
            }

            this.CreateCmdProcess(path);
        }

        private void ExitCmdProcess()
        {
            // Release attachment to the child process console. 
            WinCon.FreeConsole();

            if (this.commandProcess != null && !this.commandProcess.HasExited)
            {
                this.commandProcess.Kill();
            }
            this.UIRoot = null;
            this.commandProcess = null;
        }
    }
}
