// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AudioAttributes.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The color.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.Json.Scan
{
    public class AudioAttributes
    {
        public bool AltCommentary { get; set; }
        public bool Commentary { get; set; }
        public bool Default { get; set; }
        public bool Normal { get; set; }
        public bool Secondary { get; set; }
        public bool VisuallyImpaired { get; set; }
    }
}
