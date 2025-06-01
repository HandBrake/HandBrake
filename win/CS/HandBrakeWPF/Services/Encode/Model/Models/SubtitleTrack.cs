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

    using HandBrake.Interop.Interop.Interfaces.Model;

    using HandBrakeWPF.Services.Scan.Model;
    using HandBrakeWPF.ViewModels;

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

        private Language srtLang;

        private string name;

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
            this.Name = subtitle.Name;
            this.TrackNamingCallback = subtitle.TrackNamingCallback;
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
                    this.SubtitleType = this.sourceTrack.SubtitleType;
                }
                
                this.NotifyOfPropertyChange(() => this.SubtitleType);
                this.NotifyOfPropertyChange(() => this.CanBeBurned);
                this.NotifyOfPropertyChange(() => this.CanBeForced);

                if (this.Forced && !this.CanBeForced)
                {
                    this.Forced = false;
                }

                if (this.Burned && !this.CanBeBurned)
                {
                    this.Forced = false;
                }

                if (TrackNamingCallback != null)
                {
                    bool passthruName = TrackNamingCallback();
                    if (passthruName)
                    {
                        this.SetTrackNamePassthru();
                    }
                }
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
        public Language SrtLang
        {
            get
            {
                return this.srtLang;
            }
            set
            {
                this.srtLang = value;
                this.SrtLangCode = value?.Code;
            }
        }

        /// <summary>
        /// Gets or sets the srt lang code.
        /// </summary>
        public string SrtLangCode { get; set; }

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

        public string Name
        {
            get => this.name;
            set
            {
                if (value == this.name) return;
                this.name = value;
                this.NotifyOfPropertyChange(() => this.Name);
            }
        }

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
                    return this.SourceTrack.CanForce || this.SourceTrack.IsFakeForeignAudioScanTrack;
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
                    return this.SourceTrack.CanBurnIn || this.SourceTrack.IsFakeForeignAudioScanTrack;
                }

                if (this.SubtitleType == SubtitleType.IMPORTSRT)
                {
                    return true;
                }

                if (this.SubtitleType == SubtitleType.IMPORTSSA)
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

        public void SetTrackNamePassthru()
        {
            if (this.SourceTrack != null)
            {
                this.Name = this.SourceTrack.Name;
            }
        }

        public Func<bool> TrackNamingCallback { get; set; }

        public override string ToString()
        {
            return string.Format("Subtitle Track: Title {0}", this.SrtFileName ?? this.SourceTrack.ToString());
        }
    }
}