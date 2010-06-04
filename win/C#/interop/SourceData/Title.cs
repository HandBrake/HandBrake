/*  Title.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Text.RegularExpressions;
using HandBrake.Interop;

namespace HandBrake.SourceData
{
    /// <summary>
    /// An object that represents a single Title of a DVD
    /// </summary>
    public class Title
    {
        private static readonly CultureInfo Culture = new CultureInfo("en-US", false);
        private readonly List<AudioTrack> audioTracks;
        private readonly List<Chapter> chapters;
        private readonly List<Subtitle> subtitles;
        
        /// <summary>
        /// The constructor for this object
        /// </summary>
        public Title()
        {
            this.audioTracks = new List<AudioTrack>();
            this.chapters = new List<Chapter>();
            this.subtitles = new List<Subtitle>();
        }

        /// <summary>
        /// Collection of chapters in this Title
        /// </summary>
        public List<Chapter> Chapters
        {
            get { return this.chapters; }
        }

        /// <summary>
        /// Collection of audio tracks associated with this Title
        /// </summary>
        public List<AudioTrack> AudioTracks
        {
            get { return this.audioTracks; }
        }

        /// <summary>
        /// Collection of subtitles associated with this Title
        /// </summary>
        public List<Subtitle> Subtitles
        {
            get { return this.subtitles; }
        }

        /// <summary>
        /// The track number of this Title (1-based).
        /// </summary>
        public int TitleNumber { get; set; }

        /// <summary>
        /// The length in time of this Title
        /// </summary>
        public TimeSpan Duration { get; set; }

        /// <summary>
        /// The resolution (width/height) of this Title
        /// </summary>
        public Size Resolution { get; set; }

        /// <summary>
        /// The aspect ratio of this Title
        /// </summary>
        public double AspectRatio { get; set; }

        public int AngleCount { get; set; }

        /// <summary>
        /// Par Value
        /// </summary>
        public Size ParVal { get; set; }

        /// <summary>
        /// The automatically detected crop region for this Title.
        /// This is an int array with 4 items in it as so:
        /// 0: 
        /// 1: 
        /// 2: 
        /// 3: 
        /// </summary>
        public Cropping AutoCropDimensions { get; set; }
  
        /// <summary>
        /// Override of the ToString method to provide an easy way to use this object in the UI
        /// </summary>
        /// <returns>A string representing this track in the format: {title #} (00:00:00)</returns>
        public override string ToString()
        {
            return string.Format("{0} ({1:00}:{2:00}:{3:00})", this.TitleNumber, this.Duration.Hours,
                                 this.Duration.Minutes, this.Duration.Seconds);
        }

        public string Display
        {
            get
            {
                return this.ToString();
            }
        }
    }
}