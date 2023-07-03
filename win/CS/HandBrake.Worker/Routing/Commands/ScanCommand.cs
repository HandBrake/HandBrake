// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ScanCommand.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the EncodeCommand type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Worker.Routing.Commands
{
    using System;
    using System.Collections.Generic;

    public class ScanCommand
    {
        public InitCommand InitialiseCommand { get; set; }

        public List<string> Paths { get; set; }

        public int PreviewCount { get; set; }

        public TimeSpan MinDuration { get; set; }

        public int TitleIndex { get; set; }

        public List<string> FileExclusionList { get; set; }
    }
}