﻿// --------------------------------------------------------------------------------------------------------------------
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
    using System.Text.Json.Serialization;

    using Windows.Media.Core;

    using Caliburn.Micro;

    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.Interfaces.Model.Encoders;

    using HandBrakeWPF.Model.Audio;
    using HandBrakeWPF.Services.Scan.Model;
    using HandBrakeWPF.Utilities;

    public class AudioTrack : PropertyChangedBase
    {
        private int bitrate;
        private double drc;
        private AudioEncoder encoder;
        private int gain;
        private string mixDown;
        private double sampleRate;
        [NonSerialized]
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        private Audio scannedTrack;
        private bool isDefault;
        private IEnumerable<int> bitrates;
        private IEnumerable<double> encoderQualityValues;
        private AudioEncoderRateType encoderRateType;
        private double? quality;
        private string trackName;

        private bool isExpandedTrackView;

        public AudioTrack()
        {
            // Default Values
            this.Encoder = AudioEncoder.ffaac;
            this.MixDown = HandBrakeEncoderHelpers.Mixdowns.FirstOrDefault(m => m.ShortName == "dpl2")?.ShortName;
            this.SampleRate = 48;
            this.Bitrate = 160;
            this.DRC = 0;
            this.ScannedTrack = new Audio();
            this.TrackName = string.Empty;

            // Setup Backing Properties
            this.EncoderRateType = AudioEncoderRateType.Bitrate;
            this.SetupLimits();
        }

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

            if (!string.IsNullOrEmpty(this.scannedTrack?.Name))
            {
                this.TrackName = this.scannedTrack.Name;
            }

            if (!string.IsNullOrEmpty(track.TrackName))
            {
                this.TrackName = track.TrackName;
            }

            this.Quality = track.Quality;

            // Setup Backing Properties
            this.encoderRateType = track.EncoderRateType;
            this.SetupLimits();
        }

        public AudioTrack(AudioBehaviourTrack track, Audio sourceTrack, AllowedPassthru fallback, OutputFormat container)
        {
            AudioEncoder chosenEncoder = track.Encoder;
            HBAudioEncoder encoderInfo = HandBrakeEncoderHelpers.GetAudioEncoder(EnumHelper<AudioEncoder>.GetShortName(track.Encoder));
            if (track.IsPassthru && (sourceTrack.Codec & encoderInfo.Id) == 0)
            {
                chosenEncoder = fallback.AudioEncoderFallback;
            }

            if (track.IsPassthru && chosenEncoder == AudioEncoder.Passthrough)
            {
                HBAudioEncoder fallbackEncoderInfo = HandBrakeEncoderHelpers.GetAudioEncoder(EnumHelper<AudioEncoder>.GetShortName(fallback.AudioEncoderFallback));

                if (fallbackEncoderInfo != null)
                {
                    int format = HandBrakeEncoderHelpers.GetContainer(EnumHelper<OutputFormat>.GetShortName(container)).Id;
                    int copyMask = checked((int)HandBrakeEncoderHelpers.BuildCopyMask(
                        fallback.AudioAllowMP2Pass,
                        fallback.AudioAllowMP3Pass,
                        fallback.AudioAllowAACPass,
                        fallback.AudioAllowAC3Pass,
                        fallback.AudioAllowDTSPass,
                        fallback.AudioAllowDTSHDPass,
                        fallback.AudioAllowEAC3Pass,
                        fallback.AudioAllowFlacPass,
                        fallback.AudioAllowTrueHDPass));

                    HBAudioEncoder autoPassthruEncoderOption = HandBrakeEncoderHelpers.GetAutoPassthruEncoder(sourceTrack.Codec, copyMask, fallbackEncoderInfo.Id, format);
                    AudioEncoder autoPassthru = EnumHelper<AudioEncoder>.GetValue(autoPassthruEncoderOption.ShortName);
                    chosenEncoder = autoPassthru;
                }
            }

            encoderInfo = HandBrakeEncoderHelpers.GetAudioEncoder(EnumHelper<AudioEncoder>.GetShortName(chosenEncoder));

            this.scannedTrack = sourceTrack;
            this.drc = track.DRC;
            this.encoder = chosenEncoder;
            this.gain = track.Gain;
            this.mixDown = track.MixDown != null ? track.MixDown.ShortName : "dpl2";

            // If the mixdown isn't supported, downgrade it.
            if (track.IsPassthru && track.MixDown != null && encoderInfo != null && !HandBrakeEncoderHelpers.MixdownIsSupported(track.MixDown, encoderInfo, sourceTrack.ChannelLayout))
            {
                HBMixdown changedMixdown = HandBrakeEncoderHelpers.GetDefaultMixdown(encoderInfo, (ulong)sourceTrack.ChannelLayout);
                if (changedMixdown != null)
                {
                    this.mixDown = changedMixdown.ShortName;
                }
            }
                
            this.sampleRate = track.SampleRate;
            this.encoderRateType = track.EncoderRateType;
            this.quality = track.Quality;
            this.bitrate = track.Bitrate;
            
            if (!string.IsNullOrEmpty(this.scannedTrack?.Name))
            {
                this.TrackName = this.scannedTrack.Name;
            }

            this.SetupLimits();
        }

        /* Audio Track Properties */

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

        public string MixDown
        {
            get
            {
                return this.IsPassthru ? null : this.mixDown;
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

        public AudioEncoder Encoder
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
                this.SetupLimits();
                this.GetDefaultMixdownIfNull();

                // Refresh the available encoder rate types.
                this.NotifyOfPropertyChange(() => this.AudioEncoderRateTypes);
                if (!this.AudioEncoderRateTypes.Contains(this.EncoderRateType))
                {
                    this.EncoderRateType = AudioEncoderRateType.Bitrate; // Default to bitrate.
                }
            }
        }

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
                    HBAudioEncoder hbAudioEncoder = HandBrakeEncoderHelpers.GetAudioEncoder(EnumHelper<AudioEncoder>.GetShortName(this.Encoder));
                    this.Quality = HandBrakeEncoderHelpers.GetDefaultQuality(hbAudioEncoder);
                }
            }
        }

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

        public string TrackName
        {
            get => this.trackName;
            set
            {
                if (value == this.trackName) return;
                this.trackName = value;
                this.NotifyOfPropertyChange();
            }
        }

        /* UI Only Properties */ 

        [JsonIgnore]
        public string AudioEncoderDisplayValue
        {
            get
            {
                return EnumHelper<AudioEncoder>.GetDisplay(this.Encoder);
            }
        }

        [JsonIgnore]
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
                this.NotifyOfPropertyChange(() => this.TrackReference);

                if (string.IsNullOrEmpty(this.TrackName))
                {
                    this.TrackName = !string.IsNullOrEmpty(this.scannedTrack?.Name) ? this.scannedTrack.Name : null;
                }
                
                this.GetDefaultMixdownIfNull();
            }
        }

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

        [JsonIgnore]
        public bool IsPassthru
        {
            get
            {
                if (this.Encoder == AudioEncoder.Ac3Passthrough || this.Encoder == AudioEncoder.DtsPassthrough
                    || this.Encoder == AudioEncoder.DtsHDPassthrough || this.Encoder == AudioEncoder.AacPassthru
                    || this.Encoder == AudioEncoder.Mp3Passthru || this.Encoder == AudioEncoder.Passthrough ||
                    this.Encoder == AudioEncoder.EAc3Passthrough || this.Encoder == AudioEncoder.TrueHDPassthrough
                    || this.Encoder == AudioEncoder.FlacPassthru || this.Encoder == AudioEncoder.Mp2Passthru)
                {
                    return true;
                }
                return false;
            }
        }

        [JsonIgnore]
        public IEnumerable<int> Bitrates
        {
            get
            {
                return this.bitrates;
            }
        }

        [JsonIgnore]
        public IEnumerable<double> EncoderQualityValues
        {
            get
            {
                return this.encoderQualityValues;
            }
        }

        [JsonIgnore]
        public IEnumerable<AudioEncoderRateType> AudioEncoderRateTypes
        {
            get
            {
                IList<AudioEncoderRateType> types = EnumHelper<AudioEncoderRateType>.GetEnumList().ToList();
                HBAudioEncoder hbaenc = HandBrakeEncoderHelpers.GetAudioEncoder(EnumHelper<AudioEncoder>.GetShortName(this.Encoder));
                if (hbaenc == null || !hbaenc.SupportsQuality)
                {
                    types.Remove(AudioEncoderRateType.Quality);
                }

                return types;
            }
        }

        [JsonIgnore]
        public bool IsBitrateVisible
        {
            get
            {
                if (this.IsPassthru || this.Encoder == AudioEncoder.ffflac || this.Encoder == AudioEncoder.ffflac24)
                {
                    return false;
                }

                return Equals(this.EncoderRateType, AudioEncoderRateType.Bitrate);
            }
        }

        [JsonIgnore]
        public bool IsQualityVisible
        {
            get
            {
                if (this.IsPassthru || this.Encoder == AudioEncoder.ffflac || this.Encoder == AudioEncoder.ffflac24)
                {
                    return false;
                }

                return Equals(this.EncoderRateType, AudioEncoderRateType.Quality);
            }
        }

        [JsonIgnore]
        public bool IsRateTypeVisible
        {
            get
            {
                if (this.IsPassthru || this.Encoder == AudioEncoder.ffflac || this.Encoder == AudioEncoder.ffflac24)
                {
                    return false;
                }

                return true;
            }
        }

        [JsonIgnore]
        public bool IsLossless
        {
            get
            {
                return this.IsPassthru || this.Encoder == AudioEncoder.ffflac || this.Encoder == AudioEncoder.ffflac24;
            }
        }

        [JsonIgnore]
        public AudioTrack TrackReference
        {
            get { return this; }
        }

        [JsonIgnore]
        public bool IsExpandedTrackView
        {
            get => this.isExpandedTrackView;
            set
            {
                if (value == this.isExpandedTrackView) return;
                this.isExpandedTrackView = value;
                this.NotifyOfPropertyChange(() => this.IsExpandedTrackView);
            }
        }

        /* Helper Methods */

        private void SetupLimits()
        {
            this.SetupBitrateLimits();
            this.SetupQualityCompressionLimits();
            this.GetDefaultMixdownIfNull();
        }

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
            HBMixdown mixdown = this.mixDown != null ? HandBrakeEncoderHelpers.GetMixdown(this.mixDown) : HandBrakeEncoderHelpers.GetMixdown("dpl2");

            if (hbaenc != null)
            {
                BitrateLimits limits = HandBrakeEncoderHelpers.GetBitrateLimits(hbaenc, rate != null ? rate.Rate : 48000, mixdown);
                if (limits != null)
                {
                    max = limits.High;
                    low = limits.Low;
                }
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

        private void SetupQualityCompressionLimits()
        {
            HBAudioEncoder hbAudioEncoder = HandBrakeEncoderHelpers.GetAudioEncoder(EnumHelper<AudioEncoder>.GetShortName(this.Encoder));
            if (hbAudioEncoder != null && hbAudioEncoder.SupportsQuality)
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

        private void GetDefaultMixdownIfNull()
        {
            if (this.ScannedTrack == null)
            {
                return;
            }

            HBAudioEncoder aencoder = HandBrakeEncoderHelpers.GetAudioEncoder(EnumHelper<AudioEncoder>.GetShortName(this.encoder));
            HBMixdown currentMixdown = HandBrakeEncoderHelpers.GetMixdown(this.mixDown);
            HBMixdown sanitisedMixdown = HandBrakeEncoderHelpers.SanitizeMixdown(currentMixdown, aencoder, (uint)this.ScannedTrack.ChannelLayout);
            HBMixdown defaultMixdown = sanitisedMixdown;
            if (aencoder != null)
            {
                defaultMixdown = HandBrakeEncoderHelpers.GetDefaultMixdown(aencoder, (uint)this.ScannedTrack.ChannelLayout);
            }
         
            if (this.mixDown == null || this.mixDown == "none")
            {
                this.MixDown = defaultMixdown?.ShortName;
            }
            else if (sanitisedMixdown != null)
            {
                this.MixDown = sanitisedMixdown.ShortName;
            }
        }

        public override string ToString()
        {
            return string.Format("Audio Track: Title {0}", this.ScannedTrack.ToString());
        }
    }
}