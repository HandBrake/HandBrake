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
    using System.ComponentModel.Composition;
    using System.Globalization;

    using Caliburn.Micro;

    using HandBrake.ApplicationServices;
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Model.Encoding;
    using HandBrake.ApplicationServices.Parsing;
    using HandBrake.ApplicationServices.Services.Interfaces;
    using HandBrake.ApplicationServices.Utilities;
    using HandBrake.Interop.Model.Encoding;
    using HandBrake.Interop.Model.Encoding.x264;

    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    /// The Video View Model
    /// </summary>
    [Export(typeof(IVideoViewModel))]
    public class VideoViewModel : ViewModelBase, IVideoViewModel
    {
        #region Constants and Fields
        /// <summary>
        /// Backing field for the user setting service.
        /// </summary>
        private IUserSettingService userSettingService;

        /// <summary>
        /// Backing field used to display / hide the x264 options
        /// </summary>
        private bool displayX264Options;

        /// <summary>
        /// The quality max.
        /// </summary>
        private int qualityMax;

        /// <summary>
        /// The quality min.
        /// </summary>
        private int qualityMin;

        /// <summary>
        /// The show peak framerate.
        /// </summary>
        private bool showPeakFramerate;

        /// <summary>
        /// The show peak framerate.
        /// </summary>
        private int rf;

        #endregion

        #region Constructors and Destructors

        /// <summary>
        /// Initializes a new instance of the <see cref="VideoViewModel"/> class.
        /// </summary>
        /// <param name="windowManager">
        /// The window manager.
        /// </param>
        /// <param name="userSettingService">
        /// The user Setting Service.
        /// </param>
        public VideoViewModel(IWindowManager windowManager, IUserSettingService userSettingService)
        {
            this.Task = new EncodeTask { VideoEncoder = VideoEncoder.X264 };
            this.userSettingService = userSettingService;
            this.QualityMin = 0;
            this.QualityMax = 51;
            this.IsConstantQuantity = true;
            this.VideoEncoders = EnumHelper<VideoEncoder>.GetEnumList();

            //X264Presets = EnumHelper<x264Preset>.GetEnumList();
            //X264Profiles = EnumHelper<x264Profile>.GetEnumList();
            //X264Tunes = EnumHelper<x264Tune>.GetEnumList();
        }

        #endregion

        #region Public Properties

        /// <summary>
        /// Gets or sets the current Encode Task.
        /// </summary>
        public EncodeTask Task { get; set; }

        /// <summary>
        /// Gets Framerates.
        /// </summary>
        public IEnumerable<string> Framerates
        {
            get
            {
                return new List<string> { "Same as source", "5", "10", "12", "15", "23.976", "24", "25", "29.97", "30", "50", "59.94", "60" };
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
                    this.Task.TwoPass = false;
                    this.Task.TurboFirstPass = false;
                    this.Task.VideoBitrate = null;
                    this.NotifyOfPropertyChange(() => this.Task);
                }
                else
                {
                    this.Task.VideoEncodeRateType = VideoEncodeRateType.AverageBitrate;
                }

                this.NotifyOfPropertyChange(() => this.IsConstantQuantity);
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

                double cqStep = userSettingService.GetUserSetting<double>(ASUserSettingConstants.X264Step);
                this.SetQualitySliderBounds(); 
                switch (this.SelectedVideoEncoder)
                {
                    case VideoEncoder.FFMpeg:
                    case VideoEncoder.FFMpeg2:
                        this.Task.Quality = (32 - value);
                        break;
                    case VideoEncoder.X264:
                        double rfValue = 51.0 - value * cqStep;
                        rfValue = Math.Round(rfValue, 2);
                        this.Task.Quality = rfValue;

                        // TODO: Lossless warning.
                        break;
                    case VideoEncoder.Theora:
                        Task.Quality = value;
                        break;
                }

                this.NotifyOfPropertyChange(() => this.RF);
                this.NotifyOfPropertyChange(() => this.DisplayRF);
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

                return this.Task.Framerate.ToString();
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
                    this.Task.Framerate = double.Parse(value);
                }

                this.NotifyOfPropertyChange(() => this.SelectedFramerate);
                this.NotifyOfPropertyChange(() => this.Task);
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
                this.Task.VideoEncoder = value;
                this.NotifyOfPropertyChange(() => this.SelectedVideoEncoder);

                // Tell the Advanced Panel off the change
                IAdvancedViewModel advancedViewModel = IoC.Get<IAdvancedViewModel>();
                advancedViewModel.SetEncoder(this.Task.VideoEncoder);

                // Update the Quality Slider. Make sure the bounds are up to date with the users settings.
                this.SetQualitySliderBounds();
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

        #endregion

        #region Public Methods

        /// <summary>
        /// Setup this window for a new source
        /// </summary>
        /// <param name="title">
        /// The title.
        /// </param>
        /// <param name="preset">
        /// The preset.
        /// </param>
        /// <param name="task">
        /// The task.
        /// </param>
        public void SetSource(Title title, Preset preset, EncodeTask task)
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
            if (preset.Task.Framerate.HasValue)
            {
                this.SelectedFramerate = preset.Task.Framerate.Value.ToString(CultureInfo.InvariantCulture);
            }

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

            double cqStep = userSettingService.GetUserSetting<double>(ASUserSettingConstants.X264Step);
            double rfValue = 0;
            this.SetQualitySliderBounds();
            switch (this.SelectedVideoEncoder)
            {
                case VideoEncoder.FFMpeg:
                case VideoEncoder.FFMpeg2:
                    int cq;
                    if (preset.Task.Quality.HasValue)
                    {
                        int.TryParse(preset.Task.Quality.Value.ToString(CultureInfo.InvariantCulture), out cq);
                        this.RF = 32 - cq;
                    }
                    break;
                case VideoEncoder.X264:
 
                    double multiplier = 1.0 / cqStep;
                    if (preset.Task.Quality.HasValue)
                    {
                        rfValue = preset.Task.Quality.Value * multiplier;
                    }

                    this.RF = this.QualityMax - (int)Math.Round(rfValue, 0);

                    break;

                case VideoEncoder.Theora:
                    if (preset.Task.Quality.HasValue)
                    {
                        this.RF = (int)preset.Task.Quality.Value;
                    }
                    break;
            }

            this.Task.TwoPass = preset.Task.TwoPass;
            this.Task.TurboFirstPass = preset.Task.TurboFirstPass;
            this.Task.VideoBitrate = preset.Task.VideoBitrate;

            this.NotifyOfPropertyChange(() => this.Task);

            //if (preset != null && preset.Task != null)
            //{
            //    this.Query = preset.Task.AdvancedEncoderOptions;
            //    this.SetEncoder(preset.Task.VideoEncoder);

            //    this.X264Preset = preset.Task.x264Preset;
            //    this.X264Profile = preset.Task.x264Profile;
            //    this.X264Tune = preset.Task.X264Tune;
            //}
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
            this.NotifyOfPropertyChange(() => this.IsConstantFramerate);
            this.NotifyOfPropertyChange(() => this.IsConstantQuantity);
            this.NotifyOfPropertyChange(() => this.IsPeakFramerate);
            this.NotifyOfPropertyChange(() => this.IsVariableFramerate);
            this.NotifyOfPropertyChange(() => this.SelectedVideoEncoder);
            this.NotifyOfPropertyChange(() => this.SelectedFramerate);
            this.NotifyOfPropertyChange(() => this.RF);
            this.NotifyOfPropertyChange(() => this.DisplayRF);
            this.NotifyOfPropertyChange(() => this.Task.VideoBitrate);
            this.NotifyOfPropertyChange(() => this.Task.TwoPass);
            this.NotifyOfPropertyChange(() => this.Task.TurboFirstPass);
        }

        /// <summary>
        /// Set the currently selected encoder.
        /// </summary>
        /// <param name="encoder">
        /// The Video Encoder.
        /// </param>
        public void SetEncoder(VideoEncoder encoder)
        {
            //this.DisplayX264Options = encoder == VideoEncoder.X264;
        }

        /// <summary>
        /// Trigger a Notify Property Changed on the Task to force various UI elements to update.
        /// </summary>
        public void RefreshTask()
        {
            this.NotifyOfPropertyChange(() => this.Task);

            if (Task.OutputFormat == OutputFormat.Mp4 && this.SelectedVideoEncoder == VideoEncoder.Theora)
            {
                this.SelectedVideoEncoder = VideoEncoder.X264;
            }
        }

        #endregion

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
                case VideoEncoder.X264:
                    this.QualityMin = 0;
                    this.QualityMax = (int)(51 / userSettingService.GetUserSetting<double>(ASUserSettingConstants.X264Step));
                    break;
                case VideoEncoder.Theora:
                    this.QualityMin = 0;
                    this.QualityMax = 63;
                    break;
            }
        }

        #region Advanced
        ///// <summary>
        ///// Gets or sets State.
        ///// </summary>
        //public string Query
        //{
        //    get
        //    {
        //        return this.Task.AdvancedEncoderOptions;
        //    }
        //    set
        //    {
        //        this.Task.AdvancedEncoderOptions = value;
        //        this.NotifyOfPropertyChange(() => this.Query);
        //    }
        //}

        ///// <summary>
        ///// Gets or sets X264Preset.
        ///// </summary>
        //public x264Preset X264Preset
        //{
        //    get
        //    {
        //        return this.Task.x264Preset;
        //    }
        //    set
        //    {
        //        this.Task.x264Preset = value;
        //        this.NotifyOfPropertyChange(() => this.X264Preset);
        //    }
        //}

        ///// <summary>
        ///// Gets or sets X264Profile.
        ///// </summary>
        //public x264Profile X264Profile
        //{
        //    get
        //    {
        //        return this.Task.x264Profile;
        //    }
        //    set
        //    {
        //        this.Task.x264Profile = value;
        //        this.NotifyOfPropertyChange(() => this.X264Profile);
        //    }
        //}

        ///// <summary>
        ///// Gets or sets X264Tune.
        ///// </summary>
        //public x264Tune X264Tune
        //{
        //    get
        //    {
        //        return this.Task.X264Tune;
        //    }
        //    set
        //    {
        //        this.Task.X264Tune = value;
        //        this.NotifyOfPropertyChange(() => this.X264Tune);
        //    }
        //}

        ///// <summary>
        ///// Gets or sets X264Presets.
        ///// </summary>
        //public IEnumerable<x264Preset> X264Presets { get; set; }

        ///// <summary>
        ///// Gets or sets X264Profiles.
        ///// </summary>
        //public IEnumerable<x264Profile> X264Profiles { get; set; }

        ///// <summary>
        ///// Gets or sets X264Tunes.
        ///// </summary>
        //public IEnumerable<x264Tune> X264Tunes { get; set; }

        ///// <summary>
        ///// Gets or sets a value indicating whether DisplayX264Options.
        ///// </summary>
        //public bool DisplayX264Options
        //{
        //    get
        //    {
        //        return this.displayX264Options;
        //    }
        //    set
        //    {
        //        this.displayX264Options = value;
        //        this.NotifyOfPropertyChange(() => this.DisplayX264Options);
        //    }
        //}
        #endregion
    }
}