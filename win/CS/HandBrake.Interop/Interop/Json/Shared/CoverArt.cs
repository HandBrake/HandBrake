// --------------------------------------------------------------------------------------------------------------------
// <copyright file="CoverArt.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.Json.Shared
{
    public class CoverArt
    {
        public int ID { get; set; }

        public string Name { get; set; }

        public CoverArtType Type { get; set; }
    }
}
