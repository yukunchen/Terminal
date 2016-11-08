using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Security.Principal;
using System.Text;
using System.Threading.Tasks;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

namespace Host.Tests.UIA
{
    [TestClass]
    class Init
    {
        [AssemblyInitialize]
        public static void SetupAll()
        {
            // Must NOT run as admin
            Verify.IsFalse(IsAdmin(), "You must run this test as a standard user. WinAppDriver generally cannot find host windows launched as admin.");

            // Check if WinAppDriver is running.
            Verify.IsTrue(IsWinAppDriverRunning(), "WinAppDriver server must be running on the local machine to run this test.");
        }

        public static bool IsAdmin()
        {
            var identity = WindowsIdentity.GetCurrent();
            var principal = new WindowsPrincipal(identity);
            return principal.IsInRole(WindowsBuiltInRole.Administrator);
        }

        public static bool IsWinAppDriverRunning()
        {
            return Process.GetProcessesByName("WinAppDriver").Count() > 0;
        }
    }
}
