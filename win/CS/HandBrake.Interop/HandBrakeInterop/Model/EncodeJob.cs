// --------------------------------------------------------------------------------------------------------------------
// <copyright file="EncodeJob.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the EncodeJob type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Model
{
    using System;
    using System.Collections.Generic;
    using System.Xml.Serialization;

    using HandBrake.Interop.Model.Encoding;

    /// <summary>
    /// The encode job.
    /// </summary>
    public class EncodeJob
    {
        #region Properties

        /// <summary>
        ///     Gets or sets the angle to encode. 0 for default, 1+ for specified angle.
        /// </summary>
        public int Angle { get; set; }

        /// <summary>
        /// Gets or sets the chapter end.
        /// </summary>
        public int ChapterEnd { get; set; }

        /// <summary>
        /// Gets or sets the chapter start.
        /// </summary>
        public int ChapterStart { get; set; }

        /// <summary>
        ///     Gets or sets the list of chosen audio tracks (1-based)
        /// </summary>
        public List<int> ChosenAudioTracks { get; set; }

        /// <summary>
        /// Gets or sets the custom chapter names.
        /// </summary>
        public List<string> CustomChapterNames { get; set; }

        /// <summary>
        /// Gets or sets the encoding profile.
        /// </summary>
        public EncodingProfile EncodingProfile { get; set; }

        /// <summary>
        /// Gets or sets the frames end.
        /// </summary>
        public int FramesEnd { get; set; }

        /// <summary>
        /// Gets or sets the frames start.
        /// </summary>
        public int FramesStart { get; set; }

        /// <summary>
        /// Gets or sets the length. The length of video to encode.
        /// </summary>
        [XmlIgnore]
        public TimeSpan Length { get; set; }

        /// <summary>
        /// Gets or sets the output path.
        /// </summary>
        public string OutputPath { get; set; }

        /// <summary>
        /// Gets or sets the range type.
        /// </summary>
        public VideoRangeType RangeType { get; set; }

        /// <summary>
        /// Gets or sets the seconds end.
        /// </summary>
        public double SecondsEnd { get; set; }

        /// <summary>
        /// Gets or sets the seconds start.
        /// </summary>
        public double SecondsStart { get; set; }

        /// <summary>
        /// Gets or sets the source path.
        /// </summary>
        public string SourcePath { get; set; }

        /// <summary>
        /// Gets or sets the source type.
        /// </summary>
        public SourceType SourceType { get; set; }

        /// <summary>
        /// Gets or sets the subtitles.
        /// </summary>
        public Subtitles Subtitles { get; set; }

        /// <summary>
        ///     Gets or sets the 1-based index of the title to encode.
        /// </summary>
        public int Title { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether use default chapter names.
        /// </summary>
        public bool UseDefaultChapterNames { get; set; }

        /// <summary>
        /// Gets or sets the xml length.
        /// </summary>
        [XmlElement("Length")]
        public string XmlLength
        {
            get
            {
                return this.Length.ToString();
            }
            set
            {
                this.Length = TimeSpan.Parse(value);
            }
        }

        #endregion

        #region Public Methods

        /// <summary>
        /// The clone.
        /// </summary>
        /// <returns>
        /// The <see cref="EncodeJob"/>.
        /// </returns>
        public EncodeJob Clone()
        {
            var clone = new EncodeJob
                            {
                                SourceType = this.SourceType, 
                                SourcePath = this.SourcePath, 
                                Title = this.Title, 
                                Angle = this.Angle, 
                                RangeType = this.RangeType, 
                                ChapterStart = this.ChapterStart, 
                                ChapterEnd = this.ChapterEnd, 
                                SecondsStart = this.SecondsStart, 
                                SecondsEnd = this.SecondsEnd, 
                                FramesStart = this.FramesStart, 
                                FramesEnd = this.FramesEnd, 
                                ChosenAudioTracks = new List<int>(this.ChosenAudioTracks), 
                                Subtitles = this.Subtitles, 
                                UseDefaultChapterNames = this.UseDefaultChapterNames, 
                                OutputPath = this.OutputPath, 
                                EncodingProfile = this.EncodingProfile, 
                                Length = this.Length
                            };

            return clone;
        }

        #endregion
    }
}