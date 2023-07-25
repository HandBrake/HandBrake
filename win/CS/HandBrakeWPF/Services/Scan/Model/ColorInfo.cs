// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ColorInfo.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>

namespace HandBrakeWPF.Services.Scan.Model
{
    public class ColorInfo
    {
        public bool HDR10plus { get; set; }

        public bool HDR10 { get; set; }

        public bool HDR { get; set; }

        public bool DBV { get; set; }

        public string DBVProfileStr { get; set; }

        public int? BitDepth { get; set; }

        public string ChromaSubsampling { get; set; }

        public string ColourInfoStr { get; set; }
    }
}
