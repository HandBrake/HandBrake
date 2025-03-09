// --------------------------------------------------------------------------------------------------------------------
// <copyright file="RawCoverArtData.cs" company="HandBrake Project (https://handbrake.fr)">
// This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.Interfaces.Model.Preview
{
    using HandBrake.Interop.Interop.Json.Shared;

    public class RawCoverArtData
    {
        public RawCoverArtData(byte[] rawBitmapData)
        {
            this.RawBitmapData = rawBitmapData;
        }

        public byte[] RawBitmapData { get; set; }

        public CoverArtType CoverArtFileType { get; set; }


    }
}
