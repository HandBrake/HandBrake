// --------------------------------------------------------------------------------------------------------------------
// <copyright file="VideoViewModel.cs" company="HandBrake Project (https://handbrake.fr)">
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
    using HandBrakeWPF.ViewModels.Interfaces;

    using Clipboard = System.Windows.Clipboard;
    using EncodeTask = Services.Encode.Model.EncodeTask;
    using FramerateMode = Services.Encode.Model.Models.FramerateMode;
    using OutputFormat = Services.Encode.Model.Models.OutputFormat;
    using SettingChangedEventArgs = EventArgs.SettingChangedEventArgs;
    using VideoEncodeRateType = Model.Video.VideoEncodeRateType;
    using VideoLevel = Services.Encode.Model.Models.Video.VideoLevel;
    using VideoPreset = Services.Encode.Model.Models.Video.VideoPreset;
    using VideoProfile = Services.Encode.Model.Models.Video.VideoProfile;
    using VideoTune = Services.Encode.Model.Models.Video.VideoTune;

    public class VideoViewModel : ViewModelBase, IVideoViewModel
    {
        private const string SameAsSource = "Same as source";
        private readonly IUserSettingService userSettingService;

        private bool displayOptimiseOptions;
        private int qualityMax;
        private int qualityMin;
        private bool showPeakFramerate;
        private int rf;
        private bool displayTurboAnalysisPass;
        private int videoPresetMaxValue;
        private int videoPresetValue;
        private VideoTune videoTune;
        private bool displayTuneControls;
        private bool displayLevelControl;
        private bool displayProfileControl;
        private Dictionary<string, string> encoderOptions = new Dictionary<string, string>();

        public VideoViewModel(IUserSettingService userSettingService, IErrorService errorService)
        {
            this.Task = new EncodeTask { VideoEncoder = HandBrakeEncoderHelpers.VideoEncoders.First(s => s.IsX264) };
            this.userSettingService = userSettingService;
            this.QualityMin = 0;
            this.QualityMax = 51;
            this.IsConstantQuantity = true;
            this.VideoEncoders = new BindingList<HBVideoEncoder>(HandBrakeEncoderHelpers.VideoEncoders.ToList());

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
                List<string> framerates = new List<string> { SameAsSource };
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
                    this.VideoBitrate = null;
                    this.NotifyOfPropertyChange(() => this.Task);
                }
                else
                {
                    this.Task.VideoEncodeRateType = VideoEncodeRateType.AverageBitrate;
                }

                this.NotifyOfPropertyChange(() => this.IsConstantQuantity);
                this.NotifyOfPropertyChange(() => this.IsMultiPassEnabled);
                this.OnTabStatusChanged(null);
            }
        }

        public bool IsMultiPassEnabled
        {
            get
            {
                return this.SelectedVideoEncoder.SupportsMultiPass(this.IsConstantQuantity);
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
                this.Task.Quality = CalculateQualityValue(value);

                this.NotifyOfPropertyChange(() => this.RF);
                this.NotifyOfPropertyChange(() => this.DisplayRF);
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

        public bool MultiPass
        {
            get => this.Task.MultiPass;

            set
            {
                this.Task.MultiPass = value;
                this.NotifyOfPropertyChange(() => this.MultiPass);
                this.OnTabStatusChanged(null);
            }
        }

        public bool TurboAnalysisPass
        {
            get => this.Task.TurboAnalysisPass;

            set
            {
                this.Task.TurboAnalysisPass = value;
                this.NotifyOfPropertyChange(() => this.TurboAnalysisPass);
                this.OnTabStatusChanged(null);
            }
        }

        public string Rfqp
        {
            get => HandBrakeEncoderHelpers.GetVideoQualityRateControlName(this.SelectedVideoEncoder?.ShortName);
        }

        public string HighQualityLabel
        {
            get => this.SelectedVideoEncoder.IsX264 ? Resources.Video_PlaceboQuality : Resources.Video_HigherQuality;
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

        public HBVideoEncoder SelectedVideoEncoder
        {
            get => this.Task.VideoEncoder;
            set
            {
                if (!object.Equals(value, this.Task.VideoEncoder))
                {
                    // Cache the current extra args. We can set them back later if the user switches back
                    if (this.Task.VideoEncoder != null)
                    {
                        this.encoderOptions[this.Task.VideoEncoder.ShortName] = this.ExtraArguments;
                    }

                    this.Task.VideoEncoder = value;
                    this.NotifyOfPropertyChange(() => this.SelectedVideoEncoder);
                    this.HandleEncoderChange(this.Task.VideoEncoder);
                    this.HandleRFChange();
                    this.OnTabStatusChanged(null);

                    this.OnTabStatusChanged(new TabStatusEventArgs("filters", ChangedOption.Encoder));
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

        public IEnumerable<HBVideoEncoder> VideoEncoders { get; set; }

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

        public bool DisplayMultiPass => this.SelectedVideoEncoder.SupportsMultiPass();

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

                if (this.SelectedVideoEncoder != null && this.SelectedVideoEncoder.Presets != null)
                {
                    string preset = value >= 0 ? this.SelectedVideoEncoder.Presets[value] : null;
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

                if ((this.SelectedVideoEncoder.IsX264 || this.SelectedVideoEncoder.IsSVTAV1) && hasFastDecode)
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
            get => this.SelectedVideoEncoder != null && this.SelectedVideoEncoder.IsX264 ? string.Format(Resources.Video_EncoderExtraArgs, this.GetActualx264Query()) : Resources.Video_EncoderExtraArgsTooltip;
        }

        public bool DisplayTurboAnalysisPass
        {
            get => this.displayTurboAnalysisPass;
            set
            {
                if (value.Equals(this.displayTurboAnalysisPass))
                {
                    return;
                }

                this.displayTurboAnalysisPass = value;
                this.NotifyOfPropertyChange(() => this.DisplayTurboAnalysisPass);
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

            this.MultiPass = preset.Task.MultiPass;
            this.TurboAnalysisPass = preset.Task.TurboAnalysisPass;

            this.VideoBitrate = preset.Task.VideoEncodeRateType == VideoEncodeRateType.AverageBitrate ? preset.Task.VideoBitrate : null;

            this.NotifyOfPropertyChange(() => this.Task);

            this.HandleEncoderChange(preset.Task.VideoEncoder);
            this.SetQualitySliderBounds();
            this.SetRF(preset.Task.Quality);

            HBVideoEncoder encoder = preset.Task.VideoEncoder;
            if (encoder != null)
            {
                this.VideoLevel = encoder.Levels?.Count > 0
                    ? preset.Task.VideoLevel != null ? preset.Task.VideoLevel.Clone() :
                    this.VideoLevels.FirstOrDefault()
                    : null;

                this.VideoProfile = encoder.Profiles?.Count > 0
                    ? preset.Task.VideoProfile != null ? preset.Task.VideoProfile.Clone() :
                    this.VideoProfiles.FirstOrDefault()
                    : null;

                this.VideoPresetValue = encoder.Presets?.Count > 0
                    ? preset.Task.VideoPreset != null ? this.VideoPresets.IndexOf(preset.Task.VideoPreset) : 0
                    : 0;

                if (preset.Task.VideoEncoder.IsX265)
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
            this.NotifyOfPropertyChange(() => this.VideoBitrate);
            this.NotifyOfPropertyChange(() => this.Task.Quality);
            this.NotifyOfPropertyChange(() => this.Task.MultiPass);
            this.NotifyOfPropertyChange(() => this.Task.TurboAnalysisPass);
            this.NotifyOfPropertyChange(() => this.VideoTune);
            this.NotifyOfPropertyChange(() => this.VideoProfile);
            this.NotifyOfPropertyChange(() => this.VideoPreset);
            this.NotifyOfPropertyChange(() => this.VideoLevel);
            this.NotifyOfPropertyChange(() => this.FastDecode);
            this.NotifyOfPropertyChange(() => this.ExtraArguments);

            this.VideoTune = (task.VideoTunes != null && task.VideoTunes.Any() ? task.VideoTunes.FirstOrDefault(t => !Equals(t, VideoTune.FastDecode)) : this.VideoTunes.FirstOrDefault())
                             ?? VideoTune.None;

            if (this.SelectedVideoEncoder != null && this.VideoPreset != null)
            {
                int index = this.SelectedVideoEncoder.Presets.IndexOf(this.VideoPreset.ShortName);
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

                if (preset.Task.MultiPass != this.Task.MultiPass)
                {
                    return false;
                }

                if (preset.Task.TurboAnalysisPass != this.Task.TurboAnalysisPass)
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

            if (this.SelectedVideoEncoder != null)
            {
                if (this.SelectedVideoEncoder.Presets != null && this.SelectedVideoEncoder.Presets.Any())
                {
                    if (!Equals(preset.Task.VideoPreset, this.Task.VideoPreset))
                    {
                        return false;
                    }
                }

                if (this.SelectedVideoEncoder.Tunes != null && this.SelectedVideoEncoder.Tunes.Any())
                {
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
                }

                if (this.SelectedVideoEncoder.Profiles != null && this.SelectedVideoEncoder.Profiles.Any())
                {
                    if (!Equals(preset.Task.VideoProfile, this.Task.VideoProfile))
                    {
                        return false;
                    }
                }

                if (this.SelectedVideoEncoder.Levels != null && this.SelectedVideoEncoder.Levels.Any())
                {
                    if (!Equals(preset.Task.VideoLevel, this.Task.VideoLevel))
                    {
                        return false;
                    }
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
            if (this.SelectedVideoEncoder != null && this.Task.OutputFormat == OutputFormat.Mp4 && !this.SelectedVideoEncoder.SupportsMP4)
            {
                this.SelectedVideoEncoder = HandBrakeEncoderHelpers.VideoEncoders.First(s => s.IsX264);
            }

            if (this.SelectedVideoEncoder != null && this.Task.OutputFormat == OutputFormat.WebM && !this.SelectedVideoEncoder.SupportsWebM)
            {
                this.SelectedVideoEncoder = HandBrakeEncoderHelpers.VideoEncoders.First(s => s.IsVP9);
            }

            this.NotifyOfPropertyChange(() => this.Task);
        }

        public void CopyQuery()
        {
            Clipboard.SetDataObject(this.SelectedVideoEncoder.IsX264 ? this.GetActualx264Query() : this.ExtraArguments);
        }

        protected virtual void OnTabStatusChanged(TabStatusEventArgs e)
        {
            this.TabStatusChanged?.Invoke(this, e);
        }

        private string GetActualx264Query()
        {
            if (!this.SelectedVideoEncoder.IsX264)
            {
                return string.Empty;
            }

            if (this.SelectedVideoEncoder == null || !this.SelectedVideoEncoder.Presets.Contains(this.VideoPreset?.ShortName))
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
            if (!quality.HasValue)
            {
                return;
            }

            VideoQualityLimits limits = HandBrakeEncoderHelpers.GetVideoQualityLimits(this.SelectedVideoEncoder?.ShortName);
            if (limits == null)
            {
                return;
            }

            double cqStep = 1;
            if (limits.Granularity != 1)
            {
                cqStep = this.userSettingService.GetUserSetting<double>(UserSettingConstants.X264Step);
                cqStep = 1 / cqStep; // Inverse 
            }

            if (limits.Ascending)
            {
                this.RF = (int)quality.Value; // Theora
            }
            else
            {
                if (limits.Low == 0)
                {
                    this.RF = (int)(limits.High * cqStep) - (int)(quality * cqStep);
                }
                else // Supporting negative ranges
                {
                    float augment = limits.Low > 0 ? 0 : (limits.Low * -1);
                    this.RF = (int)(limits.High * cqStep) - ((int)(quality * cqStep) - (int)(limits.Low * cqStep));
                }
            }
        }

        private double? CalculateQualityValue(int sliderValue)
        {
            VideoQualityLimits limits = HandBrakeEncoderHelpers.GetVideoQualityLimits(this.SelectedVideoEncoder.ShortName);
            if (limits == null)
            {
                return null;
            }

            double cqStep = 1;
            if (limits.Granularity != 1)
            {
                cqStep = this.userSettingService.GetUserSetting<double>(UserSettingConstants.X264Step);
            }

            if (limits.Ascending) // Theora
            {
                return sliderValue;
            }
            else // x264, x265, MPEG-4, MPEG-2, AV1, QuickSync
            {
                if (limits.Low > 0)
                {
                    sliderValue -= (int)limits.Low; // Handles the non 0 Starting point. MPEG-4, MPEG-2
                }

                float augment = limits.Low > 0 ? 0 : (limits.Low * -1); // Handle negative ranges

                if (cqStep != 1)
                {
                    return Math.Round(limits.High - (sliderValue * cqStep) - augment, 2);
                }
                else 
                {
                    return limits.High - sliderValue - augment;
                }
            }
        }

        private void SetQualitySliderBounds()
        {
            if (this.SelectedVideoEncoder == null)
            {
                return;
            }

            VideoQualityLimits limits = HandBrakeEncoderHelpers.GetVideoQualityLimits(this.SelectedVideoEncoder.ShortName);
            if (limits == null)
            {
                return;
            }

            double cqStep = 1;
            if (limits.Granularity != 1)
            {
                cqStep = this.userSettingService.GetUserSetting<double>(UserSettingConstants.X264Step);
            }

            if (cqStep != 1)
            {
                this.QualityMin = (int)Math.Round(limits.Low / cqStep, 0);
                this.QualityMax = (int)Math.Round(limits.High / cqStep, 0);
            }
            else
            {
                this.QualityMin = (int)limits.Low;
                this.QualityMax = (int)limits.High;
            }
        }

        private void HandleEncoderChange(HBVideoEncoder selectedEncoder)
        {
            HBVideoEncoder encoder = selectedEncoder;
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
                        if (item == VideoTune.FastDecode.ShortName &&
                            (this.SelectedVideoEncoder.IsX264 || this.SelectedVideoEncoder.IsSVTAV1))
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

            this.DisplayTurboAnalysisPass = this.SelectedVideoEncoder.IsX264 || this.SelectedVideoEncoder.IsX265;

            this.DisplayTuneControls = encoder?.Tunes?.Count > 0;

            this.DisplayLevelControl = encoder?.Levels?.Count > 0;

            this.DisplayFastDecode = this.SelectedVideoEncoder.IsX264 || this.SelectedVideoEncoder.IsSVTAV1;
            this.NotifyOfPropertyChange(() => this.DisplayFastDecode);

            if (!this.DisplayFastDecode)
            {
                this.FastDecode = false;
            }

            this.DisplayProfileControl = encoder?.Profiles?.Count > 0;

            // Refresh Display
            this.NotifyOfPropertyChange(() => this.Rfqp);
            this.NotifyOfPropertyChange(() => this.HighQualityLabel);
            this.NotifyOfPropertyChange(() => this.IsMultiPassEnabled);
            this.NotifyOfPropertyChange(() => this.DisplayMultiPass);

            if (this.SelectedVideoEncoder != null && !this.SelectedVideoEncoder.SupportsMultiPass())
            {
                this.MultiPass = false;
                this.TurboAnalysisPass = false;
            }

            // Cleanup Extra Arguments
            // Load the cached arguments. Saves the user from resetting when switching encoders.
            string result;
            this.ExtraArguments = this.encoderOptions.TryGetValue(selectedEncoder?.ShortName, out result) ? result : string.Empty;
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

        private int GetDefaultEncoderPreset(HBVideoEncoder selectedEncoder)
        {
            int defaultPreset = (int)Math.Round((decimal)(this.VideoPresetMaxValue / 2), 0);

            // Override for NVEnc
            if (selectedEncoder.IsNVEnc)
            {
                defaultPreset = this.VideoPresets.IndexOf(this.VideoPresets.FirstOrDefault(s => s.ShortName == "medium"));
            }

            // Override for QuickSync
            if (selectedEncoder.IsQuickSyncH265)
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

            if (selectedEncoder.IsQuickSyncAV1)
            {
                defaultPreset = this.VideoPresets.IndexOf(this.VideoPresets.FirstOrDefault(s => s.ShortName == "speed")); // Alchemist and later
            }

            if (selectedEncoder.IsQuickSync || selectedEncoder.IsVCN)
            {
                defaultPreset = this.VideoPresets.IndexOf(this.VideoPresets.FirstOrDefault(s => s.ShortName == "balanced")); 
            }
            
            return defaultPreset;
        }
    }
}