// --------------------------------------------------------------------------------------------------------------------
// <copyright file="TabStatusEventArgs.cs" company="HandBrake Project (http://handbrake.fr)">
// This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.EventArgs
{
    using System;

    using HandBrakeWPF.Model;

    public class TabStatusEventArgs : EventArgs
    {
        public TabStatusEventArgs(string tabKey, ChangedOption changedOption = ChangedOption.None)
        {
            this.TabKey = tabKey;
            this.ChangedOption = changedOption;
        }

        public string TabKey { get; }

        public ChangedOption ChangedOption { get; }
    }
}
