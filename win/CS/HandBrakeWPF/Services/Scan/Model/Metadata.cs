// --------------------------------------------------------------------------------------------------------------------
// <copyright file="MetaData.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   An MetaData Class
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Scan.Model
{
    /// <summary>
    /// The meta data.
    /// </summary>
    public class Metadata
    {
        public Metadata()
        {
        }

        public Metadata(string albumArtist, string album, string artist, string comment, string composer, string description, string genre, string longDescription, string name, string releaseDate)
        {
            this.AlbumArtist = albumArtist;
            this.Album = album;
            this.Artist = artist;
            this.Comment = comment;
            this.Composer = composer;
            this.Description = description;
            this.Genre = genre;
            this.LongDescription = longDescription;
            this.Name = name;
            this.ReleaseDate = releaseDate;
        }

        /// <summary>
        /// Gets the album artist.
        /// </summary>
        public string AlbumArtist { get; }


        /// <summary>
        /// Gets the album.
        /// </summary>
        public string Album { get; }

        /// <summary>
        /// Gets the artist.
        /// </summary>
        public string Artist { get; }

        /// <summary>
        /// Gets the comment.
        /// </summary>
        public string Comment { get; }

        /// <summary>
        /// Gets the composer.
        /// </summary>
        public string Composer { get; }

        /// <summary>
        /// Gets the description.
        /// </summary>
        public string Description { get; }

        /// <summary>
        /// Gets the genre.
        /// </summary>
        public string Genre { get; }

        /// <summary>
        /// Gets the long description.
        /// </summary>
        public string LongDescription { get; }

        /// <summary>
        /// Gets the name.
        /// </summary>
        public string Name { get; }

        /// <summary>
        /// Gets the release date.
        /// </summary>
        public string ReleaseDate { get; }
    }
}
