// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Metadata.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The meta data.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.Json.Encode
{
    /// <summary>
    ///     The meta data.
    /// </summary>
    public class Metadata
    {
        /// <summary>
        ///     Gets or sets the album artist.
        /// </summary>
        public string AlbumArtist { get; set; }

        /// <summary>
        ///     Gets or sets the album.
        /// </summary>
        public string Album { get; set; }

        /// <summary>
        ///     Gets or sets the artist.
        /// </summary>
        public string Artist { get; set; }

        /// <summary>
        ///     Gets or sets the comment.
        /// </summary>
        public string Comment { get; set; }

        /// <summary>
        ///     Gets or sets the composer.
        /// </summary>
        public string Composer { get; set; }

        /// <summary>
        ///     Gets or sets the description.
        /// </summary>
        public string Description { get; set; }

        /// <summary>
        ///     Gets or sets the genre.
        /// </summary>
        public string Genre { get; set; }

        /// <summary>
        ///     Gets or sets the long description.
        /// </summary>
        public string LongDescription { get; set; }

        /// <summary>
        ///     Gets or sets the name.
        /// </summary>
        public string Name { get; set; }

        /// <summary>
        ///     Gets or sets the release date.
        /// </summary>
        public string ReleaseDate { get; set; }
    }
}