// --------------------------------------------------------------------------------------------------------------------
// <copyright file="PictureSettingsViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Picture Settings View Model
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels
{
    using System.Collections.Generic;
    using System.ComponentModel.Composition;

    using Caliburn.Micro;

    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Parsing;
    using HandBrake.ApplicationServices.Services.Interfaces;
    using HandBrake.Interop.Model.Encoding;

    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    /// The Picture Settings View Model
    /// </summary>
    [Export(typeof(IPictureSettingsViewModel))]
    public class PictureSettingsViewModel : ViewModelBase, IPictureSettingsViewModel
    {
        #region Constants and Fields

        /// <summary>
        /// The crop bottom.
        /// </summary>
        private int cropBottom;

        /// <summary>
        /// The crop left.
        /// </summary>
        private int cropLeft;

        /// <summary>
        /// The crop right.
        /// </summary>
        private int cropRight;

        /// <summary>
        /// The crop top.
        /// </summary>
        private int cropTop;

        /// <summary>
        /// The display size.
        /// </summary>
        private string displaySize;

        /// <summary>
        /// The display width.
        /// </summary>
        private int displayWidth;

        /// <summary>
        /// The height.
        /// </summary>
        private int height;

        /// <summary>
        /// The is custom crop.
        /// </summary>
        private bool isCustomCrop;

        /// <summary>
        /// The maintain aspect ratio.
        /// </summary>
        private bool maintainAspectRatio;

        /// <summary>
        /// The par height.
        /// </summary>
        private int parHeight;

        /// <summary>
        /// The par width.
        /// </summary>
        private int parWidth;

        /// <summary>
        /// The selected anamorphic mode.
        /// </summary>
        private Anamorphic selectedAnamorphicMode;

        /// <summary>
        /// The selected modulus
        /// </summary>
        private int selectedModulus;

        /// <summary>
        /// The source info.
        /// </summary>
        private string sourceInfo;

        /// <summary>
        /// The width.
        /// </summary>
        private int width;

        #endregion

        #region Constructors and Destructors

        /// <summary>
        /// Initializes a new instance of the <see cref="HandBrakeWPF.ViewModels.PictureSettingsViewModel"/> class.
        /// </summary>
        /// <param name="windowManager">
        /// The window manager.
        /// </param>
        /// <param name="userSettingService">
        /// The user Setting Service.
        /// </param>
        public PictureSettingsViewModel(IWindowManager windowManager, IUserSettingService userSettingService)
        {
            this.SelectedModulus = 16;
        }

        #endregion

        #region Public Properties

        /// <summary>
        /// Gets AnamorphicModes.
        /// </summary>
        public IEnumerable<Anamorphic> AnamorphicModes
        {
            get
            {
                return new List<Anamorphic> { Anamorphic.None, Anamorphic.Strict, Anamorphic.Loose, Anamorphic.Custom };
            }
        }

        /// <summary>
        /// Gets or sets CropBottom.
        /// </summary>
        public int CropBottom
        {
            get
            {
                return this.cropBottom;
            }
            set
            {
                this.cropBottom = value;
                this.NotifyOfPropertyChange(() => this.CropBottom);
            }
        }

        /// <summary>
        /// Gets or sets CropLeft.
        /// </summary>
        public int CropLeft
        {
            get
            {
                return this.cropLeft;
            }
            set
            {
                this.cropLeft = value;
                this.NotifyOfPropertyChange(() => this.CropLeft);
            }
        }

        /// <summary>
        /// Gets or sets CropRight.
        /// </summary>
        public int CropRight
        {
            get
            {
                return this.cropRight;
            }
            set
            {
                this.cropRight = value;
                this.NotifyOfPropertyChange(() => this.CropRight);
            }
        }

        /// <summary>
        /// Gets or sets CropTop.
        /// </summary>
        public int CropTop
        {
            get
            {
                return this.cropTop;
            }
            set
            {
                this.cropTop = value;
                this.NotifyOfPropertyChange(() => this.CropTop);
            }
        }

        /// <summary>
        /// Gets or sets DisplaySize.
        /// </summary>
        public string DisplaySize
        {
            get
            {
                return this.displaySize;
            }
            set
            {
                this.displaySize = value;
                this.NotifyOfPropertyChange(() => this.DisplaySize);
            }
        }

        /// <summary>
        /// Gets or sets DisplayWidth.
        /// </summary>
        public int DisplayWidth
        {
            get
            {
                return this.displayWidth;
            }
            set
            {
                this.displayWidth = value;
                this.NotifyOfPropertyChange(() => this.DisplayWidth);
            }
        }

        /// <summary>
        /// Gets or sets Height.
        /// </summary>
        public int Height
        {
            get
            {
                return this.height;
            }
            set
            {
                this.height = value;
                this.NotifyOfPropertyChange(() => this.Height);
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether IsCustomCrop.
        /// </summary>
        public bool IsCustomCrop
        {
            get
            {
                return this.isCustomCrop;
            }
            set
            {
                this.isCustomCrop = value;
                this.NotifyOfPropertyChange(() => this.IsCustomCrop);
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether MaintainAspectRatio.
        /// </summary>
        public bool MaintainAspectRatio
        {
            get
            {
                return this.maintainAspectRatio;
            }
            set
            {
                this.maintainAspectRatio = value;
                this.NotifyOfPropertyChange(() => this.MaintainAspectRatio);
            }
        }

        /// <summary>
        /// Gets ModulusValues.
        /// </summary>
        public IEnumerable<int> ModulusValues
        {
            get
            {
                return new List<int> { 16, 8, 4, 2 };
            }
        }

        /// <summary>
        /// Gets or sets ParHeight.
        /// </summary>
        public int ParHeight
        {
            get
            {
                return this.parHeight;
            }
            set
            {
                this.parHeight = value;
                this.NotifyOfPropertyChange(() => this.ParHeight);
            }
        }

        /// <summary>
        /// Gets or sets ParWidth.
        /// </summary>
        public int ParWidth
        {
            get
            {
                return this.parWidth;
            }
            set
            {
                this.parWidth = value;
                this.NotifyOfPropertyChange(() => this.ParWidth);
            }
        }

        /// <summary>
        /// Gets or sets SelectedAnamorphicMode.
        /// </summary>
        public Anamorphic SelectedAnamorphicMode
        {
            get
            {
                return this.selectedAnamorphicMode;
            }
            set
            {
                this.selectedAnamorphicMode = value;
                this.NotifyOfPropertyChange(() => this.SelectedAnamorphicMode);
            }
        }

        /// <summary>
        /// Gets or sets SelectedModulus.
        /// </summary>
        public int SelectedModulus
        {
            get
            {
                return this.selectedModulus;
            }
            set
            {
                this.selectedModulus = value;
                this.NotifyOfPropertyChange(() => this.SelectedModulus);
            }
        }

        /// <summary>
        /// Gets or sets SourceInfo.
        /// </summary>
        public string SourceInfo
        {
            get
            {
                return this.sourceInfo;
            }
            set
            {
                this.sourceInfo = value;
                this.NotifyOfPropertyChange(() => this.SourceInfo);
            }
        }

        /// <summary>
        /// Gets or sets Width.
        /// </summary>
        public int Width
        {
            get
            {
                return this.width;
            }
            set
            {
                this.width = value;
                this.NotifyOfPropertyChange(() => this.Width);
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