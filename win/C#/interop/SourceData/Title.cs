// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Title.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   An object that represents a single Title of a DVD
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.SourceData
{
    using System;
    using System.Collections.Generic;
    using Model;

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
            this.AudioTracks = new List<AudioTrack>();
            this.Chapters = new List<Chapter>();
            this.Subtitles = new List<Subtitle>();
        }

        /// <summary>
        /// Gets Collection of chapters in this Title
        /// </summary>
        public List<Chapter> Chapters { get; private set; }

        /// <summary>
        /// Gets Collection of audio tracks associated with this Title
        /// </summary>
        public List<AudioTrack> AudioTracks { get; private set; }

        /// <summary>
        /// Gets Collection of subtitles associated with this Title
        /// </summary>
        public List<Subtitle> Subtitles { get; private set; }

        /// <summary>
        /// Gets or sets The track number of this Title (1-based).
        /// </summary>
        public int TitleNumber { get; set; }

        /// <summary>
        /// Gets or sets The length in time of this Title
        /// </summary>
        public TimeSpan Duration { get; set; }

        /// <summary>
        /// Gets or sets The resolution (width/height) of this Title
        /// </summary>
        public Size Resolution { get; set; }

        /// <summary>
        /// Gets or sets The aspect ratio of this Title
        /// </summary>
        public double AspectRatio { get; set; }

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
        /// 0: 
        /// 1: 
        /// 2: 
        /// 3: 
        /// </summary>
        public Cropping AutoCropDimensions { get; set; }

        /// <summary>
        /// Gets Display.
        /// </summary>
        public string Display
        {
            get
            {
                return this.ToString();
            }
        }
  
        /// <summary>
        /// Override of the ToString method to provide an easy way to use this object in the UI
        /// </summary>
        /// <returns>A string representing this track in the format: {title #} (00:00:00)</returns>
        public override string ToString()
        {
            return string.Format("{0} ({1:00}:{2:00}:{3:00})", this.TitleNumber, this.Duration.Hours,
                                 this.Duration.Minutes, this.Duration.Seconds);
        }
    }
}