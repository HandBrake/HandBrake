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
    using Encoding;

    /// <summary>
    /// An Encode Job
    /// </summary>
    public class EncodeJob
    {
        /// <summary>
        /// Gets or sets SourceType.
        /// </summary>
        public SourceType SourceType { get; set; }

        /// <summary>
        /// Gets or sets SourcePath.
        /// </summary>
        public string SourcePath { get; set; }

        /// <summary>
        /// Gets or sets the 1-based index of the title to encode.
        /// </summary>
        public int Title { get; set; }

        /// <summary>
        /// Gets or sets the angle to encode. 0 for default, 1+ for specified angle.
        /// </summary>
        public int Angle { get; set; }

        /// <summary>
        /// Gets or sets ChapterStart.
        /// </summary>
        public int ChapterStart { get; set; }

        /// <summary>
        /// Gets or sets ChapterEnd.
        /// </summary>
        public int ChapterEnd { get; set; }

        /// <summary>
        /// Gets or sets the list of chosen audio tracks (1-based)
        /// </summary>
        public List<int> ChosenAudioTracks { get; set; }

        /// <summary>
        /// Gets or sets Subtitles.
        /// </summary>
        public Subtitles Subtitles { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether UseDefaultChapterNames.
        /// </summary>
        public bool UseDefaultChapterNames { get; set; }

        /// <summary>
        /// Gets or sets CustomChapterNames.
        /// </summary>
        public List<string> CustomChapterNames { get; set; }

        /// <summary>
        /// Gets or sets OutputPath.
        /// </summary>
        public string OutputPath { get; set; }

        /// <summary>
        /// Gets or sets EncodingProfile.
        /// </summary>
        public EncodingProfile EncodingProfile { get; set; }

        /// <summary>
        /// The length of video to encode.
        /// </summary>
        [XmlIgnore]
        public TimeSpan Length { get; set; }

        /// <summary>
        /// Gets or sets XmlLength.
        /// </summary>
        [XmlElement("Length")]
        public string XmlLength
        {
            get { return this.Length.ToString(); }
            set { this.Length = TimeSpan.Parse(value); }
        }

        /// <summary>
        /// Clone the Encode Job
        /// </summary>
        /// <returns>
        /// An EncodeJob Ojbect
        /// </returns>
        public EncodeJob Clone()
        {
            EncodeJob clone = new EncodeJob
            {
                SourceType = this.SourceType,
                SourcePath = this.SourcePath,
                Title = this.Title,
                ChapterStart = this.ChapterStart,
                ChapterEnd = this.ChapterEnd,
                ChosenAudioTracks = new List<int>(this.ChosenAudioTracks),
                Subtitles = this.Subtitles,
                UseDefaultChapterNames = this.UseDefaultChapterNames,
                OutputPath = this.OutputPath,
                EncodingProfile = this.EncodingProfile,
                Length = this.Length
            };

            return clone;
        }
    }
}
