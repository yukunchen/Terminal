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
        static Process appDriver;

        [AssemblyInitialize]
        public static void SetupAll(TestContext context)
        {
            string winAppDriver = Environment.GetEnvironmentVariable("WinAppDriverExe");
            Verify.IsNotNull(winAppDriver, "You must set the environment var 'WinAppDriverExe' with the path to WinAppDriver.exe to run this test.");
            appDriver = Process.Start(winAppDriver);
        }

        [AssemblyCleanup]
        public static void CleanupAll()
        {
            appDriver.Kill();
        }
    }
}
