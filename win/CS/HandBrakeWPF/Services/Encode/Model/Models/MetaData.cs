// --------------------------------------------------------------------------------------------------------------------
// <copyright file="MetaData.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   An MetaData Class
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Encode.Model.Models
{
    using HandBrakeWPF.Services.Scan.Model;

    public class MetaData
    {
        public MetaData()
        {
        }

        public MetaData(MetaData metadata)
        {
            if (metadata != null)
            {
                this.AlbumArtist = metadata.AlbumArtist;
                this.Album = metadata.Album;
                this.Artist = metadata.Artist;
                this.Comment = metadata.Comment;
                this.Composer = metadata.Composer;
                this.Description = metadata.Description;
                this.Genre = metadata.Genre;
                this.LongDescription = metadata.LongDescription;
                this.Name = metadata.Name;
                this.ReleaseDate = metadata.ReleaseDate;
                this.PassthruMetadataEnabled = metadata.PassthruMetadataEnabled;
            }
        }

        public MetaData(Metadata metadata, bool metadataEnabled)
        {
            this.PassthruMetadataEnabled = metadataEnabled;

            if (metadata != null)
            {
                this.AlbumArtist = metadata.AlbumArtist;
                this.Album = metadata.Album;
                this.Artist = metadata.Artist;
                this.Comment = metadata.Comment;
                this.Composer = metadata.Composer;
                this.Description = metadata.Description;
                this.Genre = metadata.Genre;
                this.LongDescription = metadata.LongDescription;
                this.Name = metadata.Name;
                this.ReleaseDate = metadata.ReleaseDate;
            }
        }

        public bool PassthruMetadataEnabled { get; set; }

        public string AlbumArtist { get; set; }

        public string Album { get; set; }

        public string Artist { get; set; }

        public string Comment { get; set; }

        public string Composer { get; set; }

        public string Description { get; set; }

        public string Genre { get; set; }

        public string LongDescription { get; set; }

        public string Name { get; set; }

        public string ReleaseDate { get; set; }

        public bool IsMetadataSet
        {
            get
            {
                return !string.IsNullOrEmpty(AlbumArtist) || !string.IsNullOrEmpty(Album)
                                                          || !string.IsNullOrEmpty(Artist)
                                                          || !string.IsNullOrEmpty(Comment)
                                                          || !string.IsNullOrEmpty(Composer)
                                                          || !string.IsNullOrEmpty(Description)
                                                          || !string.IsNullOrEmpty(Genre)
                                                          || !string.IsNullOrEmpty(LongDescription)
                                                          || !string.IsNullOrEmpty(Name)
                                                          || !string.IsNullOrEmpty(ReleaseDate);
            }
        }
    }
}
