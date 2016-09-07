//----------------------------------------------------------------------------------------------------------------------
// <copyright file="TabBase.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>Wrapper and helper for instantiating various tabs of the properties dialog.</summary>
//----------------------------------------------------------------------------------------------------------------------
namespace Conhost.UIA.Tests.Elements
{
    using System;
    using System.Collections.Generic;

    using MS.Internal.Mita.Foundation;
    using MS.Internal.Mita.Foundation.Controls;
    using MS.Internal.Mita.Foundation.Waiters;

    using Conhost.UIA.Tests.Common;

    public abstract class TabBase : IDisposable
    {
        protected string tabName;

        protected PropertiesDialog propDialog;

        private TabBase()
        {

        }

        public TabBase(PropertiesDialog propDialog, string tabName)
        {
            this.propDialog = propDialog;
            this.tabName = tabName;
        }

        ~TabBase()
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

        public void NavigateToTab()
        {
            TimeWaiter wait = new TimeWaiter();

            AutoHelpers.LogInvariant("Navigating to '{0}'", this.tabName);
            UIObject tab = this.propDialog.Tabs.GetChildByName(this.tabName);

            tab.Click(PointerButtons.Primary);
            wait.Wait(500); // instead detect which tab is visible and click if necessary and wait for elementaddedwaiter

            Window propWindow = this.propDialog.PropWindow;

            this.PopulateItemsOnNavigate(propWindow);

        }

        protected abstract void PopulateItemsOnNavigate(Window propWindow);

        public abstract IEnumerable<UIObject> GetObjectsDisabledForV1Console();
        public abstract IEnumerable<UIObject> GetObjectsUnaffectedByV1V2Switch();

        public abstract IEnumerable<CheckBoxMeta> GetCheckboxesForVerification();

        public abstract IEnumerable<SliderMeta> GetSlidersForVerification();
    }
}