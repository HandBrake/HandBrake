// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Subtitles.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the Subtitles type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Model
{
    using System.Collections.Generic;

    public class Subtitles
    {
        public List<SrtSubtitle> SrtSubtitles { get; set; }
        public List<SourceSubtitle> SourceSubtitles { get; set; }
    }
}
