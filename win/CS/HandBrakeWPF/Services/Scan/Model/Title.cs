// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Title.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   An object that represents a single Title of a DVD
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Scan.Model
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;

    using HandBrake.Interop.Interop.Interfaces.Model.Picture;

    using Size = HandBrakeWPF.Model.Picture.Size;

    /// <summary>
    /// An object that represents a single Title of a DVD
    /// </summary>
    public class Title
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="Title"/> class. 
        /// </summary>
        public Title()
        {
            this.AudioTracks = new List<Audio>();
            this.Chapters = new List<Chapter>();
            this.Subtitles = new List<Subtitle>();
            this.Metadata = new Metadata();
            this.ColorInformation = new ColorInfo();
        }

        #region Properties
        
        /// <summary>
        /// Gets or sets a Collection of chapters in this Title
        /// </summary>
        public List<Chapter> Chapters { get; set; }

        /// <summary>
        /// Gets or sets a Collection of audio tracks associated with this Title
        /// </summary>
        public List<Audio> AudioTracks { get; set; }

        /// <summary>
        /// Gets or sets a Collection of subtitles associated with this Title
        /// </summary>
        public List<Subtitle> Subtitles { get; set; }

        public Metadata Metadata { get; set; }

        /// <summary>
        /// Gets or sets The track number of this Title
        /// </summary>
        public int TitleNumber { get; set; }

        /// <summary>
        /// Gets or sets the type.
        /// HB_DVD_TYPE, HB_BD_TYPE, HB_STREAM_TYPE, HB_FF_STREAM_TYPE
        /// </summary>
        public int Type { get; set; }

        /// <summary>
        /// Gets or sets Playlist.
        /// </summary>
        public string Playlist { get; set; }

        /// <summary>
        /// Gets or sets the length in time of this Title
        /// </summary>
        public TimeSpan Duration { get; set; }

        /// <summary>
        /// Gets or sets the resolution (width/height) of this Title
        /// </summary>
        public Size Resolution { get; set; }

        /// <summary>
        /// Gets or sets AngleCount.
        /// </summary>
        public int AngleCount { get; set; }

        /// <summary>
        /// Gets or sets Par Value
        /// </summary>
        public Size ParVal { get; set; }

        /// <summary>
        /// Gets or sets the automatically detected crop region for this Title.
        /// This is an int array with 4 items in it as so:
        /// 0: T
        /// 1: B
        /// 2: L
        /// 3: R
        /// </summary>
        public Cropping AutoCropDimensions { get; set; }

        public Cropping LooseCropDimensions { get; set; }

        /// <summary>
        /// Gets or sets the FPS of the source.
        /// </summary>
        public double Fps { get; set; }

        /// <summary>
        /// Gets or sets the video frame rate numerator.
        /// </summary>
        public int FramerateNumerator { get; set; }

        /// <summary>
        /// Gets or sets the video frame rate denominator.
        /// </summary>
        public int FramerateDenominator { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether this is a MainTitle.
        /// </summary>
        public bool MainTitle { get; set; }

        /// <summary>
        /// Gets or sets the Source Name
        /// </summary>
        public string SourcePath { get; set; }

        public string DisplaySourceName
        {
            get
            {
                switch (this.Type)
                {
                    case 0: // HB_DVD_TYPE
                    case 1: // HB_BD_TYPE
                 
                        return DriveLabel;
                    case 2: // HB_STREAM_TYPE
                    case 3: // HB_FF_STREAM_TYPE
                        if (!string.IsNullOrEmpty(this.SourcePath))
                        {
                            return Path.GetFileNameWithoutExtension(this.SourcePath);
                        }
                        break;
                    default:
                        return null;
                }

                return null;
            }
        }

        public string DriveLabel { get; set; }

        public string SourceDisplayName
        {
            get
            {
                switch (this.Type)
                {
                    case 0: // HB_DVD_TYPE
                    case 1: // HB_BD_TYPE
                    default:
                        return string.Empty;
                    case 2: // HB_STREAM_TYPE
                    case 3: // HB_FF_STREAM_TYPE
                        return Path.GetFileNameWithoutExtension(this.SourcePath);
                }
            }
        }

        public string ItemDisplayText
        {
            get
            {
                return string.Format(
                    "{0} {1} ({2:00}:{3:00}:{4:00}) {5}",
                    this.TitleNumber,
                    this.Playlist,
                    this.Duration.Hours,
                    this.Duration.Minutes,
                    this.Duration.Seconds,
                    this.SourceDisplayName);
            }
        }

        public string ItemDisplayTextClosed
        {
            get
            {
                return string.Format(
                    "{0} {1} ({2:00}:{3:00}:{4:00})",
                    this.TitleNumber,
                    this.Playlist,
                    this.Duration.Hours,
                    this.Duration.Minutes,
                    this.Duration.Seconds);
            }
        }

        public ColorInfo ColorInformation { get; set; }

        #endregion

        /// <summary>
        /// Calculate the Duration
        /// </summary>
        /// <param name="startPoint">The Start Point (Chapters)</param>
        /// <param name="endPoint">The End Point (Chapters)</param>
        /// <returns>A Timespan</returns>
        public TimeSpan CalculateDuration(long startPoint, long endPoint)
        {
            IEnumerable<Chapter> chapters =
                this.Chapters.Where(c => c.ChapterNumber >= startPoint && c.ChapterNumber <= endPoint);

            TimeSpan duration = TimeSpan.FromSeconds(0.0);
            duration = chapters.Aggregate(duration, (current, chapter) => current + chapter.Duration);

            return duration;
        }

        /// <summary>
        /// Override of the ToString method to provide an easy way to use this object in the UI
        /// </summary>
        /// <returns>A string representing this track in the format: {title #} (00:00:00)</returns>
        public override string ToString()
        {
            if (!string.IsNullOrEmpty(this.Playlist) && !this.Playlist.StartsWith(" "))
            {
                this.Playlist = string.Format(" {0}", this.Playlist);
            }

            return string.Format("{0} {1} ({2:00}:{3:00}:{4:00})", this.TitleNumber, this.Playlist, this.Duration.Hours, this.Duration.Minutes, this.Duration.Seconds);
        }
    }
}