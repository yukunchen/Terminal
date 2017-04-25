//----------------------------------------------------------------------------------------------------------------------
// <copyright file="AccessibilityTests.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>UI Automation tests for the certain key presses.</summary>
//----------------------------------------------------------------------------------------------------------------------
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Drawing;
using System.Windows.Automation;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

using Conhost.UIA.Tests.Common;
using Conhost.UIA.Tests.Common.NativeMethods;
using Conhost.UIA.Tests.Elements;
using OpenQA.Selenium;


namespace Conhost.UIA.Tests
{
    [TestClass]
    class AccessibilityTests
    {

        public TestContext TestContext { get; set; }

        [TestMethod]
        public void CanAccessAccessibilityTree()
        {
            using (RegistryHelper reg = new RegistryHelper())
            {
                reg.BackupRegistry();
                using (CmdApp app = new CmdApp(CreateType.ProcessOnly, TestContext))
                {
                    IntPtr handle = app.GetWindowHandle();
                    AutomationElement windowUiaElement = AutomationElement.FromHandle(handle);
                    Verify.IsTrue(windowUiaElement.Current.AutomationId.Equals("Console Window"));
                }
            }
        }
    }
}
