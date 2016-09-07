//----------------------------------------------------------------------------------------------------------------------
// <copyright file="FontTab.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>Wrapper and helper for instantiating and interacting with the Font tab of the properties dialog.</summary>
//----------------------------------------------------------------------------------------------------------------------
namespace Conhost.UIA.Tests.Elements
{
    using System;
    using System.Collections.Generic;

    using MS.Internal.Mita.Foundation;
    using MS.Internal.Mita.Foundation.Controls;

    using Conhost.UIA.Tests.Common;
    using NativeMethods = Conhost.UIA.Tests.Common.NativeMethods;

    public class FontTab : TabBase
    {
        public FontTab(PropertiesDialog propDialog) : base(propDialog, " Font ")
        {

        }

        protected override void PopulateItemsOnNavigate(Window propWindow)
        {
            return;
        }

        public override IEnumerable<UIObject> GetObjectsDisabledForV1Console()
        {
            return new UIObject[0];
        }

        public override IEnumerable<UIObject> GetObjectsUnaffectedByV1V2Switch()
        {
            return new UIObject[0];
        }

        public override IEnumerable<CheckBoxMeta> GetCheckboxesForVerification()
        {
            return new CheckBoxMeta[0];
        }

        public override IEnumerable<SliderMeta> GetSlidersForVerification()
        {
            return new SliderMeta[0];
        }
    }
}