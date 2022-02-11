// --------------------------------------------------------------------------------------------------------------------
// <copyright file="VideoViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Video View Model
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Globalization;
    using System.Linq;

    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.Interfaces.Model.Encoders;

    using HandBrakeWPF.EventArgs;
    using HandBrakeWPF.Model;
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.Services.Scan.Model;
    using HandBrakeWPF.Utilities;
    using HandBrakeWPF.ViewModels.Interfaces;

    using Clipboard = System.Windows.Clipboard;
    using EncodeTask = Services.Encode.Model.EncodeTask;
    using FramerateMode = Services.Encode.Model.Models.FramerateMode;
    using OutputFormat = Services.Encode.Model.Models.OutputFormat;
    using SettingChangedEventArgs = EventArgs.SettingChangedEventArgs;
    using VideoEncoder = Model.Video.VideoEncoder;
    using VideoEncodeRateType = Model.Video.VideoEncodeRateType;
    using VideoLevel = Services.Encode.Model.Models.Video.VideoLevel;
    using VideoPreset = Services.Encode.Model.Models.Video.VideoPreset;
    using VideoProfile = Services.Encode.Model.Models.Video.VideoProfile;
    using VideoTune = Services.Encode.Model.Models.Video.VideoTune;

    public class VideoViewModel : ViewModelBase, IVideoViewModel
    {
        private const string SameAsSource = "Same as source";
        private readonly IUserSettingService userSettingService;
        private readonly IErrorService errorService;

        private bool displayOptimiseOptions;
        private int qualityMax;
        private int qualityMin;
        private bool showPeakFramerate;
        private int rf;
        private bool displayTurboFirstPass;
        private int videoPresetMaxValue;
        private int videoPresetValue;
        private VideoTune videoTune;
        private bool displayTuneControls;
        private bool displayLevelControl;
        private bool displayProfileControl;
        private Dictionary<string, string> encoderOptions = new Dictionary<string, string>();

        public VideoViewModel(IUserSettingService userSettingService, IErrorService errorService)
        {
            this.Task = new EncodeTask { VideoEncoder = VideoEncoder.X264 };
            this.userSettingService = userSettingService;
            this.errorService = errorService;
            this.QualityMin = 0;
            this.QualityMax = 51;
            this.IsConstantQuantity = true;
            this.VideoEncoders = EnumHelper<VideoEncoder>.GetEnumList();

            this.VideoProfiles = new BindingList<VideoProfile>();
            this.VideoTunes = new BindingList<VideoTune>();
            this.VideoPresets = new BindingList<VideoPreset>();
            this.VideoLevels = new BindingList<VideoLevel>();

            this.userSettingService.SettingChanged += this.UserSettingServiceSettingChanged;
        }

        public event EventHandler<TabStatusEventArgs> TabStatusChanged;

        public IUserSettingService UserSettingService => this.userSettingService;

        public EncodeTask Task { get; set; }

        public IEnumerable<string> Framerates
        {
            get
            {
                List<string> framerates = new List<string> { "Same as source" };
                framerates.AddRange(HandBrakeEncoderHelpers.VideoFramerates.Select(item => item.Name));
                return framerates;
            }
        }

        public bool IsConstantFramerate
        {
            get => this.Task.FramerateMode == FramerateMode.CFR;
            set
            {
                if (value)
                {
                    this.Task.FramerateMode = FramerateMode.CFR;
                    this.IsVariableFramerate = false;
                    this.IsPeakFramerate = false;
                }

                this.NotifyOfPropertyChange(() => this.IsConstantFramerate);
                this.OnTabStatusChanged(null);
            }
        }

        public bool IsConstantQuantity
        {
            get => this.Task.VideoEncodeRateType == VideoEncodeRateType.ConstantQuality;
            set
            {
                if (value)
                {
                    this.Task.VideoEncodeRateType = VideoEncodeRateType.ConstantQuality;
                    this.TwoPass = false;
                    this.TurboFirstPass = false;
                    this.VideoBitrate = null;
                    this.NotifyOfPropertyChange(() => this.Task);
                }
                else
                {
                    this.Task.VideoEncodeRateType = VideoEncodeRateType.AverageBitrate;
                }

                this.NotifyOfPropertyChange(() => this.IsConstantQuantity);
                this.NotifyOfPropertyChange(() => this.IsTwoPassEnabled);
                this.OnTabStatusChanged(null);
            }
        }

        public bool IsTwoPassEnabled
        {
            get
            {
                if (this.IsConstantQuantity)
                {
                    return false;
                }

                if (!HandBrakeEncoderHelpers.VideoEncoderSupportsTwoPass(EnumHelper<VideoEncoder>.GetShortName(this.SelectedVideoEncoder)))
                {
                    return false;
                }

                return true;
            }
        }

        public bool IsPeakFramerate
        {
            get => this.Task.FramerateMode == FramerateMode.PFR;
            set
            {
                if (value)
                {
                    this.Task.FramerateMode = FramerateMode.PFR;
                    this.IsVariableFramerate = false;
                    this.IsConstantFramerate = false;
                }

                this.NotifyOfPropertyChange(() => this.IsPeakFramerate);
                this.OnTabStatusChanged(null);
            }
        }

        public bool IsVariableFramerate
        {
            get => this.Task.FramerateMode == FramerateMode.VFR;
            set
            {
                if (value)
                {
                    this.IsPeakFramerate = false;
                    this.IsConstantFramerate = false;
                    this.Task.FramerateMode = FramerateMode.VFR;
                }

                this.NotifyOfPropertyChange(() => this.IsVariableFramerate);
                this.OnTabStatusChanged(null);
            }
        }

        public bool IsLossless
        {
            get => 0.0.Equals(this.DisplayRF) && (this.SelectedVideoEncoder == VideoEncoder.X264 || this.SelectedVideoEncoder == VideoEncoder.X264_10);
        }

        public int QualityMax
        {
            get => this.qualityMax;
            set
            {
                if (!qualityMax.Equals(value))
                {
                    this.qualityMax = value;
                    this.NotifyOfPropertyChange(() => this.QualityMax);
                }
            }
        }

        public int QualityMin
        {
            get => this.qualityMin;
            set
            {
                if (!qualityMin.Equals(value))
                {
                    this.qualityMin = value;
                    this.NotifyOfPropertyChange(() => this.QualityMin);
                }
            }
        }

        public int RF
        {
            get => rf;
            set
            {
                this.rf = value;

                this.SetQualitySliderBounds();
                switch (this.SelectedVideoEncoder)
                {
                    case VideoEncoder.FFMpeg:
                    case VideoEncoder.FFMpeg2:
                        this.Task.Quality = (32 - value);
                        break;
                    case VideoEncoder.VP8:
                    case VideoEncoder.VP9:
                        this.Task.Quality = (63 - value);
                        break;
                    case VideoEncoder.X264:
                    case VideoEncoder.X264_10:
                    case VideoEncoder.X265:
                    case VideoEncoder.X265_10:
                    case VideoEncoder.X265_12:
                    case VideoEncoder.VceH264:
                    case VideoEncoder.VceH265:
                    case VideoEncoder.NvencH264:
                    case VideoEncoder.NvencH265:
                    case VideoEncoder.NvencH26510b:
                    case VideoEncoder.MFH264:
                    case VideoEncoder.MFH265:
                        double cqStep = userSettingService.GetUserSetting<double>(UserSettingConstants.X264Step);
                        this.Task.Quality = Math.Round(51.0 - (value * cqStep), 2);
                        break;
                    case VideoEncoder.QuickSync:
                    case VideoEncoder.QuickSyncH265:
                        this.Task.Quality = Math.Round(51.0 - (value - 0), 0);
                        break;
                    case VideoEncoder.QuickSyncH26510b:
                        this.Task.Quality = Math.Round(63.0 - (value - 0), 0);
                        break;
                    case VideoEncoder.Theora:
                        Task.Quality = value;
                        break;
                }

                this.NotifyOfPropertyChange(() => this.RF);
                this.NotifyOfPropertyChange(() => this.DisplayRF);
                this.NotifyOfPropertyChange(() => this.IsLossless);
                this.OnTabStatusChanged(new TabStatusEventArgs("filters", ChangedOption.Quality));
            }
        }

        public int? VideoBitrate
        {
            get => this.Task.VideoBitrate;
            set
            {
                if (value == this.Task.VideoBitrate)
                {
                    return;
                }
                this.Task.VideoBitrate = value;
                this.NotifyOfPropertyChange(() => this.VideoBitrate);
                this.OnTabStatusChanged(new TabStatusEventArgs("filters", ChangedOption.Bitrate));
            }
        }

        public double DisplayRF
        {
            get => Task.Quality.HasValue ? this.Task.Quality.Value : 0;
        }

        public bool TwoPass
        {
            get => this.Task.TwoPass;

            set
            {
                this.Task.TwoPass = value;
                this.NotifyOfPropertyChange(() => this.TwoPass);
                this.OnTabStatusChanged(null);
            }
        }

        public bool TurboFirstPass
        {
            get => this.Task.TurboFirstPass;

            set
            {
                this.Task.TurboFirstPass = value;
                this.NotifyOfPropertyChange(() => this.TurboFirstPass);
                this.OnTabStatusChanged(null);
            }
        }

        public string Rfqp
        {
            get => HandBrakeEncoderHelpers.GetVideoQualityRateControlName(EnumHelper<VideoEncoder>.GetShortName(this.SelectedVideoEncoder));
        }

        public string HighQualityLabel
        {
            get => this.SelectedVideoEncoder == VideoEncoder.X264 || this.SelectedVideoEncoder == VideoEncoder.X264_10 ? Resources.Video_PlaceboQuality : Resources.Video_HigherQuality;
        }

        public string SelectedFramerate
        {
            get
            {
                if (this.Task.Framerate == null)
                {
                    return "Same as source";
                }

                return this.Task.Framerate.Value.ToString(CultureInfo.InvariantCulture);
            }
            set
            {
                if (value == "Same as source" || value == null)
                {
                    this.Task.Framerate = null;
                    this.ShowPeakFramerate = false;

                    if (this.Task.FramerateMode == FramerateMode.PFR)
                    {
                        this.IsVariableFramerate = true;
                    }
                }
                else if (!string.IsNullOrEmpty(value))
                {
                    this.ShowPeakFramerate = true;
                    if (this.Task.FramerateMode == FramerateMode.VFR)
                    {
                        this.IsPeakFramerate = true;
                    }
                    this.Task.Framerate = double.Parse(value, CultureInfo.InvariantCulture);
                }

                this.NotifyOfPropertyChange(() => this.SelectedFramerate);
                this.NotifyOfPropertyChange(() => this.Task);
                this.OnTabStatusChanged(null);
            }
        }

        public VideoEncoder SelectedVideoEncoder
        {
            get => this.Task.VideoEncoder;
            set
            {
                if (!object.Equals(value, this.Task.VideoEncoder))
                {
                    // Cache the current extra args. We can set them back later if the user switches back
                    this.encoderOptions[EnumHelper<VideoEncoder>.GetShortName(this.Task.VideoEncoder)] = this.ExtraArguments;

                    this.Task.VideoEncoder = value;
                    this.NotifyOfPropertyChange(() => this.SelectedVideoEncoder);
                    this.HandleEncoderChange(this.Task.VideoEncoder);
                    this.HandleRFChange();
                    this.OnTabStatusChanged(null);
                }
            }
        }
        public bool ShowPeakFramerate
        {
            get => this.showPeakFramerate;
            set
            {
                this.showPeakFramerate = value;
                this.NotifyOfPropertyChange(() => this.ShowPeakFramerate);
            }
        }

        public IEnumerable<VideoEncoder> VideoEncoders { get; set; }

        public string ExtraArguments
        {
            get => this.Task.ExtraAdvancedArguments;
            set
            {
                if (!Equals(this.Task.ExtraAdvancedArguments, value))
                {
                    this.Task.ExtraAdvancedArguments = value;
                    this.NotifyOfPropertyChange(() => this.ExtraArguments);
                    this.NotifyOfPropertyChange(() => FullOptionsTooltip);
                    this.OnTabStatusChanged(null);
                }
            }
        }

        public bool DisplayOptimiseOptions
        {
            get => this.displayOptimiseOptions;

            set
            {
                this.displayOptimiseOptions = value;
                this.NotifyOfPropertyChange(() => this.DisplayOptimiseOptions);
                this.NotifyOfPropertyChange(() => FullOptionsTooltip);
            }
        }

        public bool DisplayTwoPass
        {
            get => HandBrakeEncoderHelpers.VideoEncoderSupportsTwoPass(EnumHelper<VideoEncoder>.GetShortName(this.SelectedVideoEncoder));
        }

        public bool DisplayTuneControls
        {
            get => this.displayTuneControls;
            set
            {
                if (value.Equals(this.displayTuneControls))
                {
                    return;
                }
                this.displayTuneControls = value;
                this.NotifyOfPropertyChange(() => this.DisplayTuneControls);
            }
        }

        public bool DisplayFastDecode { get; set; }

        public bool DisplayLevelControl
        {
            get => this.displayLevelControl;
            set
            {
                if (value.Equals(this.displayLevelControl))
                {
                    return;
                }
                this.displayLevelControl = value;
                this.NotifyOfPropertyChange(() => this.DisplayLevelControl);
            }
        }

        public bool DisplayProfileControl
        {
            get => this.displayProfileControl;
            set
            {
                if (value.Equals(this.displayProfileControl))
                {
                    return;
                }
                this.displayProfileControl = value;
                this.NotifyOfPropertyChange(() => this.DisplayProfileControl);
            }
        }

        public bool FastDecode
        {
            get => this.Task.VideoTunes.Contains(VideoTune.FastDecode);
            set
            {
                // Update the encode task
                if (value && !this.Task.VideoTunes.Contains(VideoTune.FastDecode))
                {
                    this.Task.VideoTunes.Add(VideoTune.FastDecode);
                }
                else
                {
                    this.Task.VideoTunes.Remove(VideoTune.FastDecode);
                }

                this.NotifyOfPropertyChange(() => this.FastDecode);
                this.NotifyOfPropertyChange(() => this.FullOptionsTooltip);
                this.OnTabStatusChanged(null);
            }
        }

        public VideoPreset VideoPreset
        {
            get => this.Task.VideoPreset;
            set
            {
                this.Task.VideoPreset = value;
                this.NotifyOfPropertyChange(() => this.VideoPreset);
                this.NotifyOfPropertyChange(() => this.FullOptionsTooltip);
                this.OnTabStatusChanged(null);
            }
        }

        public int VideoPresetValue
        {
            get => this.videoPresetValue;
            set
            {
                this.videoPresetValue = value;

                HBVideoEncoder encoder = HandBrakeEncoderHelpers.VideoEncoders.FirstOrDefault(s => s.ShortName == EnumHelper<VideoEncoder>.GetShortName(this.SelectedVideoEncoder));
                if (encoder != null)
                {
                    string preset = value >= 0 ? encoder.Presets[value] : null;
                    this.VideoPreset = preset != null ? new VideoPreset(preset, preset) : this.VideoPresets.FirstOrDefault();
                }

                this.NotifyOfPropertyChange(() => this.VideoPresetValue);
            }
        }

        public int VideoPresetMaxValue
        {
            get => this.videoPresetMaxValue;

            set
            {
                if (value == this.videoPresetMaxValue)
                {
                    return;
                }
                this.videoPresetMaxValue = value;
                this.NotifyOfPropertyChange(() => this.VideoPresetMaxValue);
            }
        }

        public VideoTune VideoTune
        {
            get => this.videoTune;
            set
            {
                if (Equals(value, this.videoTune))
                {
                    return;
                }
                this.videoTune = value;

                // Update the encode task.
                bool hasFastDecode = this.Task.VideoTunes.Contains(VideoTune.FastDecode);
                this.Task.VideoTunes.Clear();
                if (value != null && !Equals(value, VideoTune.None))
                {
                    this.Task.VideoTunes.Add(value);
                }

                if ((this.SelectedVideoEncoder == VideoEncoder.X264 || this.SelectedVideoEncoder == VideoEncoder.X264_10) && hasFastDecode)
                {
                    this.Task.VideoTunes.Add(VideoTune.FastDecode);
                }

                this.NotifyOfPropertyChange(() => this.VideoTune);
                this.NotifyOfPropertyChange(() => this.FullOptionsTooltip);
                this.OnTabStatusChanged(null);
            }
        }

        public VideoProfile VideoProfile
        {
            get => this.Task.VideoProfile;
            set
            {
                this.Task.VideoProfile = value;
                this.NotifyOfPropertyChange(() => this.VideoProfile);
                this.NotifyOfPropertyChange(() => this.FullOptionsTooltip);
                this.OnTabStatusChanged(null);
            }
        }

        public VideoLevel VideoLevel
        {
            get => this.Task.VideoLevel;
            set
            {
                this.Task.VideoLevel = value;
                this.NotifyOfPropertyChange(() => this.VideoLevel);
                this.NotifyOfPropertyChange(() => this.FullOptionsTooltip);
                this.OnTabStatusChanged(null);
            }
        }

        public BindingList<VideoPreset> VideoPresets { get; set; }

        public BindingList<VideoTune> VideoTunes { get; set; }

        public BindingList<VideoProfile> VideoProfiles { get; set; }

        public BindingList<VideoLevel> VideoLevels { get; set; }

        public string FullOptionsTooltip
        {
            get => this.SelectedVideoEncoder == VideoEncoder.X264 || this.SelectedVideoEncoder == VideoEncoder.X264_10 ? string.Format(Resources.Video_EncoderExtraArgs, this.GetActualx264Query()) : Resources.Video_EncoderExtraArgsTooltip;
        }

        public bool DisplayTurboFirstPass
        {
            get => this.displayTurboFirstPass;
            set
            {
                if (value.Equals(this.displayTurboFirstPass))
                {
                    return;
                }
                this.displayTurboFirstPass = value;
                this.NotifyOfPropertyChange(() => this.DisplayTurboFirstPass);
            }
        }

        public void SetSource(Source source, Title title, Preset preset, EncodeTask task)
        {
            this.Task = task;
        }

        public void SetPreset(Preset preset, EncodeTask task)
        {
            this.Task = task;
            if (preset == null || preset.Task == null)
            {
                return;
            }

            this.SelectedVideoEncoder = preset.Task.VideoEncoder;
            this.SelectedFramerate = preset.Task.Framerate.HasValue ? preset.Task.Framerate.Value.ToString(CultureInfo.InvariantCulture) : SameAsSource;

            this.IsConstantQuantity = preset.Task.VideoEncodeRateType == VideoEncodeRateType.ConstantQuality;

            switch (preset.Task.FramerateMode)
            {
                case FramerateMode.CFR:
                    this.IsConstantFramerate = true;
                    break;
                case FramerateMode.VFR:
                    this.IsVariableFramerate = true;
                    this.ShowPeakFramerate = false;
                    break;
                case FramerateMode.PFR:
                    this.IsPeakFramerate = true;
                    this.ShowPeakFramerate = true;
                    break;
            }

            this.TwoPass = preset.Task.TwoPass;
            this.TurboFirstPass = preset.Task.TurboFirstPass;

            this.VideoBitrate = preset.Task.VideoEncodeRateType == VideoEncodeRateType.AverageBitrate ? preset.Task.VideoBitrate : null;

            this.NotifyOfPropertyChange(() => this.Task);

            this.HandleEncoderChange(preset.Task.VideoEncoder);
            this.SetQualitySliderBounds();
            this.SetRF(preset.Task.Quality);

            HBVideoEncoder encoder = HandBrakeEncoderHelpers.VideoEncoders.FirstOrDefault(s => s.ShortName == EnumHelper<VideoEncoder>.GetShortName(preset.Task.VideoEncoder));
            if (encoder != null)
            {
                this.VideoLevel = encoder.Levels.Count > 0
                    ? preset.Task.VideoLevel != null ? preset.Task.VideoLevel.Clone() :
                    this.VideoLevels.FirstOrDefault()
                    : null;

                this.VideoProfile = encoder.Profiles.Count > 0
                    ? preset.Task.VideoProfile != null ? preset.Task.VideoProfile.Clone() :
                    this.VideoProfiles.FirstOrDefault()
                    : null;

                this.VideoPresetValue = encoder.Presets.Count > 0
                    ? preset.Task.VideoPreset != null ? this.VideoPresets.IndexOf(preset.Task.VideoPreset) : 0
                    : 0;

                if (preset.Task.VideoEncoder == VideoEncoder.X265 || preset.Task.VideoEncoder == VideoEncoder.X265_10 || preset.Task.VideoEncoder == VideoEncoder.X265_12)
                {
                    this.FastDecode = false;
                    this.VideoTune = (preset.Task.VideoTunes != null && preset.Task.VideoTunes.Any() ? preset.Task.VideoTunes.FirstOrDefault() : this.VideoTunes.FirstOrDefault()) ?? VideoTune.None;
                }
                else
                {
                    this.FastDecode = preset.Task.VideoTunes != null && preset.Task.VideoTunes.Contains(VideoTune.FastDecode);
                    this.VideoTune = (preset.Task.VideoTunes != null && preset.Task.VideoTunes.Any() ? preset.Task.VideoTunes.FirstOrDefault(t => !Equals(t, VideoTune.FastDecode)) : this.VideoTunes.FirstOrDefault())
                                     ?? VideoTune.None;
                }
            }

            this.ExtraArguments = preset.Task.ExtraAdvancedArguments;
        }

        public void UpdateTask(EncodeTask task)
        {
            this.Task = task;
            this.SetQualitySliderBounds();
            this.SetRF(task.Quality);

            this.ShowPeakFramerate = this.IsPeakFramerate;

            this.NotifyOfPropertyChange(() => this.IsConstantFramerate);
            this.NotifyOfPropertyChange(() => this.IsConstantQuantity);
            this.NotifyOfPropertyChange(() => this.IsPeakFramerate);
            this.NotifyOfPropertyChange(() => this.IsVariableFramerate);
            this.NotifyOfPropertyChange(() => this.SelectedVideoEncoder);
            this.NotifyOfPropertyChange(() => this.SelectedFramerate);
            this.NotifyOfPropertyChange(() => this.QualityMax);
            this.NotifyOfPropertyChange(() => this.QualityMin);
            this.NotifyOfPropertyChange(() => this.RF);
            this.NotifyOfPropertyChange(() => this.DisplayRF);
            this.NotifyOfPropertyChange(() => this.IsLossless);
            this.NotifyOfPropertyChange(() => this.VideoBitrate);
            this.NotifyOfPropertyChange(() => this.Task.Quality);
            this.NotifyOfPropertyChange(() => this.Task.TwoPass);
            this.NotifyOfPropertyChange(() => this.Task.TurboFirstPass);
            this.NotifyOfPropertyChange(() => this.VideoTune);
            this.NotifyOfPropertyChange(() => this.VideoProfile);
            this.NotifyOfPropertyChange(() => this.VideoPreset);
            this.NotifyOfPropertyChange(() => this.VideoLevel);
            this.NotifyOfPropertyChange(() => this.FastDecode);
            this.NotifyOfPropertyChange(() => this.ExtraArguments);

            this.VideoTune = (task.VideoTunes != null && task.VideoTunes.Any() ? task.VideoTunes.FirstOrDefault(t => !Equals(t, VideoTune.FastDecode)) : this.VideoTunes.FirstOrDefault())
                             ?? VideoTune.None;

            HBVideoEncoder encoder = HandBrakeEncoderHelpers.VideoEncoders.FirstOrDefault(s => s.ShortName == EnumHelper<VideoEncoder>.GetShortName(this.SelectedVideoEncoder));
            if (encoder != null && this.VideoPreset != null)
            {
                int index = encoder.Presets.IndexOf(this.VideoPreset.ShortName);
                this.VideoPresetValue = index;
            }
        }

        public bool MatchesPreset(Preset preset)
        {
            if (preset.Task.VideoEncoder != this.Task.VideoEncoder)
            {
                return false;
            }

            if (preset.Task.Framerate != this.Task.Framerate)
            {
                return false;
            }

            if (preset.Task.FramerateMode != this.Task.FramerateMode)
            {
                return false;
            }

            if (preset.Task.VideoEncodeRateType != this.Task.VideoEncodeRateType)
            {
                return false;
            }

            if (preset.Task.VideoEncodeRateType == VideoEncodeRateType.AverageBitrate)
            {
                if (preset.Task.VideoBitrate != this.Task.VideoBitrate)
                {
                    return false;
                }

                if (preset.Task.TwoPass != this.Task.TwoPass)
                {
                    return false;
                }

                if (preset.Task.TurboFirstPass != this.Task.TurboFirstPass)
                {
                    return false;
                }
            }
            else
            {
                if (preset.Task.Quality != this.Task.Quality)
                {
                    return false;
                }
            }

            if (this.Task.VideoEncoder == VideoEncoder.X264 || this.Task.VideoEncoder == VideoEncoder.X264_10
                || this.Task.VideoEncoder == VideoEncoder.X265 || this.Task.VideoEncoder == VideoEncoder.X265_10
                || this.Task.VideoEncoder == VideoEncoder.X265_12 || this.Task.VideoEncoder == VideoEncoder.QuickSync
                || this.Task.VideoEncoder == VideoEncoder.QuickSyncH265 || this.Task.VideoEncoder == VideoEncoder.QuickSyncH26510b
                || this.Task.VideoEncoder == VideoEncoder.VceH264 || this.Task.VideoEncoder == VideoEncoder.VceH265
                || this.Task.VideoEncoder == VideoEncoder.NvencH264 || this.Task.VideoEncoder == VideoEncoder.NvencH265 || this.Task.VideoEncoder == VideoEncoder.NvencH26510b
                || this.Task.VideoEncoder == VideoEncoder.MFH264 || this.Task.VideoEncoder == VideoEncoder.MFH265)
            {
                if (!Equals(preset.Task.VideoPreset, this.Task.VideoPreset))
                {
                    return false;
                }

                foreach (VideoTune taskVideoTune in preset.Task.VideoTunes)
                {
                    if (!this.Task.VideoTunes.Contains(taskVideoTune))
                    {
                        return false;
                    }
                }

                foreach (VideoTune tune in preset.Task.VideoTunes)
                {
                    if (!this.Task.VideoTunes.Contains(tune))
                    {
                        return false;
                    }
                }

                if (preset.Task.VideoTunes.Count != this.Task.VideoTunes.Count)
                {
                    return false;
                }

                if (!Equals(preset.Task.VideoProfile, this.Task.VideoProfile))
                {
                    return false;
                }

                if (!Equals(preset.Task.VideoLevel, this.Task.VideoLevel))
                {
                    return false;
                }
            }

            if (!Equals(preset.Task.ExtraAdvancedArguments, this.Task.ExtraAdvancedArguments))
            {
                return false;
            }

            return true;
        }

        public void RefreshTask()
        {
            this.NotifyOfPropertyChange(() => this.Task);

            VideoEncoder[] allowableWebmEncoders = { VideoEncoder.VP8, VideoEncoder.VP9 };

            if ((Task.OutputFormat == OutputFormat.Mp4) && (this.SelectedVideoEncoder == VideoEncoder.Theora || allowableWebmEncoders.Contains(this.SelectedVideoEncoder)))
            {
                this.SelectedVideoEncoder = VideoEncoder.X264;
            }

            if ((Task.OutputFormat == OutputFormat.WebM) && !allowableWebmEncoders.Contains(this.SelectedVideoEncoder))
            {
                this.SelectedVideoEncoder = VideoEncoder.VP8;
            }
        }

        public void CopyQuery()
        {
            Clipboard.SetDataObject(this.SelectedVideoEncoder == VideoEncoder.X264 || this.SelectedVideoEncoder == VideoEncoder.X264_10 ? this.GetActualx264Query() : this.ExtraArguments);
        }

        protected virtual void OnTabStatusChanged(TabStatusEventArgs e)
        {
            this.TabStatusChanged?.Invoke(this, e);
        }

        private void SetQualitySliderBounds()
        {
            // Note Updating bounds to the same values won't trigger an update.
            // The properties are smart enough to not take in equal values.
            switch (this.SelectedVideoEncoder)
            {
                case VideoEncoder.FFMpeg:
                case VideoEncoder.FFMpeg2:
                    this.QualityMin = 1;
                    this.QualityMax = 31;
                    break;
                case VideoEncoder.QuickSync:
                case VideoEncoder.QuickSyncH265:
                    this.QualityMin = 0;
                    this.QualityMax = 51;
                    break;
                case VideoEncoder.QuickSyncH26510b:
                    this.QualityMin = 0;
                    this.QualityMax = 63;
                    break;
                case VideoEncoder.X264:
                case VideoEncoder.X264_10:
                case VideoEncoder.X265:
                case VideoEncoder.X265_10:
                case VideoEncoder.X265_12:
                case VideoEncoder.VceH264:
                case VideoEncoder.VceH265:
                case VideoEncoder.NvencH264:
                case VideoEncoder.NvencH265:
                case VideoEncoder.NvencH26510b:
                case VideoEncoder.MFH264:
                case VideoEncoder.MFH265:
                    this.QualityMin = 0;
                    this.QualityMax = (int)(51 / userSettingService.GetUserSetting<double>(UserSettingConstants.X264Step));
                    break;
                case VideoEncoder.Theora:
                case VideoEncoder.VP8:
                case VideoEncoder.VP9:
                    this.QualityMin = 0;
                    this.QualityMax = 63;
                    break;
            }
        }

        private string GetActualx264Query()
        {
            VideoEncoder encoder = this.SelectedVideoEncoder;
            if (encoder != VideoEncoder.X264 && encoder != VideoEncoder.X264_10)
            {
                return string.Empty;
            }

            HBVideoEncoder hbEncoder = HandBrakeEncoderHelpers.VideoEncoders.FirstOrDefault(s => s.ShortName == EnumHelper<VideoEncoder>.GetShortName(encoder));
            if (hbEncoder == null || !hbEncoder.Presets.Contains(this.VideoPreset?.ShortName))
            {
                return string.Empty;
            }

            string preset = this.VideoPreset != null ? this.VideoPreset.ShortName : string.Empty;
            string profile = this.VideoProfile != null ? this.VideoProfile.ShortName : string.Empty;

            List<string> tunes = new List<string>();
            if (this.VideoTune != null && this.VideoTune.ShortName != "none")
            {
                tunes.Add(this.VideoTune.ShortName);
            }
            if (this.FastDecode)
            {
                tunes.Add("fastdecode");
            }

            // Get the width or height, default if we don't have it yet so we don't crash.
            int width = this.Task.Width.HasValue ? this.Task.Width.Value : 720;
            int height = this.Task.Height.HasValue ? this.Task.Height.Value : 576;

            if (height == 0)
            {
                height = 576;
            }

            if (width == 0)
            {
                width = 720;
            }

            try
            {
                return HandBrakeUtils.CreateX264OptionsString(
                    preset,
                    tunes,
                    this.ExtraArguments,
                    profile,
                    this.VideoLevel != null ? this.VideoLevel.ShortName : string.Empty,
                    width,
                    height);
            }
            catch (Exception)
            {
                return "Error: Libhb not loaded.";
            }
        }

        private void UserSettingServiceSettingChanged(object sender, SettingChangedEventArgs e)
        {
            if (e.Key == UserSettingConstants.EnableVceEncoder || e.Key == UserSettingConstants.EnableNvencEncoder || e.Key == UserSettingConstants.EnableQuickSyncEncoding)
            {
                this.NotifyOfPropertyChange(() => this.VideoEncoders);
            }
        }

        private void SetRF(double? quality)
        {
            VideoQualityLimits limits = HandBrakeEncoderHelpers.GetVideoQualityLimits(EnumHelper<VideoEncoder>.GetShortName(this.SelectedVideoEncoder));
            double cqStep = 1;
            if (limits != null && limits.Granularity != 1)
            {
                cqStep = this.userSettingService.GetUserSetting<double>(UserSettingConstants.X264Step);
            }

            double rfValue = 0;

            switch (this.SelectedVideoEncoder)
            {
                case VideoEncoder.FFMpeg:
                case VideoEncoder.FFMpeg2:
                    if (quality.HasValue)
                    {
                        int cq;
                        int.TryParse(quality.Value.ToString(CultureInfo.InvariantCulture), out cq);
                        this.RF = 32 - cq;
                    }
                    break;
                case VideoEncoder.VP8:
                case VideoEncoder.VP9:
                    if (quality.HasValue)
                    {
                        int cq;
                        int.TryParse(quality.Value.ToString(CultureInfo.InvariantCulture), out cq);
                        this.RF = 63 - cq;
                    }

                    break;
                case VideoEncoder.X265:
                case VideoEncoder.X265_10:
                case VideoEncoder.X265_12:
                case VideoEncoder.X264:
                case VideoEncoder.X264_10:
                case VideoEncoder.QuickSync:
                case VideoEncoder.QuickSyncH265:
                case VideoEncoder.QuickSyncH26510b:
                case VideoEncoder.VceH264:
                case VideoEncoder.VceH265:
                case VideoEncoder.NvencH264:
                case VideoEncoder.NvencH265:
                case VideoEncoder.NvencH26510b:
                case VideoEncoder.MFH264:
                case VideoEncoder.MFH265:
                    double multiplier = 1.0 / cqStep;
                    if (quality.HasValue)
                    {
                        rfValue = quality.Value * multiplier;
                    }

                    this.RF = this.QualityMax - (int)Math.Round(rfValue, 0);

                    break;

                case VideoEncoder.Theora:

                    if (quality.HasValue)
                    {
                        this.RF = (int)quality.Value;
                    }

                    break;
            }
        }

        private void HandleEncoderChange(VideoEncoder selectedEncoder)
        {
            HBVideoEncoder encoder = HandBrakeEncoderHelpers.VideoEncoders.FirstOrDefault(s => s.ShortName == EnumHelper<VideoEncoder>.GetShortName(selectedEncoder));
            if (encoder != null)
            {
                // Setup Profile
                this.VideoProfiles.Clear();
                if (encoder.Profiles != null)
                {
                    foreach (var item in encoder.Profiles)
                    {
                        this.VideoProfiles.Add(new VideoProfile(item, item));
                    }
                    this.VideoProfile = this.VideoProfiles.FirstOrDefault();
                }
                else
                {
                    this.VideoProfile = null;
                }

                // Setup Tune
                this.VideoTunes.Clear();
                if (encoder.Tunes != null)
                {
                    this.VideoTunes.Add(VideoTune.None);
                    foreach (var item in encoder.Tunes)
                    {
                        if (item == VideoTune.FastDecode.ShortName && (selectedEncoder == VideoEncoder.X264 || selectedEncoder == VideoEncoder.X264_10))
                        {
                            continue;
                        }

                        this.VideoTunes.Add(new VideoTune(item, item));
                    }
                    this.FastDecode = false;
                    this.VideoTune = VideoTune.None;
                }
                else
                {
                    this.FastDecode = false;
                    this.VideoTune = VideoTune.None;
                }

                // Setup Levels
                this.VideoLevels.Clear();
                if (encoder.Levels != null)
                {
                    foreach (var item in encoder.Levels)
                    {
                        this.VideoLevels.Add(new VideoLevel(item, item));
                    }

                    this.VideoLevel = this.VideoLevels.FirstOrDefault();
                }
                else
                {
                    this.VideoLevel = VideoLevel.Auto;
                }

                // Setup Presets.
                this.VideoPresets.Clear();
                if (encoder.Presets != null)
                {
                    foreach (var item in encoder.Presets)
                    {
                        this.VideoPresets.Add(new VideoPreset(item, item));
                    }

                    this.VideoPresetMaxValue = encoder.Presets.Count - 1;
                    

                    this.VideoPresetValue = GetDefaultEncoderPreset(selectedEncoder);
                }
                else
                {
                    this.VideoPreset = null;
                }
            }

            // Update the Quality Slider. Make sure the bounds are up to date with the users settings.
            this.SetQualitySliderBounds();

            // Update control display
            this.DisplayOptimiseOptions = encoder?.Presets?.Count > 0;

            this.DisplayTurboFirstPass = selectedEncoder == VideoEncoder.X264 || selectedEncoder == VideoEncoder.X264_10 ||
                                         selectedEncoder == VideoEncoder.X265 || selectedEncoder == VideoEncoder.X265_10 || selectedEncoder == VideoEncoder.X265_12;

            this.DisplayTuneControls = encoder?.Tunes?.Count > 0;

            this.DisplayLevelControl = encoder?.Levels?.Count > 0;

            this.DisplayFastDecode = this.SelectedVideoEncoder == VideoEncoder.X264 || this.SelectedVideoEncoder == VideoEncoder.X264_10;
            this.NotifyOfPropertyChange(() => this.DisplayFastDecode);

            if (!this.DisplayFastDecode)
            {
                this.FastDecode = false;
            }

            this.DisplayProfileControl = encoder?.Profiles?.Count > 0;

            // Refresh Display
            this.NotifyOfPropertyChange(() => this.Rfqp);
            this.NotifyOfPropertyChange(() => this.HighQualityLabel);
            this.NotifyOfPropertyChange(() => this.IsTwoPassEnabled);
            this.NotifyOfPropertyChange(() => this.DisplayTwoPass);

            if (!HandBrakeEncoderHelpers.VideoEncoderSupportsTwoPass(EnumHelper<VideoEncoder>.GetShortName(this.SelectedVideoEncoder)))
            {
                this.TwoPass = false;
                this.TurboFirstPass = false;
            }

            // Cleanup Extra Arguments
            // Load the cached arguments. Saves the user from resetting when switching encoders.
            string result;
            this.ExtraArguments = this.encoderOptions.TryGetValue(EnumHelper<VideoEncoder>.GetShortName(selectedEncoder), out result) ? result : string.Empty;
        }

        private void HandleRFChange()
        {
            double displayRF = this.DisplayRF;
            if (displayRF > this.QualityMax || displayRF < this.QualityMin)
            {
                displayRF = this.qualityMax / 2;
            }

            this.SetRF(displayRF);
        }

        private int GetDefaultEncoderPreset(VideoEncoder selectedEncoder)
        {
            int defaultPreset = (int)Math.Round((decimal)(this.VideoPresetMaxValue / 2), 0);

            // Override for NVEnc
            if (selectedEncoder == VideoEncoder.NvencH264 || selectedEncoder == VideoEncoder.NvencH265 || selectedEncoder == VideoEncoder.NvencH26510b)
            {
                defaultPreset = this.VideoPresets.IndexOf(this.VideoPresets.FirstOrDefault(s => s.ShortName == "medium"));
            }

            // Override for QuickSync
            if (selectedEncoder == VideoEncoder.QuickSyncH265 || selectedEncoder == VideoEncoder.QuickSyncH26510b)
            {
                if (HandBrakeHardwareEncoderHelper.QsvHardwareGeneration > 6) 
                {
                    defaultPreset = this.VideoPresets.IndexOf(this.VideoPresets.FirstOrDefault(s => s.ShortName == "speed")); // TGL
                }
                else
                {
                    defaultPreset = this.VideoPresets.IndexOf(this.VideoPresets.FirstOrDefault(s => s.ShortName == "balanced")); // ICL and Earlier.
                }
            }

            if (selectedEncoder == VideoEncoder.QuickSync || selectedEncoder == VideoEncoder.VceH264 || selectedEncoder == VideoEncoder.VceH265)
            {
                defaultPreset = this.VideoPresets.IndexOf(this.VideoPresets.FirstOrDefault(s => s.ShortName == "balanced")); 
            }
            
            return defaultPreset;
        }
    }
}