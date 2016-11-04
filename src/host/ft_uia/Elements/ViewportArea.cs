//----------------------------------------------------------------------------------------------------------------------
// <copyright file="ViewportArea.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>Wrapper and helper for instantiating and interacting with the main text region (viewport area) of the console.</summary>
//----------------------------------------------------------------------------------------------------------------------
namespace Conhost.UIA.Tests.Elements
{
    using System;
    using System.Collections.Generic;
    using System.Drawing;
    using System.Linq;
    using System.Runtime.InteropServices;
    using System.Text;

    using OpenQA.Selenium.Appium;

    using WEX.Logging.Interop;
    using WEX.TestExecution;

    using Conhost.UIA.Tests.Common;
    using Conhost.UIA.Tests.Common.NativeMethods;
    using OpenQA.Selenium;

    public class ViewportArea : IDisposable
    {
        private CmdApp app;
        private Point clientTopLeft;
        private Size sizeFont;

        private ViewportStates state;

        public enum ViewportStates
        {
            Normal,
            Mark, // keyboard selection
            Select, // mouse selection
            Scroll
        }

        public ViewportArea(CmdApp app)
        {
            this.app = app;

            this.state = ViewportStates.Normal;

            this.InitializeFont();
            this.InitializeWindow();
        }

        ~ViewportArea()
        {
            this.Dispose(false);
        }

        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        protected virtual void Dispose(bool disposing)
        {
            // Don't need to dispose of anything now, but this helps maintain the pattern used by other controls.
        }

        private void InitializeFont()
        {
            AutoHelpers.LogInvariant("Initializing font data for viewport area...");
            this.sizeFont = new Size();

            IntPtr hCon = app.GetStdOutHandle();
            Verify.IsNotNull(hCon, "Check that we obtained the console output handle.");

            WinCon.CONSOLE_FONT_INFO cfi = new WinCon.CONSOLE_FONT_INFO();
            NativeMethods.Win32BoolHelper(WinCon.GetCurrentConsoleFont(hCon, false, out cfi), "Attempt to get current console font.");

            this.sizeFont.Width = cfi.dwFontSize.X;
            this.sizeFont.Height = cfi.dwFontSize.Y;
            AutoHelpers.LogInvariant("Font size is X:{0} Y:{1}", this.sizeFont.Width, this.sizeFont.Height);
        }

        private void InitializeWindow()
        {
            AutoHelpers.LogInvariant("Initializing window data for viewport area...");
            //Window w = new Window(this.app.UIRoot);

            //Rectangle windowBounds = new Rectangle(this.app.UIRoot.Location.X, this.app.UIRoot.Location.Y, this.app.UIRoot.Size.Width, this.app.UIRoot.Size.Height);

            IntPtr hWnd = app.GetWindowHandle();

            User32.RECT lpRect;
            User32.GetClientRect(hWnd, out lpRect);

            User32.POINT lpPoint;
            lpPoint.x = lpRect.left;
            lpPoint.y = lpRect.top;

            User32.ClientToScreen(hWnd, ref lpPoint);

            this.clientTopLeft = new Point();
            this.clientTopLeft.X = lpPoint.x;
            this.clientTopLeft.Y = lpPoint.y;
            AutoHelpers.LogInvariant("Top left corner of client area is at X:{0} Y:{1}", this.clientTopLeft.X, this.clientTopLeft.Y);
        }

        public void ExitModes()
        {
            app.UIRoot.SendKeys(Keys.Escape);
            this.state = ViewportStates.Normal;
        }
        
        public void EnterMode(ViewportStates state)
        {
            if (state == ViewportStates.Normal)
            {
                ExitModes();
                return;
            }

            //MenuOpenedWaiter contextMenuWaiter = new MenuOpenedWaiter();
            var titleBar = app.UIRoot.FindElementByAccessibilityId("TitleBar");
            app.Session.Mouse.ContextClick(titleBar.Coordinates);

            Globals.WaitForTimeout();
            //contextMenuWaiter.Wait(Globals.Timeout);
            var contextMenu = app.Session.FindElementByClassName("#32768");
            //var contextMenu = app.Session.FindElementByName("Menu");

            var editButton = contextMenu.FindElementByName("Edit");

            //MenuOpenedWaiter editWaiter = new MenuOpenedWaiter();
            editButton.Click();
            Globals.WaitForTimeout();
            //editWaiter.Wait(Globals.Timeout);

            Globals.WaitForTimeout();

            AppiumWebElement subMenuButton;
            switch (state)
            {
                case ViewportStates.Mark:
                    subMenuButton = app.Session.FindElementByName("Mark");
                    break;
                default:
                    throw new NotImplementedException(AutoHelpers.FormatInvariant("Set Mode doesn't yet support type of '{0}'", state.ToString()));
            }

            //MenuClosedWaiter actionCompleteWaiter = new MenuClosedWaiter();
            subMenuButton.Click();
            Globals.WaitForTimeout();
            //actionCompleteWaiter.Wait(Globals.Timeout);

            this.state = state;
        }
                
        public void ConvertCharacterOffsetToPixelPosition(ref Point pt)
        {
            // Scale by pixel count per character
            pt.X *= this.sizeFont.Width;
            pt.Y *= this.sizeFont.Height;

            // Adjust offset by top left corner of client area
            pt.X += this.clientTopLeft.X;
            pt.Y += this.clientTopLeft.Y;           
        }

        public WinCon.CHAR_INFO GetCharInfoAt(IntPtr handle, Point pt)
        {
            Size size = new Size(1, 1);
            Rectangle rect = new Rectangle(pt, size);

            WinCon.CHAR_INFO[,] data = GetCharInfoInRectangle(handle, rect);

            return data[0, 0];
        }

        public WinCon.CHAR_INFO[,] GetCharInfoInRectangle(IntPtr handle, Rectangle rect)
        {
            WinCon.SMALL_RECT readRectangle = new WinCon.SMALL_RECT();
            readRectangle.Top = (short)rect.Top;
            readRectangle.Bottom = (short)(rect.Bottom - 1);
            readRectangle.Left = (short)rect.Left;
            readRectangle.Right = (short)(rect.Right - 1);

            WinCon.COORD dataBufferSize = new WinCon.COORD();
            dataBufferSize.X = (short)rect.Width;
            dataBufferSize.Y = (short)rect.Height;

            WinCon.COORD dataBufferPos = new WinCon.COORD();
            dataBufferPos.X = 0;
            dataBufferPos.Y = 0;

            WinCon.CHAR_INFO[,] data = new WinCon.CHAR_INFO[dataBufferSize.Y, dataBufferSize.X];

            NativeMethods.Win32BoolHelper(WinCon.ReadConsoleOutput(handle, data, dataBufferSize, dataBufferPos, ref readRectangle), string.Format("Attempting to read rectangle (L: {0}, T: {1}, R: {2}, B: {3}) from output buffer.", readRectangle.Left, readRectangle.Top, readRectangle.Right, readRectangle.Bottom));

            return data;
        }

        public IEnumerable<string> GetLinesInRectangle(IntPtr handle, Rectangle rect)
        {
            WinCon.CHAR_INFO[,] data = GetCharInfoInRectangle(handle, rect);
            List<string> lines = new List<string>();

            for (int row = 0; row < data.GetLength(0); row++)
            {
                StringBuilder builder = new StringBuilder();

                for (int col = 0; col < data.GetLength(1); col++)
                {
                    char z = data[row, col].UnicodeChar;
                    builder.Append(z);
                }

                lines.Add(builder.ToString());
            }

            return lines;
        }
    }
}