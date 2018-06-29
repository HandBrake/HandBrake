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

    using Caliburn.Micro;

    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.Model.Encoding;

    using HandBrakeWPF.EventArgs;
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.Services.Scan.Model;
    using HandBrakeWPF.Utilities;
    using HandBrakeWPF.ViewModels.Interfaces;

    using Clipboard = System.Windows.Clipboard;
    using EncodeTask = HandBrakeWPF.Services.Encode.Model.EncodeTask;
    using FramerateMode = HandBrakeWPF.Services.Encode.Model.Models.FramerateMode;
    using OutputFormat = HandBrakeWPF.Services.Encode.Model.Models.OutputFormat;
    using SettingChangedEventArgs = HandBrakeWPF.EventArgs.SettingChangedEventArgs;
    using VideoLevel = HandBrakeWPF.Services.Encode.Model.Models.Video.VideoLevel;
    using VideoPreset = HandBrakeWPF.Services.Encode.Model.Models.Video.VideoPreset;
    using VideoProfile = HandBrakeWPF.Services.Encode.Model.Models.Video.VideoProfile;
    using VideoTune = HandBrakeWPF.Services.Encode.Model.Models.Video.VideoTune;

    /// <summary>
    /// The Video View Model
    /// </summary>
    public class VideoViewModel : ViewModelBase, IVideoViewModel
    {
        /*
         * Hard Code "None" in the Models for Tune.
         * Test Everything         */

        #region Constants and Fields

        private const string SameAsSource = "Same as source";
        private readonly IUserSettingService userSettingService;

        private bool displayOptimiseOptions;
        private int qualityMax;
        private int qualityMin;
        private bool showPeakFramerate;
        private int rf;
        private bool displayTurboFirstPass;
        private int videoPresetMaxValue;
        private int videoPresetValue;
        private bool displayNonQsvControls;
        private VideoTune videoTune;
        private bool fastDecode;
        private bool displayTuneControls;
        private bool displayLevelControl;
        private bool displayProfileControl;
        private Dictionary<string, string> encoderOptions = new Dictionary<string, string>();

        #endregion

        #region Constructors and Destructors

        /// <summary>
        /// Initializes a new instance of the <see cref="VideoViewModel"/> class.
        /// </summary>
        /// <param name="userSettingService">
        /// The user Setting Service.
        /// </param>
        public VideoViewModel(IUserSettingService userSettingService)
        {
            this.Task = new EncodeTask { VideoEncoder = VideoEncoder.X264 };
            this.userSettingService = userSettingService;
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

        #endregion

        public event EventHandler<TabStatusEventArgs> TabStatusChanged;

        #region Public Properties

        public IUserSettingService UserSettingService => this.userSettingService;

        /// <summary>
        /// Gets or sets the current Encode Task.
        /// </summary>
        public EncodeTask Task { get; set; }

        /// <summary>
        /// Gets a value indicating whether show advanced tab.
        /// </summary>
        public bool IsAdvancedTabOptionEnabled
        {
            get
            {
                bool showAdvTabSetting = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.ShowAdvancedTab);
                if (!showAdvTabSetting)
                {
                    this.UseAdvancedTab = false;
                }

                if (this.SelectedVideoEncoder != VideoEncoder.X264 && this.SelectedVideoEncoder != VideoEncoder.X264_10)
                {
                    this.UseAdvancedTab = false;
                    return false;
                }

                return showAdvTabSetting;
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether use video tab.
        /// </summary>
        public bool UseAdvancedTab
        {
            get
            {
                return this.Task.ShowAdvancedTab;
            }
            set
            {
                if (!Equals(value, this.Task.ShowAdvancedTab))
                {
                    // Set the Advanced Tab up with the current settings, if we can.
                    this.Task.AdvancedEncoderOptions = value ? this.GetActualx264Query() : string.Empty;
                    this.Task.ShowAdvancedTab = value;

                    this.NotifyOfPropertyChange(() => this.UseAdvancedTab);
                }
            }
        }

        /// <summary>
        /// Gets Framerates.
        /// </summary>
        public IEnumerable<string> Framerates
        {
            get
            {
                List<string> framerates = new List<string> { "Same as source" };
                framerates.AddRange(HandBrakeEncoderHelpers.VideoFramerates.Select(item => item.Name));
                return framerates;
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether IsConstantFramerate.
        /// </summary>
        public bool IsConstantFramerate
        {
            get
            {
                return this.Task.FramerateMode == FramerateMode.CFR;
            }
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

        /// <summary>
        /// Gets or sets a value indicating whether IsConstantQuantity.
        /// </summary>
        public bool IsConstantQuantity
        {
            get
            {
                return this.Task.VideoEncodeRateType == VideoEncodeRateType.ConstantQuality;
            }
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
                this.OnTabStatusChanged(null);
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether IsPeakFramerate.
        /// </summary>
        public bool IsPeakFramerate
        {
            get
            {
                return this.Task.FramerateMode == FramerateMode.PFR;
            }
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

        /// <summary>
        /// Gets or sets a value indicating whether IsVariableFramerate.
        /// </summary>
        public bool IsVariableFramerate
        {
            get
            {
                return this.Task.FramerateMode == FramerateMode.VFR;
            }
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

        /// <summary>
        /// Gets a value indicating whether is lossless.
        /// </summary>
        public bool IsLossless
        {
            get
            {
                return 0.0.Equals(this.DisplayRF) && (this.SelectedVideoEncoder == VideoEncoder.X264 || this.SelectedVideoEncoder == VideoEncoder.X264_10);
            }
        }

        /// <summary>
        /// Gets or sets QualityMax.
        /// </summary>
        public int QualityMax
        {
            get
            {
                return this.qualityMax;
            }
            set
            {
                if (!qualityMax.Equals(value))
                {
                    this.qualityMax = value;
                    this.NotifyOfPropertyChange(() => this.QualityMax);
                }
            }
        }

        /// <summary>
        /// Gets or sets QualityMin.
        /// </summary>
        public int QualityMin
        {
            get
            {
                return this.qualityMin;
            }
            set
            {
                if (!qualityMin.Equals(value))
                {
                    this.qualityMin = value;
                    this.NotifyOfPropertyChange(() => this.QualityMin);
                }
            }
        }

        /// <summary>
        /// Gets or sets RF.
        /// </summary>
        public int RF
        {
            get
            {
                return rf;
            }
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
                        double cqStep = userSettingService.GetUserSetting<double>(UserSettingConstants.X264Step);
                        double rfValue = 51.0 - (value * cqStep);
                        rfValue = Math.Round(rfValue, 2);
                        this.Task.Quality = rfValue;
                        break;
                    case VideoEncoder.QuickSync:
                    case VideoEncoder.QuickSyncH265:
                    case VideoEncoder.VceH264:
                    case VideoEncoder.VceH265:
                    case VideoEncoder.NvencH264:
                    case VideoEncoder.NvencH265:
                        rfValue = 51.0 - value;
                        rfValue = Math.Round(rfValue, 0);
                        this.Task.Quality = rfValue;
                        break;
                    case VideoEncoder.QuickSyncH26510b:
                        rfValue = 63.0 - (value - 0);
                        rfValue = Math.Round(rfValue, 0);
                        this.Task.Quality = rfValue;
                        break;
                    case VideoEncoder.Theora:
                        Task.Quality = value;
                        break;
                }

                this.NotifyOfPropertyChange(() => this.RF);
                this.NotifyOfPropertyChange(() => this.DisplayRF);
                this.NotifyOfPropertyChange(() => this.IsLossless);
                this.OnTabStatusChanged(null);
            }
        }

        /// <summary>
        /// Gets or sets the Video Bitrate.
        /// </summary>
        public int? VideoBitrate
        {
            get
            {
                return this.Task.VideoBitrate;
            }
            set
            {
                if (value == this.Task.VideoBitrate)
                {
                    return;
                }
                this.Task.VideoBitrate = value;
                this.NotifyOfPropertyChange(() => this.VideoBitrate);
                this.OnTabStatusChanged(null);
            }
        }

        /// <summary>
        /// Gets DisplayRF.
        /// </summary>
        public double DisplayRF
        {
            get
            {
                return Task.Quality.HasValue ? this.Task.Quality.Value : 0;
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether two pass.
        /// </summary>
        public bool TwoPass
        {
            get
            {
                return this.Task.TwoPass;
            }

            set
            {
                this.Task.TwoPass = value;
                this.NotifyOfPropertyChange(() => this.TwoPass);
                this.OnTabStatusChanged(null);
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether turbo first pass.
        /// </summary>
        public bool TurboFirstPass
        {
            get
            {
                return this.Task.TurboFirstPass;
            }

            set
            {
                this.Task.TurboFirstPass = value;
                this.NotifyOfPropertyChange(() => this.TurboFirstPass);
                this.OnTabStatusChanged(null);
            }
        }

        /// <summary>
        /// Gets the rfqp.
        /// </summary>
        public string Rfqp
        {
            get
            {
                return this.SelectedVideoEncoder == VideoEncoder.X264 || this.SelectedVideoEncoder == VideoEncoder.X264_10 || this.SelectedVideoEncoder == VideoEncoder.X265
                       || this.SelectedVideoEncoder == VideoEncoder.X265_10 || this.SelectedVideoEncoder == VideoEncoder.X265_12 ? "RF" : "QP";
            }
        }

        /// <summary>
        /// Gets the high quality label.
        /// </summary>
        public string HighQualityLabel
        {
            get
            {
                return this.SelectedVideoEncoder == VideoEncoder.X264 || this.SelectedVideoEncoder == VideoEncoder.X264_10 ? Resources.Video_PlaceboQuality : Resources.Video_HigherQuality;
            }
        }

        /// <summary>
        /// Gets or sets SelectedFramerate.
        /// </summary>
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
                if (value == "Same as source")
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

        /// <summary>
        /// Gets or sets SelectedVideoEncoder.
        /// </summary>
        public VideoEncoder SelectedVideoEncoder
        {
            get
            {
                return this.Task.VideoEncoder;
            }
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

        /// <summary>
        /// Gets or sets a value indicating whether ShowPeakFramerate.
        /// </summary>
        public bool ShowPeakFramerate
        {
            get
            {
                return this.showPeakFramerate;
            }
            set
            {
                this.showPeakFramerate = value;
                this.NotifyOfPropertyChange(() => this.ShowPeakFramerate);
            }
        }

        /// <summary>
        /// Gets or sets VideoEncoders.
        /// </summary>
        public IEnumerable<VideoEncoder> VideoEncoders { get; set; }

        /// <summary>
        /// Gets or sets the extra arguments.
        /// </summary>
        public string ExtraArguments
        {
            get
            {
                return this.Task.ExtraAdvancedArguments;
            }
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

        /// <summary>
        /// Gets or sets a value indicating whether to display H264
        /// </summary>
        public bool DisplayOptimiseOptions
        {
            get
            {
                return this.displayOptimiseOptions;
            }

            set
            {
                this.displayOptimiseOptions = value;
                this.NotifyOfPropertyChange(() => this.DisplayOptimiseOptions);
                this.NotifyOfPropertyChange(() => FullOptionsTooltip);
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether display non qsv controls.
        /// </summary>
        public bool DisplayNonQSVControls
        {
            get
            {
                return this.displayNonQsvControls;
            }

            set
            {
                if (value.Equals(this.displayNonQsvControls))
                {
                    return;
                }
                this.displayNonQsvControls = value;
                this.NotifyOfPropertyChange(() => this.DisplayNonQSVControls);
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether display tune controls.
        /// </summary>
        public bool DisplayTuneControls
        {
            get
            {
                return this.displayTuneControls;
            }
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

        /// <summary>
        /// Gets or sets a value indicating whether display level control.
        /// </summary>
        public bool DisplayLevelControl
        {
            get
            {
                return this.displayLevelControl;
            }
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

        /// <summary>
        /// Gets or sets a value indicating whether the profile control is displayed.
        /// </summary>
        public bool DisplayProfileControl
        {
            get
            {
                return this.displayProfileControl;
            }
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

        /// <summary>
        /// Gets or sets a value indicating whether fast decode.
        /// </summary>
        public bool FastDecode
        {
            get
            {
                return this.Task.VideoTunes.Contains(VideoTune.FastDecode);
            }
            set
            {
                if (value.Equals(this.fastDecode))
                {
                    return;
                }
                this.fastDecode = value;

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

        /// <summary>
        /// Gets or sets the video preset.
        /// </summary>
        public VideoPreset VideoPreset
        {
            get
            {
                return this.Task.VideoPreset;
            }
            set
            {
                this.Task.VideoPreset = value;
                this.NotifyOfPropertyChange(() => this.VideoPreset);
                this.NotifyOfPropertyChange(() => this.FullOptionsTooltip);
                this.OnTabStatusChanged(null);
            }
        }

        /// <summary>
        /// Gets or sets the video preset value.
        /// </summary>
        public int VideoPresetValue
        {
            get
            {
                return this.videoPresetValue;
            }
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

        /// <summary>
        /// Gets or sets the video preset max value.
        /// </summary>
        public int VideoPresetMaxValue
        {
            get
            {
                return this.videoPresetMaxValue;
            }

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

        /// <summary>
        /// Gets or sets the video tune.
        /// </summary>
        public VideoTune VideoTune
        {
            get
            {
                return this.videoTune;
            }
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

                if (hasFastDecode)
                {
                    this.Task.VideoTunes.Add(VideoTune.FastDecode);
                }

                this.NotifyOfPropertyChange(() => this.VideoTune);
                this.NotifyOfPropertyChange(() => this.FullOptionsTooltip);
                this.OnTabStatusChanged(null);
            }
        }

        /// <summary>
        /// Gets or sets the video profile.
        /// </summary>
        public VideoProfile VideoProfile
        {
            get
            {
                return this.Task.VideoProfile;
            }
            set
            {
                this.Task.VideoProfile = value;
                this.NotifyOfPropertyChange(() => this.VideoProfile);
                this.NotifyOfPropertyChange(() => this.FullOptionsTooltip);
                this.OnTabStatusChanged(null);
            }
        }

        /// <summary>
        /// Gets or sets the video level.
        /// </summary>
        public VideoLevel VideoLevel
        {
            get
            {
                return this.Task.VideoLevel;
            }
            set
            {
                this.Task.VideoLevel = value;
                this.NotifyOfPropertyChange(() => this.VideoLevel);
                this.NotifyOfPropertyChange(() => this.FullOptionsTooltip);
                this.OnTabStatusChanged(null);
            }
        }

        /// <summary>
        /// Gets or sets the video presets.
        /// </summary>
        public BindingList<VideoPreset> VideoPresets { get; set; }

        /// <summary>
        /// Gets or sets the video tunes.
        /// </summary>
        public BindingList<VideoTune> VideoTunes { get; set; }

        /// <summary>
        /// Gets or sets the video profiles.
        /// </summary>
        public BindingList<VideoProfile> VideoProfiles { get; set; }

        /// <summary>
        /// Gets or sets the video levels.
        /// </summary>
        public BindingList<VideoLevel> VideoLevels { get; set; }

        /// <summary>
        /// Gets the full options tooltip.
        /// </summary>
        public string FullOptionsTooltip
        {
            get
            {
                return this.SelectedVideoEncoder == VideoEncoder.X264 || this.SelectedVideoEncoder == VideoEncoder.X264_10 ? string.Format(Resources.Video_EncoderExtraArgs, this.GetActualx264Query()) : Resources.Video_EncoderExtraArgsTooltip;
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether display turbo first pass.
        /// </summary>
        public bool DisplayTurboFirstPass
        {
            get
            {
                return this.displayTurboFirstPass;
            }
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

        #endregion

        #region Public Methods

        /// <summary>
        /// Setup this window for a new source
        /// </summary>
        /// <param name="source">
        /// The source.
        /// </param>
        /// <param name="title">
        /// The title.
        /// </param>
        /// <param name="preset">
        /// The preset.
        /// </param>
        /// <param name="task">
        /// The task.
        /// </param>
        public void SetSource(Source source, Title title, Preset preset, EncodeTask task)
        {
            this.Task = task;
        }

        /// <summary>
        /// Setup this tab for the specified preset.
        /// </summary>
        /// <param name="preset">
        /// The preset.
        /// </param>
        /// <param name="task">
        /// The task.
        /// </param>
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
                if (preset.Task.VideoEncoder == VideoEncoder.X264 || preset.Task.VideoEncoder == VideoEncoder.X264_10
                    || preset.Task.VideoEncoder == VideoEncoder.X265 || preset.Task.VideoEncoder == VideoEncoder.X265_10 || preset.Task.VideoEncoder == VideoEncoder.X265_12
                    || preset.Task.VideoEncoder == VideoEncoder.QuickSync || preset.Task.VideoEncoder == VideoEncoder.QuickSyncH265 || preset.Task.VideoEncoder == VideoEncoder.QuickSyncH26510b
                    || preset.Task.VideoEncoder == VideoEncoder.VceH264 || preset.Task.VideoEncoder == VideoEncoder.VceH265
                    || preset.Task.VideoEncoder == VideoEncoder.NvencH264 || preset.Task.VideoEncoder == VideoEncoder.NvencH265)
                {
                    this.VideoLevel = preset.Task.VideoLevel != null ? preset.Task.VideoLevel.Clone() : this.VideoLevels.FirstOrDefault();
                    this.VideoProfile = preset.Task.VideoProfile != null ? preset.Task.VideoProfile.Clone() : this.VideoProfiles.FirstOrDefault();
                    this.VideoPresetValue = preset.Task.VideoPreset != null ? this.VideoPresets.IndexOf(preset.Task.VideoPreset) : 0;
                    this.FastDecode = preset.Task.VideoTunes != null && preset.Task.VideoTunes.Contains(VideoTune.FastDecode);
                    this.VideoTune = (preset.Task.VideoTunes != null && preset.Task.VideoTunes.Any() ? preset.Task.VideoTunes.FirstOrDefault(t => !Equals(t, VideoTune.FastDecode)) : this.VideoTunes.FirstOrDefault())
                                     ?? VideoTune.None;
                }
            }

            this.ExtraArguments = preset.Task.ExtraAdvancedArguments;
            this.UseAdvancedTab = this.IsAdvancedTabOptionEnabled && preset.Task.ShowAdvancedTab;
        }

        /// <summary>
        /// Update all the UI controls based on the encode task passed in.
        /// </summary>
        /// <param name="task">
        /// The task.
        /// </param>
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
                || this.Task.VideoEncoder == VideoEncoder.NvencH264 || this.Task.VideoEncoder == VideoEncoder.NvencH265)
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

        /// <summary>
        /// Trigger a Notify Property Changed on the Task to force various UI elements to update.
        /// </summary>
        public void RefreshTask()
        {
            this.NotifyOfPropertyChange(() => this.Task);

            if ((Task.OutputFormat == OutputFormat.Mp4) && this.SelectedVideoEncoder == VideoEncoder.Theora)
            {
                this.SelectedVideoEncoder = VideoEncoder.X264;
            }
        }

        /// <summary>
        /// The copy query.
        /// </summary>
        public void CopyQuery()
        {
            Clipboard.SetDataObject(this.SelectedVideoEncoder == VideoEncoder.X264 || this.SelectedVideoEncoder == VideoEncoder.X264_10 ? this.GetActualx264Query() : this.ExtraArguments);
        }

        #endregion

        protected virtual void OnTabStatusChanged(TabStatusEventArgs e)
        {
            this.TabStatusChanged?.Invoke(this, e);
        }

        /// <summary>
        /// Set the bounds of the Constant Quality Slider
        /// </summary>
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
                case VideoEncoder.VceH264:
                case VideoEncoder.VceH265:
                case VideoEncoder.NvencH264:
                case VideoEncoder.NvencH265:
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

        /// <summary>
        /// The get actualx 264 query.
        /// </summary>
        /// <returns>
        /// The <see cref="string"/>.
        /// </returns>
        private string GetActualx264Query()
        {
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

        /// <summary>
        /// The user setting service_ setting changed.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void UserSettingServiceSettingChanged(object sender, SettingChangedEventArgs e)
        {
            if (e.Key == UserSettingConstants.ShowAdvancedTab)
            {
                this.NotifyOfPropertyChange(() => this.IsAdvancedTabOptionEnabled);
                this.NotifyOfPropertyChange(() => this.VideoEncoders);
            }
        }

        /// <summary>
        /// The set rf.
        /// </summary>
        /// <param name="quality">
        /// The quality.
        /// </param>
        private void SetRF(double? quality)
        {
            double cqStep = this.userSettingService.GetUserSetting<double>(UserSettingConstants.X264Step);
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

                    if (this.SelectedVideoEncoder == VideoEncoder.QuickSync || this.SelectedVideoEncoder == VideoEncoder.QuickSyncH265 || this.SelectedVideoEncoder == VideoEncoder.QuickSyncH26510b
                        || this.SelectedVideoEncoder == VideoEncoder.VceH264 || this.SelectedVideoEncoder == VideoEncoder.VceH265
                        || this.SelectedVideoEncoder == VideoEncoder.NvencH264 || this.SelectedVideoEncoder == VideoEncoder.NvencH265)
                    {
                        cqStep = 1;
                    }

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

        /// <summary>
        /// The handle encoder change.
        /// </summary>
        /// <param name="selectedEncoder">
        /// The selected encoder.
        /// </param>
        private void HandleEncoderChange(VideoEncoder selectedEncoder)
        {
            if (selectedEncoder != VideoEncoder.X264)
            {
                this.UseAdvancedTab = false;
            }

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
                        if (item != VideoTune.FastDecode.ShortName)
                        {
                            this.VideoTunes.Add(new VideoTune(item, item));
                        }
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
                    int middlePreset = (int)Math.Round((decimal)(this.VideoPresetMaxValue / 2), 0);
                    this.VideoPresetValue = middlePreset;
                }
                else
                {
                    this.VideoPreset = null;
                }
            }

            // Tell the Advanced Panel off the change
            IX264ViewModel advancedViewModel = IoC.Get<IX264ViewModel>();
            advancedViewModel.SetEncoder(this.Task.VideoEncoder);

            // Update the Quality Slider. Make sure the bounds are up to date with the users settings.
            this.SetQualitySliderBounds();

            // Update control display
            this.UseAdvancedTab = selectedEncoder != VideoEncoder.QuickSync && selectedEncoder != VideoEncoder.QuickSyncH265 && selectedEncoder != VideoEncoder.QuickSyncH26510b
                               && selectedEncoder != VideoEncoder.VceH264 && selectedEncoder != VideoEncoder.VceH265
                               && selectedEncoder != VideoEncoder.NvencH264 && selectedEncoder != VideoEncoder.NvencH265
                               && this.UseAdvancedTab;

            this.DisplayOptimiseOptions = this.SelectedVideoEncoder == VideoEncoder.X264 || this.SelectedVideoEncoder == VideoEncoder.X264_10 ||
                                          this.SelectedVideoEncoder == VideoEncoder.X265 || this.SelectedVideoEncoder == VideoEncoder.X265_10 || this.SelectedVideoEncoder == VideoEncoder.X265_12 ||
                                          this.SelectedVideoEncoder == VideoEncoder.QuickSync || this.SelectedVideoEncoder == VideoEncoder.QuickSyncH265 || this.SelectedVideoEncoder == VideoEncoder.QuickSyncH26510b ||
                                          this.SelectedVideoEncoder == VideoEncoder.VceH264 || this.SelectedVideoEncoder == VideoEncoder.VceH265 ||
                                          this.SelectedVideoEncoder == VideoEncoder.NvencH264 || this.SelectedVideoEncoder == VideoEncoder.NvencH265 ||
                                          this.SelectedVideoEncoder == VideoEncoder.VP8 || this.SelectedVideoEncoder == VideoEncoder.VP9;

            this.DisplayNonQSVControls = this.SelectedVideoEncoder != VideoEncoder.QuickSync && this.SelectedVideoEncoder != VideoEncoder.QuickSyncH265 && this.SelectedVideoEncoder != VideoEncoder.QuickSyncH26510b;

            this.DisplayTurboFirstPass = selectedEncoder == VideoEncoder.X264 || selectedEncoder == VideoEncoder.X264_10 ||
                                         selectedEncoder == VideoEncoder.X265 || selectedEncoder == VideoEncoder.X265_10 || selectedEncoder == VideoEncoder.X265_12;

            this.DisplayTuneControls = this.SelectedVideoEncoder == VideoEncoder.X264 || this.SelectedVideoEncoder == VideoEncoder.X264_10 || 
                                       this.SelectedVideoEncoder == VideoEncoder.X265 || this.SelectedVideoEncoder == VideoEncoder.X265_10 || this.SelectedVideoEncoder == VideoEncoder.X265_12;

            this.DisplayLevelControl = this.SelectedVideoEncoder == VideoEncoder.X264 || this.SelectedVideoEncoder == VideoEncoder.X264_10 || 
                                       this.SelectedVideoEncoder == VideoEncoder.QuickSync || this.SelectedVideoEncoder == VideoEncoder.QuickSyncH265 || this.SelectedVideoEncoder == VideoEncoder.QuickSyncH26510b ||
                                       this.SelectedVideoEncoder == VideoEncoder.VceH264 || this.SelectedVideoEncoder == VideoEncoder.VceH265 ||
                                       this.SelectedVideoEncoder == VideoEncoder.NvencH264 || this.SelectedVideoEncoder == VideoEncoder.NvencH265;

            this.DisplayProfileControl = this.SelectedVideoEncoder == VideoEncoder.X264
                                         || this.SelectedVideoEncoder == VideoEncoder.X264_10
                                         || this.SelectedVideoEncoder == VideoEncoder.X265
                                         || this.SelectedVideoEncoder == VideoEncoder.X265_10
                                         || this.SelectedVideoEncoder == VideoEncoder.X265_12
                                         || this.SelectedVideoEncoder == VideoEncoder.QuickSync
                                         || this.SelectedVideoEncoder == VideoEncoder.QuickSyncH265
                                         || this.SelectedVideoEncoder == VideoEncoder.QuickSyncH26510b
                                         || this.SelectedVideoEncoder == VideoEncoder.VceH264
                                         || this.SelectedVideoEncoder == VideoEncoder.VceH265
                                         || this.SelectedVideoEncoder == VideoEncoder.NvencH264
                                         || this.SelectedVideoEncoder == VideoEncoder.NvencH265;

            // Refresh Display
            this.NotifyOfPropertyChange(() => this.Rfqp);
            this.NotifyOfPropertyChange(() => this.IsAdvancedTabOptionEnabled);
            this.NotifyOfPropertyChange(() => this.HighQualityLabel);

            // Handle some quicksync specific options.
            if (selectedEncoder == VideoEncoder.QuickSync || selectedEncoder == VideoEncoder.QuickSyncH265 || selectedEncoder == VideoEncoder.QuickSyncH26510b)
            {
                this.TwoPass = false;
                this.TurboFirstPass = false;
                this.Task.Framerate = null;
                this.NotifyOfPropertyChange(() => SelectedFramerate);
                this.UseAdvancedTab = false;
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
    }
}