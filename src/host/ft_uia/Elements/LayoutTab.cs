//----------------------------------------------------------------------------------------------------------------------
// <copyright file="LayoutTab.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>Wrapper and helper for instantiating and interacting with the Layout tab of the properties dialog.</summary>
//----------------------------------------------------------------------------------------------------------------------
namespace Conhost.UIA.Tests.Elements
{
    using System;
    using System.Collections.Generic;
    using System.Linq;

    using MS.Internal.Mita.Foundation;
    using MS.Internal.Mita.Foundation.Controls;
    using MS.Internal.Mita.Foundation.Waiters;

    using Conhost.UIA.Tests.Common;
    using NativeMethods = Conhost.UIA.Tests.Common.NativeMethods;

    public class LayoutTab : TabBase
    {
        public List<CheckBoxMeta> Checkboxes { get; private set; }

        public LayoutTab(PropertiesDialog propDialog) : base(propDialog, " Layout ")
        {
        }

        protected override void PopulateItemsOnNavigate(Window propWindow)
        {
            this.Checkboxes = new List<CheckBoxMeta>();
            this.Checkboxes.Add(new CheckBoxMeta(propWindow, "Wrap text output on resize", "LineWrap", false, false, true, NativeMethods.WinConP.PKEY_Console_WrapText));
        }

        public override IEnumerable<UIObject> GetObjectsDisabledForV1Console()
        {
            return this.Checkboxes.Select(meta => meta.Box);
        }

        public override IEnumerable<UIObject> GetObjectsUnaffectedByV1V2Switch()
        {
            return new UIObject[0];
        }

        public override IEnumerable<CheckBoxMeta> GetCheckboxesForVerification()
        {
            return this.Checkboxes;
        }

        public override IEnumerable<SliderMeta> GetSlidersForVerification()
        {
            return new SliderMeta[0];
        }
    }
}