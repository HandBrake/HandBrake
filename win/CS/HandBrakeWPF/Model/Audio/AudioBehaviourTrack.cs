// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AudioBehaviourTrack.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Model of a HandBrake Audio Track and it's associated behaviours.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Model.Audio
{
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Globalization;
    using System.Linq;
    using System.Text.Json.Serialization;

    using HandBrake.App.Core.Utilities;
    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.Interfaces.Model.Encoders;

    using HandBrakeWPF.ViewModels;

    using Services.Encode.Model.Models;

    /// <summary>
    /// Model of a HandBrake Audio Track and it's associated behaviours.
    /// </summary>
    public class AudioBehaviourTrack : PropertyChangedBase
    {
        private int bitrate;
        private double drc;
        private HBAudioEncoder encoder;
        private int gain;
        private HBMixdown mixDown;
        private double sampleRate;
        private bool isDefault;
        private IEnumerable<int> bitrates;
        private IEnumerable<double> encoderQualityValues;
        private AudioEncoderRateType encoderRateType;
        private double? quality;
        private IEnumerable<HBMixdown> mixdowns;
        private HBAudioEncoder fallbackEncoder;

        /// <summary>
        /// Initializes a new instance of the <see cref="AudioBehaviourTrack"/> class. 
        /// </summary>
        public AudioBehaviourTrack(HBAudioEncoder fallback)
        {
            // Default Values
            this.Encoder = HandBrakeEncoderHelpers.GetAudioEncoder(HBAudioEncoder.AvAac);
            this.MixDown = HandBrakeEncoderHelpers.Mixdowns.FirstOrDefault(m => m.ShortName == "dpl2");
            this.SampleRate = 48;
            this.Bitrate = 160;
            this.DRC = 0;
            this.EncoderRateType = AudioEncoderRateType.Bitrate;
            this.fallbackEncoder = fallback;

            this.SetupLimits();
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="AudioBehaviourTrack"/> class. 
        /// Copy Constructor
        /// </summary>
        /// <param name="track">
        /// The track.
        /// </param>
        public AudioBehaviourTrack(AudioBehaviourTrack track)
        {
            this.bitrate = track.Bitrate;
            this.drc = track.DRC;
            this.encoder = track.Encoder;
            this.gain = track.Gain;
            this.mixDown = track.MixDown;
            this.sampleRate = track.SampleRate;
            this.Quality = track.Quality;
            this.encoderRateType = track.EncoderRateType;
            this.fallbackEncoder = track.fallbackEncoder;

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
        ///   Gets or sets Audio Mixdown (ShortName)
        /// </summary>
        public HBMixdown MixDown
        {
            get
            {
                return this.mixDown;
            }

            set
            {
                if (!object.Equals(this.mixDown, value))
                {
                    this.mixDown = value;
                    this.NotifyOfPropertyChange(() => this.MixDown);
                    this.SetupLimits();
                }
            }
        }

        /// <summary>
        ///   Gets or sets Audio Encoder
        /// </summary>
        public HBAudioEncoder Encoder
        {
            get
            {
                return this.encoder;
            }

            set
            {
                if (object.Equals(this.encoder, value))
                {
                    return;
                }

                this.encoder = value;
                this.NotifyOfPropertyChange(() => this.Encoder);
                this.NotifyOfPropertyChange(() => this.IsPassthru);
                this.NotifyOfPropertyChange(() => this.IsBitrateVisible);
                this.NotifyOfPropertyChange(() => this.IsQualityVisible);
                this.NotifyOfPropertyChange(() => this.IsRateTypeVisible);
                this.NotifyOfPropertyChange(() => this.TrackReference);
                this.GetDefaultMixdownIfNull();
                this.SetupLimits();

                // Refresh the available encoder rate types.
                this.NotifyOfPropertyChange(() => this.AudioEncoderRateTypes);
                if (!this.AudioEncoderRateTypes.Contains(this.EncoderRateType))
                {
                    this.EncoderRateType = AudioEncoderRateType.Bitrate; // Default to bitrate.
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
            }
        }

        /// <summary>
        /// Gets or sets the encoder rate type.
        /// </summary>
        public AudioEncoderRateType EncoderRateType
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
                    this.Quality = HandBrakeEncoderHelpers.GetDefaultQuality(this.Encoder);
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

        #endregion

        /// <summary>
        ///   Gets AudioEncoderDisplayValue.
        /// </summary>
        [JsonIgnore]
        public string AudioEncoderDisplayValue
        {
            get
            {
                return this.Encoder?.DisplayName;
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
                if (this.Encoder != null && this.Encoder.IsPassthrough)
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
        /// Gets a value indicating whether IsPassthru.
        /// </summary>
        [JsonIgnore]
        public bool IsPassthru
        {
            get
            {
                return this.Encoder != null && this.Encoder.IsPassthrough;
            }
        }

        public bool IsAutoPassthru => this.Encoder != null && this.Encoder.IsAutoPassthru;
        
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

        [JsonIgnore]
        public IEnumerable<HBMixdown> Mixdowns
        {
            get
            {
                return this.mixdowns;
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
        public IEnumerable<AudioEncoderRateType> AudioEncoderRateTypes
        {
            get
            {
                IList<AudioEncoderRateType> types = EnumHelper<AudioEncoderRateType>.GetEnumList().ToList();
                if (this.Encoder == null || !this.Encoder.SupportsQuality)
                {
                    types.Remove(AudioEncoderRateType.Quality);
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
                if (this.Encoder != null && this.Encoder.IsLosslessEncoder)
                {
                    return false;
                }

                return Equals(this.EncoderRateType, AudioEncoderRateType.Bitrate);
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
                if (this.Encoder != null && this.Encoder.IsLosslessEncoder)
                {
                    return false;
                }

                return Equals(this.EncoderRateType, AudioEncoderRateType.Quality);
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
                if (this.Encoder != null && this.Encoder.IsLosslessEncoder)
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
                return this.IsPassthru || this.IsLossless;
            }
        }

        /// <summary>
        /// Gets TrackReference.
        /// </summary>
        [JsonIgnore]
        public AudioBehaviourTrack TrackReference
        {
            get { return this; }
        }

        public void SetFallbackEncoder(HBAudioEncoder fallbackEncoder)
        {
            this.fallbackEncoder = fallbackEncoder;
            this.SetupLimits();
        }

        #region Handler Methods

        /// <summary>
        /// The setup limits.
        /// </summary>
        private void SetupLimits()
        {
            this.SetupBitrateLimits();
            this.SetupQualityCompressionLimits();
            this.SetupMixdowns();
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
            HBAudioEncoder hbaenc = GetEncoderForLimits();
            HBRate rate = HandBrakeEncoderHelpers.AudioSampleRates.FirstOrDefault(t => t.Name == this.SampleRate.ToString(CultureInfo.InvariantCulture));
            HBMixdown mixdown = this.mixDown ?? HandBrakeEncoderHelpers.GetMixdown("dpl2");

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
            HBAudioEncoder hbAudioEncoder = GetEncoderForLimits();
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
            if (Equals(this.EncoderRateType, AudioEncoderRateType.Quality))
            {
                if (this.Quality.HasValue && !this.encoderQualityValues.Contains(this.Quality.Value))
                {
                    this.Quality = HandBrakeEncoderHelpers.GetDefaultQuality(hbAudioEncoder);
                }
            }

            this.NotifyOfPropertyChange(() => this.EncoderQualityValues);
        }

        /// <summary>
        /// Restrict the available mixdowns to those that the encoder actually supports.
        /// </summary>
        private void SetupMixdowns()
        {
            this.mixdowns = new BindingList<HBMixdown>(HandBrakeEncoderHelpers.Mixdowns.ToList());
  
            HBAudioEncoder audioEncoder = GetEncoderForLimits();

            BindingList<HBMixdown> mixdownList = new BindingList<HBMixdown>();
            foreach (HBMixdown mixdown in HandBrakeEncoderHelpers.Mixdowns)
            {
                if (this.IsPassthru)
                {
                    if (HandBrakeEncoderHelpers.MixdownHasCodecSupport(mixdown, this.fallbackEncoder)) 
                    {
                        mixdownList.Add(mixdown);
                    }
                }
                else
                {
                    if (HandBrakeEncoderHelpers.MixdownHasCodecSupport(mixdown, audioEncoder)) // Show only supported, or all for passthru.
                    {
                        mixdownList.Add(mixdown);
                    }
                }
            }

            this.mixdowns = new BindingList<HBMixdown>(mixdownList);
            this.NotifyOfPropertyChange(() => this.Mixdowns);

            // If the mixdown isn't supported, downgrade it to the best available. 
            if (!this.Mixdowns.Contains(this.MixDown))
            {
                this.mixDown = this.Mixdowns.LastOrDefault();
                this.NotifyOfPropertyChange(() => this.MixDown);
            }
        }

        /// <summary>
        /// Set the default mixdown when the mixdown is null or "none"
        /// </summary>
        private void GetDefaultMixdownIfNull()
        {
            // if (this.ScannedTrack == null)
            // {
            //    return;
            // }

            // HBAudioEncoder aencoder = HandBrakeEncoderHelpers.GetAudioEncoder(EnumHelper<AudioEncoder>.GetShortName(this.encoder));
            // HBMixdown currentMixdown = HandBrakeEncoderHelpers.GetMixdown(this.mixDown);
            // HBMixdown sanitisedMixdown = HandBrakeEncoderHelpers.SanitizeMixdown(currentMixdown, aencoder, (uint)this.ScannedTrack.ChannelLayout);
            // HBMixdown defaultMixdown = HandBrakeEncoderHelpers.GetDefaultMixdown(aencoder, (uint)this.ScannedTrack.ChannelLayout);

            // if (this.mixDown == null || this.mixDown == "none")
            // {
            //    this.MixDown = defaultMixdown.ShortName;
            // }
            // else if (sanitisedMixdown != null)
            // {
            //    this.MixDown = sanitisedMixdown.ShortName;
            // }
        }

        private HBAudioEncoder GetEncoderForLimits()
        {
            HBAudioEncoder hbaenc = this.Encoder;
            if (hbaenc != null && hbaenc.IsPassthrough)
            {
                hbaenc = this.fallbackEncoder;
            }

            return hbaenc;
        }

        #endregion
    }
}