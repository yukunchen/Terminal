//----------------------------------------------------------------------------------------------------------------------
// <copyright file="PropertiesDialog.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>Wrapper and helper for instantiating and interacting with the properties dialog.</summary>
//----------------------------------------------------------------------------------------------------------------------
namespace Conhost.UIA.Tests.Elements
{
    using System;

    using MS.Internal.Mita.Foundation;
    using MS.Internal.Mita.Foundation.Controls;
    using MS.Internal.Mita.Foundation.Waiters;

    using Conhost.UIA.Tests.Common;
    public class PropertiesDialog : IDisposable
    {
        public enum CloseAction
        {
            OK,
            Cancel
        }

        public Window PropWindow { get; private set; }
        public Tab Tabs { get; private set; }

        private Button okButton;
        private Button cancelButton;

        private CmdApp app;

        private bool isOpened;

        public PropertiesDialog(CmdApp app)
        {
            this.app = app;
        }

        ~PropertiesDialog()
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
            if (disposing)
            {
                // if we're being disposed of, close the dialog.
                if (this.isOpened)
                {
                    this.ClosePropertiesDialog(CloseAction.Cancel);
                }
            }
        }

        public void Open(OpenTarget target)
        {
            if (this.isOpened)
            {
                throw new InvalidOperationException("Can't open an already opened window.");
            }

            this.OpenPropertiesDialog(this.app, target);
            this.isOpened = true;
        }

        public void Close(CloseAction action)
        {
            if (!this.isOpened)
            {
                throw new InvalidOperationException("Can't close an unopened window.");
            }

            this.ClosePropertiesDialog(action);
            this.isOpened = false;
        }

        private void OpenPropertiesDialog(CmdApp app, OpenTarget target)
        {
            MenuOpenedWaiter contextMenuWaiter = new MenuOpenedWaiter();

            UIObject titleBar = app.GetTitleBar();
            titleBar.Click(PointerButtons.Secondary);

            contextMenuWaiter.Wait(Globals.Timeout);
            UIObject contextMenu = contextMenuWaiter.Source;

            UIObject propButton = null;
            switch (target)
            {
                case OpenTarget.Specifics:
                    propButton = contextMenu.Children.Find(UICondition.CreateFromName("Properties"));
                    break;
                case OpenTarget.Defaults:
                    propButton = contextMenu.Children.Find(UICondition.CreateFromName("Defaults"));
                    break;
                default:
                    throw new NotImplementedException(AutoHelpers.FormatInvariant("Open Properties dialog doesn't yet support target type of '{0}'", target.ToString()));
            }

            WindowOpenedWaiter propWindowWaiter = new WindowOpenedWaiter();
            propButton.Click(PointerButtons.Primary);

            propWindowWaiter.Wait(Globals.Timeout);

            this.PropWindow = new Window(propWindowWaiter.Source);
            this.Tabs = new Tab(this.PropWindow.Children.Find(UICondition.CreateFromClassName("SysTabControl32")));

            okButton = new Button(this.PropWindow.Children.Find(UICondition.CreateFromName("OK")));
            cancelButton = new Button(this.PropWindow.Children.Find(UICondition.CreateFromName("Cancel")));
        }

        private void ClosePropertiesDialog(CloseAction action)
        {
            WindowClosedWaiter waiter = new WindowClosedWaiter(this.PropWindow);

            switch (action)
            {
                case CloseAction.OK:
                    okButton.Click();
                    break;
                case CloseAction.Cancel:
                    cancelButton.Click();
                    break;
            }

            waiter.Wait(Globals.Timeout);
        }
    }
}
