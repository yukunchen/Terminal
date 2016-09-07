//----------------------------------------------------------------------------------------------------------------------
// <copyright file="CheckBoxMeta.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>CheckBox metadata information helper class.</summary>
//----------------------------------------------------------------------------------------------------------------------
namespace Conhost.UIA.Tests.Common
{
    using System;
    using System.Globalization;

    using MS.Internal.Mita.Foundation.Controls;

    using WEX.Logging.Interop;

    public struct CheckBoxMeta
    {
        public CheckBox Box { get; private set; }
        public string ValueName { get; private set; }
        public bool IsInverse { get; private set; }
        public bool IsGlobalOnly { get; private set; }
        public bool IsV2Property { get; private set; }
        public NativeMethods.Wtypes.PROPERTYKEY? PropKey { get; private set; }

        public CheckBoxMeta(Window window, string englishText, string valueName, bool isInverse, bool isGlobalOnly, bool isV2Property, NativeMethods.Wtypes.PROPERTYKEY? propKey)
            : this()
        {
            this.Box = window.GetCheckboxByName(englishText);
            this.ValueName = valueName;
            this.IsInverse = isInverse;
            this.IsGlobalOnly = isGlobalOnly;
            this.IsV2Property = isV2Property;
            this.PropKey = propKey;
        }
    }
}