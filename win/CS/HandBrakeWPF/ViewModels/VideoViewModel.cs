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

    using Caliburn.Micro;

    using HandBrake.ApplicationServices;
    using HandBrake.ApplicationServices.Functions;
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Model.Encoding;
    using HandBrake.ApplicationServices.Parsing;
    using HandBrake.ApplicationServices.Services.Interfaces;
    using HandBrake.Interop.Model.Encoding;

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
                return new List<string> { "Same as source", "5", "10", "12", "15", "23.976", "24", "25", "29.97`" };
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
                this.qualityMax = value;
                this.NotifyOfPropertyChange(() => this.QualityMax);
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
                this.qualityMin = value;
                this.NotifyOfPropertyChange(() => this.QualityMin);
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
                }
                else if (!string.IsNullOrEmpty(value))
                {
                    this.ShowPeakFramerate = true;
                    if (this.Task.FramerateMode == FramerateMode.VFR)
                    {
                        this.Task.FramerateMode = FramerateMode.PFR; 
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

                // Update the Quality Slider
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
        public void SetPreset(Preset preset)
        {
            if (preset == null || preset.Task == null)
            {
                return;
            }
            
            this.SelectedVideoEncoder = preset.Task.VideoEncoder;
            this.SelectedFramerate = preset.Task.Framerate.ToString();
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
             
            // TODO Compute RF
            this.RF = 20;

            this.Task.TwoPass = preset.Task.TwoPass;
            this.Task.TurboFirstPass = preset.Task.TurboFirstPass;
            this.Task.VideoBitrate = preset.Task.VideoBitrate;

            this.NotifyOfPropertyChange(() => this.Task);
        }

        #endregion
    }
}