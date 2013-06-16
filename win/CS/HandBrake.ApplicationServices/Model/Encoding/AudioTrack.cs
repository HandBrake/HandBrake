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

    using Caliburn.Micro;

    using HandBrake.ApplicationServices.Parsing;
    using HandBrake.ApplicationServices.Utilities;
    using HandBrake.Interop.Model.Encoding;

    /// <summary>
    /// An Audio Track for the Audio Panel
    /// </summary>
    public class AudioTrack : PropertyChangedBase
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

        private string trackName;

        #endregion

        #region Constructors and Destructors

        /// <summary>
        ///   Initializes a new instance of the <see cref = "AudioTrack" /> class.
        /// </summary>
        public AudioTrack()
        {
            // Default Values
            this.Encoder = AudioEncoder.ffaac;
            this.MixDown = Mixdown.DolbyProLogicII;
            this.SampleRate = 48;
            this.Bitrate = 160;
            this.DRC = 0;
            this.ScannedTrack = new Audio();
            this.TrackName = string.Empty;
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
            this.scannedTrack = track.ScannedTrack ?? new Audio();
            this.TrackName = track.TrackName;
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
                this.NotifyOfPropertyChange(() => this.Bitrate);
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
                    this.NotifyOfPropertyChange(() => this.DRC);
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
                this.NotifyOfPropertyChange(() => this.Encoder);
                this.NotifyOfPropertyChange(() => this.IsPassthru);
                this.NotifyOfPropertyChange(() => this.CannotSetBitrate);
                this.NotifyOfPropertyChange(() => this.TrackReference);
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
                    this.NotifyOfPropertyChange(() => this.Gain);
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
                this.NotifyOfPropertyChange(() => this.MixDown);
                this.NotifyOfPropertyChange(() => this.TrackReference);
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
                this.NotifyOfPropertyChange(() => this.SampleRate);
                this.NotifyOfPropertyChange(() => this.TrackReference);
            }
        }

        /// <summary>
        ///  Gets or sets the The UI display value for sample rate
        /// </summary>
        public string SampleRateDisplayValue
        {
            get
            {
                return this.SampleRate == 0 ? "Auto" : this.SampleRate.ToString(CultureInfo.InvariantCulture);
            }
            set
            {
                // TODO change this to be a converted field
                if (string.IsNullOrEmpty(value))
                {
                    return;
                }

                double samplerate;
                double.TryParse(value, NumberStyles.Any, CultureInfo.InvariantCulture, out samplerate);

                this.SampleRate = samplerate;
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
                this.NotifyOfPropertyChange(() => this.ScannedTrack);
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
        /// Gets a value indicating whether IsPassthru.
        /// </summary>
        public bool IsPassthru
        {
            get
            {
                if (this.Encoder == AudioEncoder.Ac3Passthrough || this.Encoder == AudioEncoder.DtsPassthrough
                    || this.Encoder == AudioEncoder.DtsHDPassthrough || this.Encoder == AudioEncoder.AacPassthru
                    || this.Encoder == AudioEncoder.Mp3Passthru || this.Encoder == AudioEncoder.Passthrough)
                {
                    return true;
                }
                return false;
            }
        }

        /// <summary>
        /// Gets a value indicating whether can set bitrate.
        /// </summary>
        public bool CannotSetBitrate
        {
            get
            {
                return this.IsPassthru || this.Encoder == AudioEncoder.ffflac || this.Encoder == AudioEncoder.ffflac24;
            }
        }

        /// <summary>
        /// Gets a value indicating whether IsLossless.
        /// </summary>
        public bool IsLossless
        {
            get
            {
                return this.IsPassthru || this.Encoder == AudioEncoder.ffflac || this.Encoder == AudioEncoder.ffflac24;
            }
        }

        /// <summary>
        /// Gets TrackReference.
        /// </summary>
        public AudioTrack TrackReference
        {
            get { return this; }
        }

        /// <summary>
        /// Gets or sets the track name.
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
            }
        }

        #endregion
    }
}