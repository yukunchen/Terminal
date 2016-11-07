using System;
using System.Collections.Generic;
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
            Verify.IsFalse(IsAdmin(), "You must run this test as a standard user. WinAppDriver generally cannot find host windows launched as admin.");
        }

        public static bool IsAdmin()
        {
            var identity = WindowsIdentity.GetCurrent();
            var principal = new WindowsPrincipal(identity);
            return principal.IsInRole(WindowsBuiltInRole.Administrator);
        }
    }
}
