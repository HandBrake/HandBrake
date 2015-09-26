// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AudioTrack.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Model of a HandBrake Audio Track and it's associated behaviours.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Encode.Model.Models
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Globalization;
    using System.Linq;

    using HandBrake.ApplicationServices.Interop;
    using HandBrake.ApplicationServices.Interop.Model;
    using HandBrake.ApplicationServices.Interop.Model.Encoding;
    using HandBrake.ApplicationServices.Utilities;

    using HandBrakeWPF.Services.Scan.Model;
    using HandBrakeWPF.Utilities;

    using Newtonsoft.Json;

    /// <summary>
    /// Model of a HandBrake Audio Track and it's associated behaviours.
    /// </summary>
    public class AudioTrack : PropertyChangedBase
    {
        private int bitrate;
        private double drc;
        private HandBrakeWPF.Services.Encode.Model.Models.AudioEncoder encoder;
        private int gain;
        private Mixdown mixDown;
        private double sampleRate;
        [NonSerialized]
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        private Audio scannedTrack;
        private bool isDefault;
        private IEnumerable<int> bitrates;
        private IEnumerable<double> encoderQualityValues;
        private HandBrakeWPF.Services.Encode.Model.Models.AudioEncoderRateType encoderRateType;
        private double? quality;

        /// <summary>
        ///   Initializes a new instance of the <see cref = "AudioTrack" /> class.
        /// </summary>
        public AudioTrack()
        {
            // Default Values
            this.Encoder = HandBrakeWPF.Services.Encode.Model.Models.AudioEncoder.ffaac;
            this.MixDown = Mixdown.DolbyProLogicII;
            this.SampleRate = 48;
            this.Bitrate = 160;
            this.DRC = 0;
            this.ScannedTrack = new Audio();
            this.TrackName = string.Empty;

            // Setup Backing Properties
            this.EncoderRateType = HandBrakeWPF.Services.Encode.Model.Models.AudioEncoderRateType.Bitrate;
            this.SetupLimits();
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
            this.Quality = track.Quality;

            // Setup Backing Properties
            this.encoderRateType = track.EncoderRateType;
            this.SetupLimits();
        }

        #region Track Properties

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
                this.SetupLimits();
                this.NotifyOfPropertyChange(() => this.TrackReference);
            }
        }

        /// <summary>
        ///   Gets or sets Audio Encoder
        /// </summary>
        public HandBrakeWPF.Services.Encode.Model.Models.AudioEncoder Encoder
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
                this.NotifyOfPropertyChange(() => this.IsBitrateVisible);
                this.NotifyOfPropertyChange(() => this.IsQualityVisible);
                this.NotifyOfPropertyChange(() => this.IsRateTypeVisible);
                this.SetupLimits();
                this.NotifyOfPropertyChange(() => this.TrackReference);

                // Refresh the available encoder rate types.
                this.NotifyOfPropertyChange(() => this.AudioEncoderRateTypes);
                if (!this.AudioEncoderRateTypes.Contains(this.EncoderRateType))
                {
                    this.EncoderRateType = HandBrakeWPF.Services.Encode.Model.Models.AudioEncoderRateType.Bitrate; // Default to bitrate.
                }
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
                this.SetupLimits();
                this.NotifyOfPropertyChange(() => this.TrackReference);
            }
        }

        /// <summary>
        /// Gets or sets the encoder rate type.
        /// </summary>
        public HandBrakeWPF.Services.Encode.Model.Models.AudioEncoderRateType EncoderRateType
        {
            get
            {
                return this.encoderRateType;
            }

            set
            {
                this.encoderRateType = value;
                this.SetupLimits();
                this.NotifyOfPropertyChange(() => this.EncoderRateType);
                this.NotifyOfPropertyChange(() => this.IsBitrateVisible);
                this.NotifyOfPropertyChange(() => this.IsQualityVisible);

                if (!this.Quality.HasValue)
                {
                    HBAudioEncoder hbAudioEncoder = HandBrakeEncoderHelpers.GetAudioEncoder(EnumHelper<HandBrakeWPF.Services.Encode.Model.Models.AudioEncoder>.GetShortName(this.Encoder));
                    this.Quality = HandBrakeEncoderHelpers.GetDefaultQuality(hbAudioEncoder);
                }
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
        ///   Gets or sets Audio quality
        /// </summary>
        public double? Quality
        {
            get
            {
                return this.quality;
            }

            set
            {
                this.quality = value;
                this.NotifyOfPropertyChange(() => this.quality);
            }
        }

        /// <summary>
        /// Gets or sets the track name.
        /// </summary>
        public string TrackName { get; set; }
        #endregion

        /// <summary>
        ///   Gets AudioEncoderDisplayValue.
        /// </summary>
        [JsonIgnore]
        public string AudioEncoderDisplayValue
        {
            get
            {
                return EnumHelper<HandBrakeWPF.Services.Encode.Model.Models.AudioEncoder>.GetDisplay(this.Encoder);
            }
        }

        /// <summary>
        ///   Gets AudioMixdownDisplayValue.
        /// </summary>
        [JsonIgnore]
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
        [JsonIgnore]
        public string BitRateDisplayValue
        {
            get
            {
                if (this.Encoder == HandBrakeWPF.Services.Encode.Model.Models.AudioEncoder.Ac3Passthrough || this.Encoder == HandBrakeWPF.Services.Encode.Model.Models.AudioEncoder.DtsPassthrough
                    || this.Encoder == HandBrakeWPF.Services.Encode.Model.Models.AudioEncoder.DtsHDPassthrough)
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
        [JsonIgnore]
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
        [JsonIgnore]
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
        [JsonIgnore]
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
        [JsonIgnore]
        public bool IsPassthru
        {
            get
            {
                if (this.Encoder == HandBrakeWPF.Services.Encode.Model.Models.AudioEncoder.Ac3Passthrough || this.Encoder == HandBrakeWPF.Services.Encode.Model.Models.AudioEncoder.DtsPassthrough
                    || this.Encoder == HandBrakeWPF.Services.Encode.Model.Models.AudioEncoder.DtsHDPassthrough || this.Encoder == HandBrakeWPF.Services.Encode.Model.Models.AudioEncoder.AacPassthru
                    || this.Encoder == HandBrakeWPF.Services.Encode.Model.Models.AudioEncoder.Mp3Passthru || this.Encoder == HandBrakeWPF.Services.Encode.Model.Models.AudioEncoder.Passthrough ||
                    this.Encoder == HandBrakeWPF.Services.Encode.Model.Models.AudioEncoder.EAc3Passthrough || this.Encoder == HandBrakeWPF.Services.Encode.Model.Models.AudioEncoder.TrueHDPassthrough
                    || this.Encoder == HandBrakeWPF.Services.Encode.Model.Models.AudioEncoder.FlacPassthru)
                {
                    return true;
                }
                return false;
            }
        }

        /// <summary>
        /// Gets the bitrates.
        /// </summary>
        [JsonIgnore]
        public IEnumerable<int> Bitrates
        {
            get
            {
                return this.bitrates;
            }
        }

        /// <summary>
        /// Gets the quality compression values.
        /// </summary>
        [JsonIgnore]
        public IEnumerable<double> EncoderQualityValues
        {
            get
            {
                return this.encoderQualityValues;
            }
        }

        /// <summary>
        /// Gets the audio encoder rate types.
        /// </summary>
        [JsonIgnore]
        public IEnumerable<HandBrakeWPF.Services.Encode.Model.Models.AudioEncoderRateType> AudioEncoderRateTypes
        {
            get
            {
                IList<HandBrakeWPF.Services.Encode.Model.Models.AudioEncoderRateType> types = EnumHelper<HandBrakeWPF.Services.Encode.Model.Models.AudioEncoderRateType>.GetEnumList().ToList();
                HBAudioEncoder hbaenc = HandBrakeEncoderHelpers.GetAudioEncoder(EnumHelper<HandBrakeWPF.Services.Encode.Model.Models.AudioEncoder>.GetShortName(this.Encoder));
                if (hbaenc == null || !hbaenc.SupportsQuality)
                {
                    types.Remove(HandBrakeWPF.Services.Encode.Model.Models.AudioEncoderRateType.Quality);
                }

                return types;
            }
        }

        /// <summary>
        /// Gets a value indicating whether can set bitrate.
        /// </summary>
        [JsonIgnore]
        public bool IsBitrateVisible
        {
            get
            {
                if (this.IsPassthru || this.Encoder == HandBrakeWPF.Services.Encode.Model.Models.AudioEncoder.ffflac || this.Encoder == HandBrakeWPF.Services.Encode.Model.Models.AudioEncoder.ffflac24)
                {
                    return false;
                }

                return Equals(this.EncoderRateType, HandBrakeWPF.Services.Encode.Model.Models.AudioEncoderRateType.Bitrate);
            }
        }

        /// <summary>
        /// Gets a value indicating whether is quality visible.
        /// </summary>
        [JsonIgnore]
        public bool IsQualityVisible
        {
            get
            {
                if (this.IsPassthru || this.Encoder == HandBrakeWPF.Services.Encode.Model.Models.AudioEncoder.ffflac || this.Encoder == HandBrakeWPF.Services.Encode.Model.Models.AudioEncoder.ffflac24)
                {
                    return false;
                }

                return Equals(this.EncoderRateType, HandBrakeWPF.Services.Encode.Model.Models.AudioEncoderRateType.Quality);
            }
        }

        /// <summary>
        /// Gets a value indicating whether is rate type visible.
        /// </summary>
        [JsonIgnore]
        public bool IsRateTypeVisible
        {
            get
            {
                if (this.IsPassthru || this.Encoder == HandBrakeWPF.Services.Encode.Model.Models.AudioEncoder.ffflac || this.Encoder == HandBrakeWPF.Services.Encode.Model.Models.AudioEncoder.ffflac24)
                {
                    return false;
                }

                return true;
            }
        }

        /// <summary>
        /// Gets a value indicating whether IsLossless.
        /// </summary>
        [JsonIgnore]
        public bool IsLossless
        {
            get
            {
                return this.IsPassthru || this.Encoder == HandBrakeWPF.Services.Encode.Model.Models.AudioEncoder.ffflac || this.Encoder == HandBrakeWPF.Services.Encode.Model.Models.AudioEncoder.ffflac24;
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

        #region Handler Methods

        /// <summary>
        /// The setup limits.
        /// </summary>
        private void SetupLimits()
        {
            this.SetupBitrateLimits();
            this.SetupQualityCompressionLimits();
        }

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
            HBAudioEncoder hbaenc = HandBrakeEncoderHelpers.GetAudioEncoder(EnumHelper<HandBrakeWPF.Services.Encode.Model.Models.AudioEncoder>.GetShortName(this.Encoder));
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

        /// <summary>
        /// The setup quality compression limits.
        /// </summary>
        private void SetupQualityCompressionLimits()
        {
            HBAudioEncoder hbAudioEncoder = HandBrakeEncoderHelpers.GetAudioEncoder(EnumHelper<HandBrakeWPF.Services.Encode.Model.Models.AudioEncoder>.GetShortName(this.Encoder));
            if (hbAudioEncoder.SupportsQuality)
            {
                RangeLimits limits = null;

                if (hbAudioEncoder.SupportsQuality)
                {
                    limits = hbAudioEncoder.QualityLimits;
                }

                if (limits != null)
                {
                    double value = limits.Ascending ? limits.Low : limits.High;
                    List<double> values = new List<double> { value };

                    if (limits.Ascending)
                    {
                        while (value < limits.High)
                        {
                            value += limits.Granularity;
                            values.Add(value);
                        }
                    }
                    else
                    {
                        while (value > limits.Low)
                        {
                            value -= limits.Granularity;
                            values.Add(value);
                        }
                    }

                    this.encoderQualityValues = values;
                }
                else
                {
                    this.encoderQualityValues = new List<double>();
                }
            }
            else
            {
                this.encoderQualityValues = new List<double>();
            }

            // Default the audio quality value if it's out of range.
            if (Equals(this.EncoderRateType, HandBrakeWPF.Services.Encode.Model.Models.AudioEncoderRateType.Quality))
            {
                if (this.Quality.HasValue && !this.encoderQualityValues.Contains(this.Quality.Value))
                {
                    this.Quality = HandBrakeEncoderHelpers.GetDefaultQuality(hbAudioEncoder);
                }
            }

            this.NotifyOfPropertyChange(() => this.EncoderQualityValues);
        }

        #endregion
    }
}