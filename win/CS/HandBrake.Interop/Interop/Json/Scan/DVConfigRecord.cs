// --------------------------------------------------------------------------------------------------------------------
// <copyright file="DVConfigRecord.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.Json.Scan
{
    public class DVConfigRecord
    {
        public int BLPresentFlag { get; set; }

        public int BLSignalCompatibilityId { get; set; }

        public int DVLevel { get; set; }

        public int? DVProfile { get; set; }

        public int DVVersionMajor { get; set; }

        public int DVVersionMinor { get; set; }

        public int ELPresentFlag { get; set; }

        public int RPUPresentFlag { get; set; }
    }
}
