// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AudioTrack.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Model of a HandBrake Audio Track and it's associated behaviours.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Services.Encode.Model.Models
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Globalization;
    using System.Linq;

    using HandBrake.ApplicationServices.Interop;
    using HandBrake.ApplicationServices.Interop.Model;
    using HandBrake.ApplicationServices.Interop.Model.Encoding;
    using HandBrake.ApplicationServices.Services.Scan.Model;
    using HandBrake.ApplicationServices.Utilities;

    using Newtonsoft.Json;

    /// <summary>
    /// Model of a HandBrake Audio Track and it's associated behaviours.
    /// </summary>
    public class AudioTrack : PropertyChangedBase
    {
        private int bitrate;
        private double drc;
        private AudioEncoder encoder;
        private int gain;
        private Mixdown mixDown;
        private double sampleRate;
        [NonSerialized]
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        private Audio scannedTrack;
        private bool isDefault;
        private IEnumerable<int> bitrates;

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

            // Setup Backing Properties
            this.SetupBitrateLimits();
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="AudioTrack"/> class.
        /// Copy Constructor
        /// </summary>
        /// <param name="track">
        /// The track.
        /// </param>
        /// <param name="setScannedTrack">
        /// The set Scanned Track.
        /// </param>
        public AudioTrack(AudioTrack track, bool setScannedTrack)
        {
            this.bitrate = track.Bitrate;
            this.drc = track.DRC;
            this.encoder = track.Encoder;
            this.gain = track.Gain;
            this.mixDown = track.MixDown;
            this.sampleRate = track.SampleRate;
            if (setScannedTrack)
            {
                this.scannedTrack = track.ScannedTrack ?? new Audio();
            }
            this.TrackName = track.TrackName;

            // Setup Backing Properties
            this.SetupBitrateLimits();
        }

        #region Track Properties

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
                if (!Equals(value, this.drc))
                {
                    this.drc = value;
                    this.NotifyOfPropertyChange(() => this.DRC);
                }
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
                if (!Equals(value, this.gain))
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
                this.SetupBitrateLimits();
                this.NotifyOfPropertyChange(() => this.TrackReference);
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
                this.SetupBitrateLimits();
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
                this.SetupBitrateLimits();
                this.NotifyOfPropertyChange(() => this.TrackReference);
            }
        }

        #endregion

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
        /// Gets or sets a value indicating whether is default.
        /// TODO - Can this be removed? May have been added as a quick fix for a styling quirk.
        /// </summary>
        public bool IsDefault
        {
            get
            {
                return this.isDefault;
            }
            set
            {
                this.isDefault = value;
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
                    || this.Encoder == AudioEncoder.Mp3Passthru || this.Encoder == AudioEncoder.Passthrough ||
                    this.Encoder == AudioEncoder.EAc3Passthrough || this.Encoder == AudioEncoder.TrueHDPassthrough
                    || this.Encoder == AudioEncoder.FlacPassthru)
                {
                    return true;
                }
                return false;
            }
        }

        /// <summary>
        /// Gets the bitrates.
        /// </summary>
        public IEnumerable<int> Bitrates
        {
            get
            {
                return this.bitrates;
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
        [JsonIgnore]
        public AudioTrack TrackReference
        {
            get { return this; }
        }

        /// <summary>
        /// Gets or sets the track name.
        /// </summary>
        public string TrackName { get; set; }

        /// <summary>
        /// The calculate bitrate limits.
        /// </summary>
        private void SetupBitrateLimits()
        {
            // Base set of bitrates available.
            List<int> audioBitrates = HandBrakeEncoderHelpers.AudioBitrates;

            // Defaults
            int max = 256;
            int low = 32;

            // Based on the users settings, find the high and low bitrates.
            HBAudioEncoder hbaenc = HandBrakeEncoderHelpers.GetAudioEncoder(EnumHelper<AudioEncoder>.GetShortName(this.Encoder));
            HBRate rate = HandBrakeEncoderHelpers.AudioSampleRates.FirstOrDefault(t => t.Name == this.SampleRate.ToString(CultureInfo.InvariantCulture));
            HBMixdown mixdown = HandBrakeEncoderHelpers.GetMixdown(EnumHelper<Mixdown>.GetShortName(this.MixDown));

            BitrateLimits limits = HandBrakeEncoderHelpers.GetBitrateLimits(hbaenc, rate != null ? rate.Rate : 48000, mixdown);
            if (limits != null)
            {
                max = limits.High;
                low = limits.Low;
            }

            // Return the subset of available bitrates.
            List<int> subsetBitrates = audioBitrates.Where(b => b <= max && b >= low).ToList();
            this.bitrates = subsetBitrates;
            this.NotifyOfPropertyChange(() => this.Bitrates);

            // If the subset does not contain the current bitrate, request the default.
            if (!subsetBitrates.Contains(this.Bitrate))
            {
                this.Bitrate = HandBrakeEncoderHelpers.GetDefaultBitrate(hbaenc, rate != null ? rate.Rate : 48000, mixdown);
            }
        }
    }
}