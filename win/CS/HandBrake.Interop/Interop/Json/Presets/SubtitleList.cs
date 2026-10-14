// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SubtitleList.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.Json.Presets
{
    public class SubtitleList
    {
        public string IsoLangCode { get; set; }
        public int TrackSelectionMode { get; set; }
        public int DefaultMode { get; set; }
        public int BurnPassthruMode { get; set; }
        public int ForcedMode { get; set; }
        public string TrackNameOverride { get; set; }
        public bool IsForeignAudioScanRule { get; set; }
    }
}
