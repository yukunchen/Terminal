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
    using WEX.TestExecution.Markup;

    using System.Runtime.InteropServices;
    using System.Drawing;
    using System.Text;
    using System.Threading.Tasks;
    using System.Threading;

    public class CmdApp : IDisposable
    {
        private static readonly string binPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.System), "cmd.exe");
        private static readonly string linkPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData),
                                                               @"Microsoft\Windows\Start Menu\Programs\System Tools\Command Prompt.lnk");

        protected const string AppDriverUrl = "http://127.0.0.1:4723";

        private IntPtr job;
        private int pid;
        
        public IOSDriver<IOSElement> Session { get; private set; }
        public Actions Actions { get; private set; }
        public AppiumWebElement UIRoot { get; private set; }

        private IntPtr hStdOut = IntPtr.Zero;
        private bool isDisposed = false;

        private TestContext context;

        public CmdApp(CreateType type, TestContext context)
        {
            this.context = context;
            this.CreateCmdProcess(type);
        }

        public CmdApp(string path, TestContext context)
        {
            this.context = context;
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

        public void SetWrapState(bool WrapOn)
        {
            // Go to property sheet and make sure that wrap is set
            using (PropertiesDialog props = new PropertiesDialog(this))
            {
                props.Open(OpenTarget.Specifics);
                using (Tabs tabs = new Tabs(props))
                {
                    tabs.SetWrapState(WrapOn);
                }
                props.Close(PropertiesDialog.CloseAction.OK);
            }
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

        public int GetPid()
        {
            return pid;
        }

        public int GetRowsPerScroll()
        {
            uint rows = 0;
            NativeMethods.Win32BoolHelper(User32.SystemParametersInfo(User32.SPI.SPI_GETWHEELSCROLLLINES, 0, ref rows, 0), "Retrieve rows per click.");

            return (int)rows;
        }

        public int GetColsPerScroll()
        {
            uint cols = 0;
            NativeMethods.Win32BoolHelper(User32.SystemParametersInfo(User32.SPI.SPI_GETWHEELSCROLLCHARACTERS, 0, ref cols, 0), "Retrieve cols per click.");

            return (int)cols;
        }
        
        public void ScrollWindow(int scrolls = -1)
        {
            User32.SendMessage(WinCon.GetConsoleWindow(), User32.WindowMessages.WM_MOUSEWHEEL, (User32.WHEEL_DELTA * scrolls) << 16, IntPtr.Zero);
        }

        public void HScrollWindow(int scrolls = -1)
        {
            User32.SendMessage(WinCon.GetConsoleWindow(), User32.WindowMessages.WM_MOUSEHWHEEL, (User32.WHEEL_DELTA * scrolls) << 16, IntPtr.Zero);
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

        public WinCon.CONSOLE_SCREEN_BUFFER_INFO_EX GetScreenBufferInfo()
        {
            return GetScreenBufferInfo(GetStdOutHandle());
        }

        public WinCon.CONSOLE_SCREEN_BUFFER_INFO_EX GetScreenBufferInfo(IntPtr hConsole)
        {
            WinCon.CONSOLE_SCREEN_BUFFER_INFO_EX sbiex = new WinCon.CONSOLE_SCREEN_BUFFER_INFO_EX();
            sbiex.cbSize = (uint)Marshal.SizeOf(sbiex);
            NativeMethods.Win32BoolHelper(WinCon.GetConsoleScreenBufferInfoEx(hConsole, ref sbiex), "Get screen buffer info for cursor position.");
            return sbiex;
        }

        public void SetScreenBufferInfo(WinCon.CONSOLE_SCREEN_BUFFER_INFO_EX sbiex)
        {
            SetScreenBufferInfo(GetStdOutHandle(), sbiex);
        }

        public void SetScreenBufferInfo(IntPtr hConsole, WinCon.CONSOLE_SCREEN_BUFFER_INFO_EX sbiex)
        {
            NativeMethods.Win32BoolHelper(WinCon.SetConsoleScreenBufferInfoEx(hConsole, ref sbiex), "Set screen buffer info with given data.");
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
            string WindowTitleToFind = "Host.Tests.UIA window under test";

            job = WinBase.CreateJobObject(IntPtr.Zero, IntPtr.Zero);
            NativeMethods.Win32NullHelper(job, "Creating job object to hold binaries under test.");

            Log.Comment("Attempting to launch command-line application at '{0}'", path);

            string openConsolePath = Path.Combine(this.context.TestDeploymentDir, "OpenConsole.exe");
            string binaryToRunPath = path;

            string launchArgs = $"{openConsolePath}  {binaryToRunPath}";

            WinBase.STARTUPINFO si = new WinBase.STARTUPINFO();
            si.cb = Marshal.SizeOf(si);

            WinBase.PROCESS_INFORMATION pi = new WinBase.PROCESS_INFORMATION();

            NativeMethods.Win32BoolHelper(WinBase.CreateProcess(null,
                launchArgs,
                IntPtr.Zero,
                IntPtr.Zero,
                false,
                WinBase.CP_CreationFlags.CREATE_NEW_CONSOLE | WinBase.CP_CreationFlags.CREATE_SUSPENDED,
                IntPtr.Zero,
                null,
                ref si,
                out pi),
                "Attempting to create child host window process.");

            Log.Comment($"Host window PID: {pi.dwProcessId}");

            NativeMethods.Win32BoolHelper(WinBase.AssignProcessToJobObject(job, pi.hProcess), "Assigning new host window (suspended) to job object.");
            NativeMethods.Win32BoolHelper(-1 != WinBase.ResumeThread(pi.hThread), "Resume host window process now that it is attached and its launch of the child application will be caught in the job object.");

            Globals.WaitForTimeout();

            WinBase.JOBOBJECT_BASIC_PROCESS_ID_LIST list = new WinBase.JOBOBJECT_BASIC_PROCESS_ID_LIST();
            list.NumberOfAssignedProcesses = 2;

            int listptrsize = Marshal.SizeOf(list);
            IntPtr listptr = Marshal.AllocHGlobal(listptrsize);
            Marshal.StructureToPtr(list, listptr, false);

            TimeSpan totalWait = TimeSpan.Zero;
            TimeSpan waitLimit = TimeSpan.FromSeconds(30);
            TimeSpan pollInterval = TimeSpan.FromMilliseconds(500);
            while (totalWait < waitLimit)
            {
                WinBase.QueryInformationJobObject(job, WinBase.JOBOBJECTINFOCLASS.JobObjectBasicProcessIdList, listptr, listptrsize, IntPtr.Zero);
                list = (WinBase.JOBOBJECT_BASIC_PROCESS_ID_LIST)Marshal.PtrToStructure(listptr, typeof(WinBase.JOBOBJECT_BASIC_PROCESS_ID_LIST));

                if (list.NumberOfAssignedProcesses > 1)
                {
                    break;
                }
                else if (list.NumberOfAssignedProcesses < 1)
                {
                    Verify.Fail("Somehow we lost the one console host process in the job already.");
                }

                Thread.Sleep(pollInterval);
                totalWait += pollInterval;
            }
            Verify.IsLessThan(totalWait, waitLimit);

            WinBase.QueryInformationJobObject(job, WinBase.JOBOBJECTINFOCLASS.JobObjectBasicProcessIdList, listptr, listptrsize, IntPtr.Zero);
            list = (WinBase.JOBOBJECT_BASIC_PROCESS_ID_LIST)Marshal.PtrToStructure(listptr, typeof(WinBase.JOBOBJECT_BASIC_PROCESS_ID_LIST));

            Verify.AreEqual(list.NumberOfAssignedProcesses, list.NumberOfProcessIdsInList);

            // Take whichever PID isn't the host window's PID as the child.
            pid = pi.dwProcessId == (int)list.ProcessId ? (int)list.ProcessId2 : (int)list.ProcessId;
            Log.Comment($"Child command app PID: {pid}");

            // Free any attached consoles and attach to the console we just created.
            // The driver will bind our calls to the Console APIs into the child process.
            // This will allow us to use the APIs to get/set the console state of the test window.
            NativeMethods.Win32BoolHelper(WinCon.FreeConsole(), "Free existing console bindings.");
            NativeMethods.Win32BoolHelper(WinCon.AttachConsole((uint)pid), "Bind to the new PID for console APIs.");

            NativeMethods.Win32BoolHelper(WinCon.SetConsoleTitle(WindowTitleToFind), "Set the window title so AppDriver can find it.");

            DesiredCapabilities appCapabilities = new DesiredCapabilities();
            appCapabilities.SetCapability("app", @"Root");
            Session = new IOSDriver<IOSElement>(new Uri(AppDriverUrl), appCapabilities);
            Session.Manage().Timeouts().ImplicitlyWait(TimeSpan.FromSeconds(15));
            Verify.IsNotNull(Session);
            Actions = new Actions(Session);
            Verify.IsNotNull(Session);

            Globals.WaitForTimeout();
            
            this.UIRoot = Session.FindElementByName(WindowTitleToFind);
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

            this.UIRoot = null;

            if (this.job != IntPtr.Zero)
            {
                WinBase.TerminateJobObject(this.job, 0);
            }
            this.job = IntPtr.Zero;
        }

        public WinEventSystem AttachWinEventSystem(IWinEventCallbacks callbacks)
        {
            return new WinEventSystem(callbacks, (uint)this.pid);
        }
    }
}
