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
    using System.Collections.Generic;
    using System.ComponentModel.Composition;

    using Caliburn.Micro;

    using HandBrake.ApplicationServices.Functions;
    using HandBrake.ApplicationServices.Model;
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
        /// The average bitrate.
        /// </summary>
        private int averageBitrate;

        /// <summary>
        /// The is constant framerate.
        /// </summary>
        private bool isConstantFramerate;

        /// <summary>
        /// The is constant quantity.
        /// </summary>
        private bool isConstantQuantity;

        /// <summary>
        /// The is peak framerate.
        /// </summary>
        private bool isPeakFramerate;

        /// <summary>
        /// The is turbo first pass.
        /// </summary>
        private bool isTurboFirstPass;

        /// <summary>
        /// The is two pass.
        /// </summary>
        private bool isTwoPass;

        /// <summary>
        /// The is variable framerate.
        /// </summary>
        private bool isVariableFramerate;

        /// <summary>
        /// The quality max.
        /// </summary>
        private int qualityMax;

        /// <summary>
        /// The quality min.
        /// </summary>
        private int qualityMin;

        /// <summary>
        /// The rf.
        /// </summary>
        private int rf;

        /// <summary>
        /// The selected framerate.
        /// </summary>
        private double? selectedFramerate;

        /// <summary>
        /// The selected video encoder.
        /// </summary>
        private VideoEncoder selectedVideoEncoder;

        /// <summary>
        /// The show peak framerate.
        /// </summary>
        private bool showPeakFramerate;

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
            this.QualityMin = 0;
            this.QualityMax = 51;
            this.IsConstantQuantity = true;
        }

        #endregion

        #region Public Properties

        /// <summary>
        /// Gets or sets AverageBitrate.
        /// </summary>
        public string AverageBitrate
        {
            get
            {
                return this.averageBitrate.ToString();
            }
            set
            {
                if (value != null)
                {
                    this.averageBitrate = int.Parse(value);
                }
                this.NotifyOfPropertyChange(() => this.AverageBitrate);
            }
        }

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
                return this.isConstantFramerate;
            }
            set
            {
                this.isConstantFramerate = value;
                if (value)
                {
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
                return this.isConstantQuantity;
            }
            set
            {
                this.isConstantQuantity = value;
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
                return this.isPeakFramerate;
            }
            set
            {
                this.isPeakFramerate = value;
                if (value)
                {
                    this.IsVariableFramerate = false;
                    this.IsConstantFramerate = false;
                }

                this.NotifyOfPropertyChange(() => this.IsPeakFramerate);
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether IsTurboFirstPass.
        /// </summary>
        public bool IsTurboFirstPass
        {
            get
            {
                return this.isTurboFirstPass;
            }
            set
            {
                this.isTurboFirstPass = value;
                this.NotifyOfPropertyChange(() => this.IsTurboFirstPass);
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether IsTwoPass.
        /// </summary>
        public bool IsTwoPass
        {
            get
            {
                return this.isTwoPass;
            }
            set
            {
                this.isTwoPass = value;
                this.NotifyOfPropertyChange(() => this.IsTwoPass);
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether IsVariableFramerate.
        /// </summary>
        public bool IsVariableFramerate
        {
            get
            {
                return this.isVariableFramerate;
            }
            set
            {
                this.isVariableFramerate = value;
                if (value)
                {
                    this.IsPeakFramerate = false;
                    this.IsConstantFramerate = false;
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
                return this.rf;
            }
            set
            {
                this.rf = value;
                this.NotifyOfPropertyChange(() => this.RF);
            }
        }

        /// <summary>
        /// Gets or sets SelectedFramerate.
        /// </summary>
        public string SelectedFramerate
        {
            get
            {
                if (this.selectedFramerate == null)
                {
                    return "Same as source";
                }

                return this.selectedFramerate.ToString();
            }
            set
            {
                if (value == "Same as source")
                {
                    this.selectedFramerate = null;
                    this.ShowPeakFramerate = false;
                }
                else
                {
                    this.ShowPeakFramerate = true;
                    this.selectedFramerate = double.Parse(value);
                }

                this.NotifyOfPropertyChange(() => this.SelectedFramerate);
            }
        }

        /// <summary>
        /// Gets or sets SelectedVideoEncoder.
        /// </summary>
        public string SelectedVideoEncoder
        {
            get
            {
                return EnumHelper<VideoEncoder>.GetDisplay(this.selectedVideoEncoder);
            }
            set
            {
                this.selectedVideoEncoder = EnumHelper<VideoEncoder>.GetValue(value);
                this.NotifyOfPropertyChange(() => this.SelectedVideoEncoder);
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
        /// Gets VideoEncoders.
        /// </summary>
        public IEnumerable<string> VideoEncoders
        {
            get
            {
                return EnumHelper<VideoEncoder>.GetEnumDisplayValues(typeof(VideoEncoder));
            }
        }

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
        }

        #endregion
    }
}