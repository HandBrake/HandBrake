// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AudioTrack.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   An Audio Track for the Audio Panel
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Model.Encoding
{
    using System;
    using System.ComponentModel;
    using System.Globalization;

    using HandBrake.ApplicationServices.Functions;
    using HandBrake.ApplicationServices.Parsing;
    using HandBrake.Interop.Model.Encoding;

    /// <summary>
    /// An Audio Track for the Audio Panel
    /// </summary>
    public class AudioTrack : ModelBase
    {
        #region Constants and Fields

        /// <summary>
        ///   The bitrate.
        /// </summary>
        private int bitrate;

        /// <summary>
        ///   The DRC Value
        /// </summary>
        private double drc;

        /// <summary>
        ///   The encoder.
        /// </summary>
        private AudioEncoder encoder;

        /// <summary>
        ///   The gain value
        /// </summary>
        private int gain;

        /// <summary>
        ///   The mix down.
        /// </summary>
        private Mixdown mixDown;

        /// <summary>
        ///   The sample rate.
        /// </summary>
        private double sampleRate;

        /// <summary>
        ///   The Scanned Audio Track
        /// </summary>
        [NonSerialized]
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        private Audio scannedTrack;

        /// <summary>
        /// The track name.
        /// </summary>
        private string trackName;

        #endregion

        #region Constructors and Destructors

        /// <summary>
        ///   Initializes a new instance of the <see cref = "AudioTrack" /> class.
        /// </summary>
        public AudioTrack()
        {
            // Default Values
            this.Encoder = AudioEncoder.Faac;
            this.MixDown = Mixdown.DolbyProLogicII;
            this.SampleRate = 48;
            this.Bitrate = 160;
            this.DRC = 0;
            this.ScannedTrack = new Audio();
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="AudioTrack"/> class.
        /// Copy Constructor
        /// </summary>
        /// <param name="track">
        /// The track.
        /// </param>
        public AudioTrack(AudioTrack track)
        {
            this.bitrate = track.Bitrate;
            this.drc = track.DRC;
            this.encoder = track.Encoder;
            this.gain = track.Gain;
            this.mixDown = track.MixDown;
            this.sampleRate = track.SampleRate;
            this.scannedTrack = new Audio();
            this.trackName = track.TrackName;
        }

        #endregion

        #region Public Properties

        /// <summary>
        ///   Gets AudioEncoderDisplayValue.
        /// </summary>
        public string AudioEncoderDisplayValue
        {
            get
            {
                return EnumHelper<AudioEncoder>.GetDisplay(this.Encoder);
            }
        }

        /// <summary>
        ///   Gets AudioMixdownDisplayValue.
        /// </summary>
        public string AudioMixdownDisplayValue
        {
            get
            {
                return EnumHelper<Mixdown>.GetDisplay(this.MixDown);
            }
        }

        /// <summary>
        ///   Gets the The UI display value for bit rate
        /// </summary>
        public string BitRateDisplayValue
        {
            get
            {
                if (this.Encoder == AudioEncoder.Ac3Passthrough || this.Encoder == AudioEncoder.DtsPassthrough
                    || this.Encoder == AudioEncoder.DtsHDPassthrough)
                {
                    return "Auto";
                }

                return this.Bitrate.ToString();
            }
        }

        /// <summary>
        ///   Gets or sets Audio Bitrate
        /// </summary>
        public int Bitrate
        {
            get
            {
                return this.bitrate;
            }

            set
            {
                this.bitrate = value;
                this.OnPropertyChanged("Bitrate");
            }
        }

        /// <summary>
        ///   Gets or sets Dynamic Range Compression
        /// </summary>
        public double DRC
        {
            get
            {
                return this.drc;
            }

            set
            {
                if (!object.Equals(value, this.drc))
                {
                    this.drc = value;
                    this.OnPropertyChanged("DRC");
                }
            }
        }

        /// <summary>
        ///   Gets or sets Audio Encoder
        /// </summary>
        public AudioEncoder Encoder
        {
            get
            {
                return this.encoder;
            }

            set
            {
                this.encoder = value;
                this.OnPropertyChanged("Encoder");
            }
        }

        /// <summary>
        ///   Gets or sets the Gain for the audio track
        /// </summary>
        public int Gain
        {
            get
            {
                return this.gain;
            }

            set
            {
                if (!object.Equals(value, this.gain))
                {
                    this.gain = value;
                    this.OnPropertyChanged("Gain");
                }
            }
        }

        /// <summary>
        ///   Gets or sets Audio Mixdown
        /// </summary>
        public Mixdown MixDown
        {
            get
            {
                return this.mixDown;
            }

            set
            {
                this.mixDown = value;
                this.OnPropertyChanged("MixDown");
            }
        }

        /// <summary>
        ///   Gets or sets Audio SampleRate
        /// </summary>
        public double SampleRate
        {
            get
            {
                return this.sampleRate;
            }

            set
            {
                this.sampleRate = value;
                this.OnPropertyChanged("SampleRate");
            }
        }

        /// <summary>
        ///   Gets the The UI display value for sample rate
        /// </summary>
        public string SampleRateDisplayValue
        {
            get
            {
                return this.SampleRate == 0 ? "Auto" : this.SampleRate.ToString(CultureInfo.InvariantCulture);
            }
        }

        /// <summary>
        ///   Gets or sets the Scanned Audio Tracks
        /// </summary>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        public Audio ScannedTrack
        {
            get
            {
                return this.scannedTrack;
            }

            set
            {
                this.scannedTrack = value;
                this.OnPropertyChanged("ScannedTrack");
                this.OnPropertyChanged("TrackDisplay");
            }
        }

        /// <summary>
        ///   Gets the Audio Track Name
        /// </summary>
        public int? Track
        {
            get
            {
                if (this.ScannedTrack != null)
                {
                    return this.ScannedTrack.TrackNumber;
                }

                return null;
            }
        }

        /// <summary>
        ///   Gets the Display Value for this model.
        /// </summary>
        public string TrackDisplay
        {
            get
            {
                return this.ScannedTrack == null ? string.Empty : this.ScannedTrack.ToString();
            }
        }

        /// <summary>
        ///   Gets or sets TrackName.
        /// </summary>
        public string TrackName
        {
            get
            {
                return this.trackName;
            }

            set
            {
                this.trackName = value;
                this.OnPropertyChanged("TrackName");
            }
        }

        #endregion
    }
}