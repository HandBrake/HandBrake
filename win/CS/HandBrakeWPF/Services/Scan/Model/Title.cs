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
    using System.Linq;

    using HandBrake.ApplicationServices.Interop.Model;

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
        /// Gets or sets the aspect ratio of this Title
        /// </summary>
        public decimal AspectRatio { get; set; }

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
        public string SourceName { get; set; }

        #endregion

        /// <summary>
        /// Calcuate the Duration
        /// </summary>
        /// <param name="startPoint">The Start Point (Chapters)</param>
        /// <param name="endPoint">The End Point (Chapters)</param>
        /// <returns>A Timespan</returns>
        public TimeSpan CalculateDuration(int startPoint, int endPoint)
        {
            IEnumerable<Chapter> chapers =
                this.Chapters.Where(c => c.ChapterNumber >= startPoint && c.ChapterNumber <= endPoint);

            TimeSpan duration = TimeSpan.FromSeconds(0.0);
            duration = chapers.Aggregate(duration, (current, chapter) => current + chapter.Duration);

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

            return string.Format("{0}{1} ({2:00}:{3:00}:{4:00})", this.TitleNumber, this.Playlist, this.Duration.Hours, this.Duration.Minutes, this.Duration.Seconds);
        }
    }
}