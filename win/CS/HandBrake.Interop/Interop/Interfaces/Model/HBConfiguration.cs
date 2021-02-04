// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HBConfiguration.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   HandBrakes Configuration options
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.Interfaces.Model
{
    public class HBConfiguration
    {
        public HBConfiguration()
        {
        }

        public int PreviewScanCount { get; set; }

        public bool EnableQuickSyncDecoding { get; set; }

        public bool UseQSVDecodeForNonQSVEnc { get; set; }

        public bool EnableQsvLowPower { get; set; }
    }
}
