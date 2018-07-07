// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SourceMetadata.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The meta data.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.Json.Scan
{
    /// <summary>
    /// The meta data.
    /// </summary>
    public class SourceMetadata
    {
        public string Name { get; set; }
        public string Artist { get; set; }
        public string Composer { get; set; }
        public string Comment { get; set; }
        public string Genre { get; set; }
        public string Album { get; set; }
        public string AlbumArtist { get; set; }
        public string Description { get; set; }
        public string LongDescription { get; set; }
        public string ReleaseDate { get; set; }
    }
}