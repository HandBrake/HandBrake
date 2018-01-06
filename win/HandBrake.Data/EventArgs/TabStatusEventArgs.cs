// --------------------------------------------------------------------------------------------------------------------
// <copyright file="TabStatusEventArgs.cs" company="HandBrake Project (http://handbrake.fr)">
// This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.EventArgs
{
    using System;

    public class TabStatusEventArgs : EventArgs
    {
        public TabStatusEventArgs(string tabKey)
        {
            this.TabKey = tabKey;
        }

        public string TabKey { get; private set; }
    }
}
