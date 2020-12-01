// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SubtitleAttributes.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The color.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.Json.Scan
{
    using System.Text.Json.Serialization;

    public class SubtitleAttributes
    {
        [JsonPropertyName("4By3")]
        public bool FourByThree { get; set; }

        public bool Children { get; set; }

        public bool ClosedCaption { get; set; }

        public bool Commentary { get; set; }

        public bool Default { get; set; }

        public bool Forced { get; set; }

        public bool Large { get; set; }

        public bool Letterbox { get; set; }

        public bool Normal { get; set; }

        public bool PanScan { get; set; }

        public bool Wide { get; set; }
    }
}
