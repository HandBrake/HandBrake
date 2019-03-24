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
    using System;
    using System.Collections.Generic;
    using System.Globalization;

    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.Model;
    using HandBrake.Interop.Interop.Model.Encoding;

    using HandBrakeWPF.EventArgs;
    using HandBrakeWPF.Helpers;
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.Services.Scan.Model;
    using HandBrakeWPF.Utilities;
    using HandBrakeWPF.ViewModels.Interfaces;

    using EncodeTask = HandBrakeWPF.Services.Encode.Model.EncodeTask;
    using PresetPictureSettingsMode = HandBrakeWPF.Model.Picture.PresetPictureSettingsMode;

    /// <summary>
    /// The Picture Settings View Model
    /// </summary>
    public class PictureSettingsViewModel : ViewModelBase, IPictureSettingsViewModel
    {
        /*
         * TODO:
         * - We are not handling cropping correctly within the UI.
         * - The Height is not correctly set when using no Anamorphic
         * - Maintain Aspect ratio needs corrected.
         * - Custom Anamorphic.
         * 
         */
        #region Constants and Fields

        /// <summary>
        /// The display size.
        /// </summary>
        private string displaySize;

        /// <summary>
        ///  Backing field for for height control enabled
        /// </summary>
        private bool heightControlEnabled = true;

        /// <summary>
        ///  Backing field for show custom anamorphic controls
        /// </summary>
        private bool showCustomAnamorphicControls;

        /// <summary>
        /// The source info.
        /// </summary>
        private string sourceInfo;

        /// <summary>
        /// Source Par Values
        /// </summary>
        private Size sourceParValues;

        /// <summary>
        /// Source Resolution
        /// </summary>
        private Size sourceResolution;

        /// <summary>
        /// Backing field for width control enabled.
        /// </summary>
        private bool widthControlEnabled = true;

        /// <summary>
        /// Backing field for the show modulus field
        /// </summary>
        private bool showModulus;

        /// <summary>
        /// Backing field for showing display size.
        /// </summary>
        private bool showDisplaySize;

        /// <summary>
        /// Backing field for max height
        /// </summary>
        private int maxHeight;

        /// <summary>
        /// Backing field for max width
        /// </summary>
        private int maxWidth;

        /// <summary>
        /// The show keep ar backing field.
        /// </summary>
        private bool showKeepAr = true;

        /// <summary>
        /// The delayed previewprocessor.
        /// </summary>
        private DelayedActionProcessor delayedPreviewprocessor = new DelayedActionProcessor();

        /// <summary>
        /// The current title.
        /// </summary>
        private Title currentTitle;

        /// <summary>
        /// The scanned source.
        /// </summary>
        private Source scannedSource;

        #endregion

        #region Constructors and Destructors

        /// <summary>
        /// Initializes a new instance of the <see cref="HandBrakeWPF.ViewModels.PictureSettingsViewModel"/> class.
        /// </summary>
        public PictureSettingsViewModel(IStaticPreviewViewModel staticPreviewViewModel)
        {
            this.StaticPreviewViewModel = staticPreviewViewModel;
            this.sourceResolution = new Size(0, 0);
            this.Task = new EncodeTask();
            this.Init();
        }

        #endregion

        public event EventHandler<TabStatusEventArgs> TabStatusChanged;

        #region Properties

        /// <summary>
        /// Gets or sets the static preview view model.
        /// </summary>
        public IStaticPreviewViewModel StaticPreviewViewModel { get; set; }

        /// <summary>
        /// Gets AnamorphicModes.
        /// </summary>
        public IEnumerable<Anamorphic> AnamorphicModes
        {
            get
            {
                return new List<Anamorphic> { Anamorphic.None, Anamorphic.Automatic, Anamorphic.Loose, Anamorphic.Custom };
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
        /// Gets or sets a value indicating whether HeightControlEnabled.
        /// </summary>
        public bool HeightControlEnabled
        {
            get
            {
                return this.heightControlEnabled;
            }

            set
            {
                this.heightControlEnabled = value;
                this.NotifyOfPropertyChange(() => this.HeightControlEnabled);
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
        /// Gets or sets a value indicating whether ShowCustomAnamorphicControls.
        /// </summary>
        public bool ShowCustomAnamorphicControls
        {
            get
            {
                return this.showCustomAnamorphicControls;
            }

            set
            {
                this.showCustomAnamorphicControls = value;
                this.NotifyOfPropertyChange(() => this.ShowCustomAnamorphicControls);
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
        /// Gets or sets Task.
        /// </summary>
        public EncodeTask Task { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether WidthControlEnabled.
        /// </summary>
        public bool WidthControlEnabled
        {
            get
            {
                return this.widthControlEnabled;
            }

            set
            {
                this.widthControlEnabled = value;
                this.NotifyOfPropertyChange(() => this.WidthControlEnabled);
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether ShowModulus.
        /// </summary>
        public bool ShowModulus
        {
            get
            {
                return this.showModulus;
            }
            set
            {
                this.showModulus = value;
                this.NotifyOfPropertyChange(() => this.ShowModulus);
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether ShowDisplaySize.
        /// </summary>
        public bool ShowDisplaySize
        {
            get
            {
                return this.showDisplaySize;
            }
            set
            {
                this.showDisplaySize = value;
                this.NotifyOfPropertyChange(() => this.ShowDisplaySize);
            }
        }

        /// <summary>
        /// Gets or sets MaxHeight.
        /// </summary>
        public int MaxHeight
        {
            get
            {
                return this.maxHeight;
            }
            set
            {
                this.maxHeight = value;
                this.NotifyOfPropertyChange(() => this.MaxHeight);
            }
        }

        /// <summary>
        /// Gets or sets MinHeight.
        /// </summary>
        public int MaxWidth
        {
            get
            {
                return this.maxWidth;
            }
            set
            {
                this.maxWidth = value;
                this.NotifyOfPropertyChange(() => this.MaxWidth);
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether show keep ar.
        /// </summary>
        public bool ShowKeepAR
        {
            get
            {
                return this.showKeepAr;
            }
            set
            {
                this.showKeepAr = value;
                this.NotifyOfPropertyChange(() => this.ShowKeepAR);
            }
        }

        #endregion

        #region Task Properties

        /// <summary>
        /// Gets or sets CropBottom.
        /// </summary>
        public int CropBottom
        {
            get
            {
                return this.Task.Cropping.Bottom;
            }

            set
            {
                this.Task.Cropping.Bottom = value;
                this.NotifyOfPropertyChange(() => this.CropBottom);
                this.RecaulcatePictureSettingsProperties(ChangedPictureField.Crop);
            }
        }

        /// <summary>
        /// Gets or sets CropLeft.
        /// </summary>
        public int CropLeft
        {
            get
            {
                return this.Task.Cropping.Left;
            }

            set
            {
                this.Task.Cropping.Left = value;
                this.NotifyOfPropertyChange(() => this.CropLeft);
                this.RecaulcatePictureSettingsProperties(ChangedPictureField.Crop);
            }
        }

        /// <summary>
        /// Gets or sets CropRight.
        /// </summary>
        public int CropRight
        {
            get
            {
                return this.Task.Cropping.Right;
            }

            set
            {
                this.Task.Cropping.Right = value;
                this.NotifyOfPropertyChange(() => this.CropRight);
                this.RecaulcatePictureSettingsProperties(ChangedPictureField.Crop);
            }
        }

        /// <summary>
        /// Gets or sets CropTop.
        /// </summary>
        public int CropTop
        {
            get
            {
                return this.Task.Cropping.Top;
            }

            set
            {
                this.Task.Cropping.Top = value;
                this.NotifyOfPropertyChange(() => this.CropTop);
                this.RecaulcatePictureSettingsProperties(ChangedPictureField.Crop);
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether IsCustomCrop.
        /// </summary>
        public bool IsCustomCrop
        {
            get
            {
                return this.Task.HasCropping;
            }

            set
            {
                this.Task.HasCropping = value;
                this.NotifyOfPropertyChange(() => this.IsCustomCrop);

                if (!value && this.currentTitle != null)
                {
                    this.CropTop = currentTitle.AutoCropDimensions.Top;
                    this.CropBottom = currentTitle.AutoCropDimensions.Bottom;
                    this.CropLeft = currentTitle.AutoCropDimensions.Left;
                    this.CropRight = currentTitle.AutoCropDimensions.Right;
                }
            }
        }

        /// <summary>
        /// Gets or sets DisplayWidth.
        /// </summary>
        public long DisplayWidth
        {
            get
            {
                return this.Task.DisplayWidth.HasValue
                           ? int.Parse(Math.Round(this.Task.DisplayWidth.Value, 0).ToString(CultureInfo.InvariantCulture))
                           : 0;
            }

            set
            {
                if (!object.Equals(this.Task.DisplayWidth, value))
                {
                    this.Task.DisplayWidth = value;
                    this.NotifyOfPropertyChange(() => this.DisplayWidth);
                    this.RecaulcatePictureSettingsProperties(ChangedPictureField.DisplayWidth);
                }
            }
        }

        /// <summary>
        /// Gets or sets Width.
        /// </summary>
        public int Width
        {
            get
            {
                return this.Task.Width.HasValue ? this.Task.Width.Value : 0;
            }

            set
            {
                if (!object.Equals(this.Task.Width, value))
                {
                    this.Task.Width = value;
                    this.NotifyOfPropertyChange(() => this.Width);
                    this.RecaulcatePictureSettingsProperties(ChangedPictureField.Width);
                }
            }
        }

        /// <summary>
        /// Gets or sets Height.
        /// </summary>
        public int Height
        {
            get
            {
                return this.Task.Height.HasValue ? this.Task.Height.Value : 0;
            }

            set
            {
                if (!object.Equals(this.Task.Height, value))
                {
                    this.Task.Height = value;
                    this.NotifyOfPropertyChange(() => this.Height);
                    this.RecaulcatePictureSettingsProperties(ChangedPictureField.Height);
                }
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether MaintainAspectRatio.
        /// </summary>
        public bool MaintainAspectRatio
        {
            get
            {
                return this.Task.KeepDisplayAspect;
            }

            set
            {
                this.Task.KeepDisplayAspect = value;
                this.NotifyOfPropertyChange(() => this.MaintainAspectRatio);
                this.RecaulcatePictureSettingsProperties(ChangedPictureField.MaintainAspectRatio);
            }
        }

        /// <summary>
        /// Gets or sets ParHeight.
        /// </summary>
        public int ParHeight
        {
            get
            {
                return this.Task.PixelAspectY;
            }

            set
            {
                if (!object.Equals(this.Task.PixelAspectY, value))
                {
                    this.Task.PixelAspectY = value;
                    this.NotifyOfPropertyChange(() => this.ParHeight);
                    this.RecaulcatePictureSettingsProperties(ChangedPictureField.ParH);
                }
            }
        }

        /// <summary>
        /// Gets or sets ParWidth.
        /// </summary>
        public int ParWidth
        {
            get
            {
                return this.Task.PixelAspectX;
            }

            set
            {
                if (!object.Equals(this.Task.PixelAspectX, value))
                {
                    this.Task.PixelAspectX = value;
                    this.NotifyOfPropertyChange(() => this.ParWidth);
                    this.RecaulcatePictureSettingsProperties(ChangedPictureField.ParW);
                }
            }
        }

        /// <summary>
        /// Gets or sets SelectedAnamorphicMode.
        /// </summary>
        public Anamorphic SelectedAnamorphicMode
        {
            get
            {
                return this.Task.Anamorphic;
            }

            set
            {
                if (!object.Equals(this.SelectedAnamorphicMode, value))
                {
                    this.Task.Anamorphic = value;
                    this.NotifyOfPropertyChange(() => this.SelectedAnamorphicMode);
                    this.RecaulcatePictureSettingsProperties(ChangedPictureField.Anamorphic);
                    this.OnTabStatusChanged(null);
                }
            }
        }

        /// <summary>
        /// Gets or sets SelectedModulus.
        /// </summary>
        public int? SelectedModulus
        {
            get
            {
                return this.Task.Modulus;
            }

            set
            {
                this.Task.Modulus = value;
                this.NotifyOfPropertyChange(() => this.SelectedModulus);
                this.RecaulcatePictureSettingsProperties(ChangedPictureField.Modulus);
                this.OnTabStatusChanged(null);
            }
        }

        #endregion

        #region Public Methods

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

            // Handle built-in presets.
            if (preset.IsBuildIn)
            {
                preset.PictureSettingsMode = PresetPictureSettingsMode.Custom;
            }

            // Cropping
            if (preset.Task.HasCropping)
            {
                this.IsCustomCrop = true;
                this.Task.Cropping.Left = preset.Task.Cropping.Left;
                this.Task.Cropping.Right = preset.Task.Cropping.Right;
                this.Task.Cropping.Top = preset.Task.Cropping.Top;
                this.Task.Cropping.Bottom = preset.Task.Cropping.Bottom;

                this.NotifyOfPropertyChange(() => this.CropLeft);
                this.NotifyOfPropertyChange(() => this.CropRight);
                this.NotifyOfPropertyChange(() => this.CropTop);
                this.NotifyOfPropertyChange(() => this.CropBottom);
            }
            else
            {
                this.IsCustomCrop = false;
            }


            // Setup the Picture Sizes
            switch (preset.PictureSettingsMode)
            {
                default:
                case PresetPictureSettingsMode.Custom:
                case PresetPictureSettingsMode.SourceMaximum:

                    // Anamorphic Mode
                    this.SelectedAnamorphicMode = preset.Task.Anamorphic;

                    // Modulus
                    if (preset.Task.Modulus.HasValue)
                    {
                        this.SelectedModulus = preset.Task.Modulus;
                    }

                    // Set the Maintain Aspect ratio.
                    this.MaintainAspectRatio = preset.Task.KeepDisplayAspect;

                    // Set the Maximum so libhb can correctly manage the size.
                    if (preset.PictureSettingsMode == PresetPictureSettingsMode.SourceMaximum)
                    {
                        this.MaxWidth = this.sourceResolution.Width;
                        this.MaxHeight = this.sourceResolution.Height;
                    }
                    else
                    {

                        int presetWidth = preset.Task.MaxWidth ?? this.sourceResolution.Width;
                        int presetHeight = preset.Task.MaxHeight ?? this.sourceResolution.Height;

                        this.MaxWidth = presetWidth <= this.sourceResolution.Width ? presetWidth : this.sourceResolution.Width;
                        this.MaxHeight = presetHeight <= this.sourceResolution.Height ? presetHeight : this.sourceResolution.Height;                        
                    }             

                    // Set the width, then check the height doesn't breach the max height and correct if necessary.
                    int width = this.GetModulusValue(this.GetRes((this.sourceResolution.Width - this.CropLeft - this.CropRight), this.MaxWidth));
                    int height = this.GetModulusValue(this.GetRes((this.sourceResolution.Height - this.CropTop - this.CropBottom), this.MaxHeight));

                    // Set the backing fields to avoid triggering recalulation until both are set.
                    this.Task.Width = width;
                    this.Task.Height = height;

                    // Trigger a Recalc
                    this.RecaulcatePictureSettingsProperties(ChangedPictureField.Width);

                    // Update the UI
                    this.NotifyOfPropertyChange(() => this.Width);
                    this.NotifyOfPropertyChange(() => this.Height);

                    break;
                case PresetPictureSettingsMode.None:
                    // Do Nothing except reset the Max Width/Height
                    this.MaxWidth = this.sourceResolution.Width;
                    this.MaxHeight = this.sourceResolution.Height;
                    this.SelectedAnamorphicMode = preset.Task.Anamorphic;

                    if (this.Width > this.MaxWidth)
                    {
                        // Trigger a Recalc
                        this.Task.Width = this.GetModulusValue(this.GetRes((this.sourceResolution.Width - this.CropLeft - this.CropRight), this.MaxWidth));
                        this.RecaulcatePictureSettingsProperties(ChangedPictureField.Width);
                    }

                    break;
            }

            // Custom Anamorphic
            if (preset.Task.Anamorphic == Anamorphic.Custom)
            {
                this.DisplayWidth = preset.Task.DisplayWidth != null ? int.Parse(preset.Task.DisplayWidth.ToString()) : 0;
                this.ParWidth = preset.Task.PixelAspectX;
                this.ParHeight = preset.Task.PixelAspectY;
            }

            this.NotifyOfPropertyChange(() => this.Task);

            this.UpdateVisibileControls();
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
            this.NotifyOfPropertyChange(() => this.Width);
            this.NotifyOfPropertyChange(() => this.Height);
            this.NotifyOfPropertyChange(() => this.SelectedAnamorphicMode);
            this.NotifyOfPropertyChange(() => this.SelectedModulus);
            this.NotifyOfPropertyChange(() => this.CropTop);
            this.NotifyOfPropertyChange(() => this.CropBottom);
            this.NotifyOfPropertyChange(() => this.CropLeft);
            this.NotifyOfPropertyChange(() => this.CropRight);
            this.NotifyOfPropertyChange(() => this.IsCustomCrop);
            this.NotifyOfPropertyChange(() => this.MaintainAspectRatio);
            this.NotifyOfPropertyChange(() => this.DisplayWidth);
            this.NotifyOfPropertyChange(() => this.ParWidth);
            this.NotifyOfPropertyChange(() => this.ParHeight);

            this.UpdateVisibileControls();
        }

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
            this.currentTitle = title;
            this.Task = task;

            this.scannedSource = source;

            if (title != null)
            {
                // Set cached info
                this.sourceParValues = title.ParVal;
                this.sourceResolution = title.Resolution;

                // Update the cropping values, preffering those in the presets.
                if (!preset.Task.HasCropping)
                {
                    this.Task.Cropping.Top = title.AutoCropDimensions.Top;
                    this.Task.Cropping.Bottom = title.AutoCropDimensions.Bottom;
                    this.Task.Cropping.Left = title.AutoCropDimensions.Left;
                    this.Task.Cropping.Right = title.AutoCropDimensions.Right;
                    this.IsCustomCrop = false;
                }
                else
                {
                    this.Task.Cropping.Left = preset.Task.Cropping.Left;
                    this.Task.Cropping.Right = preset.Task.Cropping.Right;
                    this.Task.Cropping.Top = preset.Task.Cropping.Top;
                    this.Task.Cropping.Bottom = preset.Task.Cropping.Bottom;
                    this.IsCustomCrop = true;
                }

                // Set the Max Width / Height available to the user controls.
                // Preset Max is null for None / SourceMax
                this.MaxWidth = preset.Task.MaxWidth ?? this.sourceResolution.Width;
                if (this.sourceResolution.Width < this.MaxWidth)
                {
                    this.MaxWidth = this.sourceResolution.Width;
                }

                this.MaxHeight = preset.Task.MaxHeight ?? this.sourceResolution.Height;
                if (this.sourceResolution.Height < this.MaxHeight)
                {
                    this.MaxHeight = this.sourceResolution.Height;
                }

                // Set the W/H
                if (preset.PictureSettingsMode == PresetPictureSettingsMode.None)
                {
                    this.Task.Width = this.GetModulusValue(this.sourceResolution.Width - this.CropLeft - this.CropRight);
                    this.Task.Height = this.GetModulusValue(this.sourceResolution.Height - this.CropTop - this.CropBottom);
                }
                else if (preset.PictureSettingsMode == PresetPictureSettingsMode.SourceMaximum)
                {
                    this.Task.Width = this.GetModulusValue(this.sourceResolution.Width - this.CropLeft - this.CropRight);
                    this.Task.Height = this.GetModulusValue(this.sourceResolution.Height - this.CropTop - this.CropBottom);
                    this.MaintainAspectRatio = preset.Task.KeepDisplayAspect;
                }
                else 
                {
                    // Custom
                    // Set the Width, and Maintain Aspect ratio. That should calc the Height for us.
                    this.Task.Width = this.GetModulusValue(this.sourceResolution.Width - this.CropLeft - this.CropRight);
           
                    if (this.SelectedAnamorphicMode != Anamorphic.Loose)
                    {
                        this.Task.Height = this.GetModulusValue(this.sourceResolution.Height - this.CropTop - this.CropBottom);
                    }
                }

                // Set Screen Controls
                this.SourceInfo = string.Format(
                    "{0}x{1}, PAR: {2}/{3}",
                    title.Resolution.Width,
                    title.Resolution.Height,
                    title.ParVal.Width,
                    title.ParVal.Height);

                // Force a re-calc. This will handle MaxWidth / Height corrections.
                this.RecaulcatePictureSettingsProperties(ChangedPictureField.Width);
            }

            this.NotifyOfPropertyChange(() => this.Task);
        }

        public bool MatchesPreset(Preset preset)
        {
            if (preset.Task.Anamorphic != this.SelectedAnamorphicMode)
            {
                return false;
            }

            if (preset.Task.Modulus != this.SelectedModulus)
            {
                return false;
            }

            return true;
        }

        #endregion

        #region Methods

        protected virtual void OnTabStatusChanged(TabStatusEventArgs e)
        {
            this.TabStatusChanged?.Invoke(this, e);
        }

        /// <summary>
        /// The init.
        /// </summary>
        private void Init()
        {
            this.Task.Modulus = 16;
            this.Task.KeepDisplayAspect = true;

            this.NotifyOfPropertyChange(() => this.SelectedModulus);
            this.NotifyOfPropertyChange(() => this.MaintainAspectRatio);

            // Default the Max Width / Height to 1080p format
            this.MaxHeight = 1080;
            this.MaxWidth = 1920;
        }

        /// <summary>
        /// The get picture title info.
        /// </summary>
        /// <returns>
        /// The <see cref="PictureSize.PictureSettingsTitle"/>.
        /// </returns>
        private PictureSize.PictureSettingsTitle GetPictureTitleInfo()
        {
            PictureSize.PictureSettingsTitle title = new PictureSize.PictureSettingsTitle
            {
                Width = this.sourceResolution.Width,
                Height = this.sourceResolution.Height,
                ParW = this.sourceParValues.Width,
                ParH = this.sourceParValues.Height,
            };

            return title;
        }

        /// <summary>
        /// The get picture settings.
        /// </summary>
        /// <returns>
        /// The <see cref="PictureSize.PictureSettingsJob"/>.
        /// </returns>
        private PictureSize.PictureSettingsJob GetPictureSettings(ChangedPictureField changedField)
        {
            PictureSize.PictureSettingsJob job = new PictureSize.PictureSettingsJob
            {
                Width = this.Width,
                Height = this.Height,
                ItuPar = false,
                Modulus = this.SelectedModulus,
                ParW = this.ParWidth,
                ParH = this.ParHeight,
                MaxWidth = this.MaxWidth,
                MaxHeight = this.MaxHeight,
                KeepDisplayAspect = this.MaintainAspectRatio,
                AnamorphicMode = this.SelectedAnamorphicMode,
                Crop = new Cropping(this.CropTop, this.CropBottom, this.CropLeft, this.CropRight),
            };

            if (this.SelectedAnamorphicMode == Anamorphic.Custom)
            {
                if (changedField == ChangedPictureField.DisplayWidth)
                {
                    var displayWidth = this.DisplayWidth;
                    job.ParW = (int)displayWidth;  // num
                    job.ParH = job.Width; // den
                }
            }

            // Reduce the Par W/H if we can. Don't do it while the user is altering the PAR controls through as it will mess with the result.
            if (changedField != ChangedPictureField.ParH && changedField != ChangedPictureField.ParW)
            {
                long x, y;
                HandBrakeUtils.Reduce(job.ParW, job.ParH, out x, out y);
                job.ParW = (int)y;
                job.ParH = (int)x;
            }

            return job;
        }

        /// <summary>
        /// Recalculate the picture settings when the user changes a particular field defined in the ChangedPictureField enum.
        /// The properties in this class are dumb. They simply call this method if there is a change.
        /// It is the job of this method to update all affected private fields and raise change notifications. 
        /// </summary>
        /// <param name="changedField">
        /// The changed field.
        /// </param>
        private void RecaulcatePictureSettingsProperties(ChangedPictureField changedField)
        {
            // Sanity Check
            if (this.currentTitle == null)
            {
                return;
            }

            // Step 1, Update what controls are visible.
            this.UpdateVisibileControls();

            // Step 2, Set sensible defaults
            if (changedField == ChangedPictureField.Anamorphic && (this.SelectedAnamorphicMode == Anamorphic.None || this.SelectedAnamorphicMode == Anamorphic.Loose))
            {
                this.Task.Width = this.sourceResolution.Width > this.MaxWidth
                                      ? this.MaxWidth
                                      : this.sourceResolution.Width;
                this.Task.KeepDisplayAspect = true;
            }

            // Choose which setting to keep.
            PictureSize.KeepSetting setting = PictureSize.KeepSetting.HB_KEEP_WIDTH;
            switch (changedField)
            {
                case ChangedPictureField.Width:
                    setting = PictureSize.KeepSetting.HB_KEEP_WIDTH;
                    break;
                case ChangedPictureField.Height:
                    setting = PictureSize.KeepSetting.HB_KEEP_HEIGHT;
                    break;
            }

            // Step 2, For the changed field, call hb_set_anamorphic_size and process the results.
            PictureSize.AnamorphicResult result = PictureSize.hb_set_anamorphic_size2(this.GetPictureSettings(changedField), this.GetPictureTitleInfo(), setting);
            double dispWidth = Math.Round((result.OutputWidth * result.OutputParWidth / result.OutputParHeight), 0);

            this.Task.Width = result.OutputWidth;
            this.Task.Height = result.OutputHeight;
            long x, y;
            HandBrakeUtils.Reduce((int)Math.Round(result.OutputParWidth, 0), (int)Math.Round(result.OutputParHeight, 0), out x, out y);
            this.Task.PixelAspectX = (int)Math.Round(result.OutputParWidth, 0);
            this.Task.PixelAspectY = (int)Math.Round(result.OutputParHeight, 0);
            this.Task.DisplayWidth = dispWidth;

            // Step 3, Set the display width label to indicate the output.
            this.DisplaySize = this.sourceResolution == null || this.sourceResolution.IsEmpty
                           ? string.Empty
                           : string.Format(Resources.PictureSettingsViewModel_StorageDisplayLabel, dispWidth, result.OutputHeight, this.ParWidth, this.ParHeight);

            // Step 4, Force an update on all the UI elements.
            this.NotifyOfPropertyChange(() => this.Width);
            this.NotifyOfPropertyChange(() => this.Height);
            this.NotifyOfPropertyChange(() => this.DisplayWidth);
            this.NotifyOfPropertyChange(() => this.ParWidth);
            this.NotifyOfPropertyChange(() => this.ParHeight);
            this.NotifyOfPropertyChange(() => this.CropTop);
            this.NotifyOfPropertyChange(() => this.CropBottom);
            this.NotifyOfPropertyChange(() => this.CropLeft);
            this.NotifyOfPropertyChange(() => this.CropRight);
            this.NotifyOfPropertyChange(() => this.SelectedModulus);
            this.NotifyOfPropertyChange(() => this.MaintainAspectRatio);

            // Step 5, Update the Preview
            if (delayedPreviewprocessor != null && this.Task != null && this.StaticPreviewViewModel != null && this.StaticPreviewViewModel.IsOpen)
            {
                delayedPreviewprocessor.PerformTask(() => this.StaticPreviewViewModel.UpdatePreviewFrame(this.Task, this.scannedSource), 800);
            }
        }

        /// <summary>
        /// The update visible controls.
        /// </summary>
        private void UpdateVisibileControls()
        {
            this.ShowDisplaySize = true;
            this.ShowKeepAR = true;

            switch (this.SelectedAnamorphicMode)
            {
                case Anamorphic.None:
                    this.WidthControlEnabled = true;
                    this.HeightControlEnabled = true;
                    this.ShowCustomAnamorphicControls = false;
                    this.ShowModulus = true;
                    this.ShowDisplaySize = true;
                    this.ShowKeepAR = true;
                    break;
                case Anamorphic.Automatic:
                    this.WidthControlEnabled = true;
                    this.HeightControlEnabled = true;
                    this.ShowCustomAnamorphicControls = false;
                    this.ShowModulus = true;
                    this.ShowKeepAR = false;
                    break;

                case Anamorphic.Loose:
                    this.WidthControlEnabled = true;
                    this.HeightControlEnabled = false;
                    this.ShowCustomAnamorphicControls = false;
                    this.ShowModulus = true;
                    this.ShowKeepAR = false;
                    break;

                case Anamorphic.Custom:
                    this.WidthControlEnabled = true;
                    this.HeightControlEnabled = true;
                    this.ShowCustomAnamorphicControls = true;
                    this.ShowModulus = true;
                    this.ShowDisplaySize = true;
                    this.ShowKeepAR = true;
                    break;
            }
        }

        /// <summary>
        /// For a given value, correct so that it matches the users currently selected modulus value
        /// </summary>
        /// <param name="value">
        /// The value.
        /// </param>
        /// <returns>
        /// Value corrected so that value % selected modulus == 0
        /// </returns>
        private int GetModulusValue(double value)
        {
            if (this.SelectedModulus == null)
            {
                return 0;
            }

            double remainder = value % this.SelectedModulus.Value;

            if (remainder.Equals(0.0d))
            {
                return (int)Math.Abs(value);
            }

            double result = remainder >= ((double)this.SelectedModulus.Value / 2)
                       ? value + (this.SelectedModulus.Value - remainder)
                       : value - remainder;

            return (int)Math.Abs(result);
        }

        /// <summary>
        /// The get res.
        /// </summary>
        /// <param name="value">
        /// The value.
        /// </param>
        /// <param name="max">
        /// The max.
        /// </param>
        /// <returns>
        /// The <see cref="int"/>.
        /// </returns>
        private int GetRes(int value, int? max)
        {
            return max.HasValue ? (value > max.Value ? max.Value : value) : value;
        }

        #endregion
    }

    /// <summary>
    /// The changed picture field.
    /// </summary>
    public enum ChangedPictureField
    {
        Width,
        Height,
        ParW,
        ParH,
        DisplayWidth,
        Crop,
        Anamorphic,
        MaintainAspectRatio,
        Modulus
    }
}