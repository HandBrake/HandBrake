// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SubtitleTrack.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Subtitle Information
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Encode.Model.Models
{
    using System;

    using HandBrakeWPF.Services.Scan.Model;
    using HandBrakeWPF.Utilities;

    /// <summary>
    /// Subtitle Information
    /// </summary>
    public class SubtitleTrack : PropertyChangedBase
    {
        #region Constants and Fields

        /// <summary>
        /// The burned in backing field.
        /// </summary>
        private bool burned;

        /// <summary>
        /// The is default backing field.
        /// </summary>
        private bool isDefault;

        /// <summary>
        /// The source track.
        /// </summary>
        private Subtitle sourceTrack;

        /// <summary>
        /// Backing field for the srt file name.
        /// </summary>
        private string srtFileName;

        /// <summary>
        /// Backing field for Forced Subs
        /// </summary>
        private bool forced;

        #endregion

        #region Constructors and Destructors

        /// <summary>
        /// Initializes a new instance of the <see cref="SubtitleTrack"/> class.
        /// </summary>
        public SubtitleTrack()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="SubtitleTrack"/> class.
        /// Copy Constructor
        /// </summary>
        /// <param name="subtitle">
        /// The subtitle.
        /// </param>
        public SubtitleTrack(SubtitleTrack subtitle)
        {
            this.Burned = subtitle.Burned;
            this.Default = subtitle.Default;
            this.Forced = subtitle.Forced;
            this.sourceTrack = subtitle.SourceTrack;
            this.SrtCharCode = subtitle.SrtCharCode;
            this.SrtFileName = subtitle.SrtFileName;
            this.SrtLang = subtitle.SrtLang;
            this.SrtOffset = subtitle.SrtOffset;
            this.SrtPath = subtitle.SrtPath;
            this.SubtitleType = subtitle.SubtitleType;
            this.SourceTrack = subtitle.SourceTrack;
        }

        #endregion

        #region Properties

        /// <summary>
        ///   Gets or sets a value indicating whether Burned.
        /// </summary>
        public bool Burned
        {
            get
            {
                return this.burned;
            }

            set
            {
                if (!Equals(this.burned, value))
                {
                    this.burned = value;
                    this.NotifyOfPropertyChange(() => this.Burned);

                    if (value)
                    {
                        this.Default = false;
                    }
                }
            }
        }

        /// <summary>
        ///   Gets or sets a value indicating whether Default.
        /// </summary>
        public bool Default
        {
            get
            {
                return this.isDefault;
            }

            set
            {
                if (!Equals(this.isDefault, value))
                {
                    this.isDefault = value;
                    this.NotifyOfPropertyChange(() => this.Default);

                    if (value)
                    {
                        this.Burned = false;
                    }
                }
            }
        }

        /// <summary>
        ///   Gets or sets a value indicating whether Forced.
        /// </summary>
        public bool Forced
        {
            get
            {
                return this.forced;
            }
            set
            {
                this.forced = value;
                this.NotifyOfPropertyChange(() => this.Forced);
            }
        }

        /// <summary>
        ///   Gets or sets SourceTrack.
        /// </summary>
        public Subtitle SourceTrack
        {
            get
            {
                return this.sourceTrack;
            }

            set
            {
                this.sourceTrack = value;
                this.NotifyOfPropertyChange(() => this.SourceTrack);
                if (this.sourceTrack != null)
                {
                    this.Track = this.sourceTrack.ToString();
                }

                this.NotifyOfPropertyChange(() => this.CanBeBurned);
                this.NotifyOfPropertyChange(() => this.CanBeForced);
            }
        }

        /// <summary>
        ///   Gets or sets the SRT Character Code
        /// </summary>
        public string SrtCharCode { get; set; }

        /// <summary>
        ///   Gets or sets the SRT Filename
        /// </summary>
        public string SrtFileName
        {
            get
            {
                return this.srtFileName;
            }

            set
            {
                this.srtFileName = value;
                this.NotifyOfPropertyChange(() => this.IsSrtSubtitle);
            }
        }

        /// <summary>
        ///   Gets or sets the SRT Language
        /// </summary>
        public string SrtLang { get; set; }

        /// <summary>
        ///   Gets or sets the SRT Offset
        /// </summary>
        public int SrtOffset { get; set; }

        /// <summary>
        ///   Gets or sets the Path to the SRT file
        /// </summary>
        public string SrtPath { get; set; }

        /// <summary>
        ///   Gets or sets the type of the subtitle
        /// </summary>
        public SubtitleType SubtitleType { get; set; }

        /// <summary>
        ///   Gets or sets Track.
        /// </summary>
        [Obsolete("Use SourceTrack Instead")]
        public string Track { get; set; }

        #endregion

        /// <summary>
        /// Gets a value indicating whether CanForced.
        /// </summary>
        public bool CanBeForced
        {
            get
            {
                if (this.SourceTrack != null)
                {
                    return this.SourceTrack.CanForce || this.SourceTrack.SubtitleType == SubtitleType.ForeignAudioSearch;
                }

                return false;
            }
        }

        /// <summary>
        /// Gets a value indicating whether CanBeBurned.
        /// </summary>
        public bool CanBeBurned
        {
            get
            {    
                if (this.SourceTrack != null)
                {
                    return this.SourceTrack.CanBurnIn || this.SourceTrack.SubtitleType == SubtitleType.ForeignAudioSearch || this.SubtitleType == SubtitleType.SRT;
                }

                if (this.SubtitleType == SubtitleType.SRT)
                {
                    return true;
                }

                return false;
            }
        }

        /// <summary>
        ///   Gets a value indicating whether this is an SRT subtitle.
        /// </summary>
        public bool IsSrtSubtitle
        {
            get
            {
                return this.SrtFileName != "-" && this.SrtFileName != null;
            }
        }
    }
}