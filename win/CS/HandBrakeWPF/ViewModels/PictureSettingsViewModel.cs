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
    using HandBrakeWPF.ViewModelItems.Filters;
    using HandBrakeWPF.ViewModels.Interfaces;

    using EncodeTask = HandBrakeWPF.Services.Encode.Model.EncodeTask;
    using PresetPictureSettingsMode = HandBrakeWPF.Model.Picture.PresetPictureSettingsMode;

    public class PictureSettingsViewModel : ViewModelBase, IPictureSettingsViewModel
    {
        private string displaySize;
        private bool heightControlEnabled = true;
        private bool showCustomAnamorphicControls;
        private string sourceInfo;
        private Size sourceParValues;
        private Size sourceResolution;
        private bool widthControlEnabled = true;
        private bool showModulus;
        private bool showDisplaySize;
        private int maxHeight;
        private int maxWidth;
        private bool showKeepAr = true;

        private DelayedActionProcessor delayedPreviewprocessor = new DelayedActionProcessor();
        private Title currentTitle;
        private Source scannedSource;

        public PictureSettingsViewModel(IStaticPreviewViewModel staticPreviewViewModel)
        {
            this.StaticPreviewViewModel = staticPreviewViewModel;
            this.sourceResolution = new Size(0, 0);
            this.Task = new EncodeTask();
            this.PaddingFilter = new PadFilter(this.Task, () => this.OnTabStatusChanged(null));
            this.RotateFlipFilter = new RotateFlipFilter(this.Task, () => this.OnTabStatusChanged(null));
            this.Init();
        }

        public event EventHandler<TabStatusEventArgs> TabStatusChanged;

        /* Properties */

        public IStaticPreviewViewModel StaticPreviewViewModel { get; set; }

        public IEnumerable<Anamorphic> AnamorphicModes { get; } = new List<Anamorphic> { Anamorphic.None, Anamorphic.Automatic, Anamorphic.Loose, Anamorphic.Custom };

        public PadFilter PaddingFilter { get; set; }

        public RotateFlipFilter RotateFlipFilter { get; set; }

        public string DisplaySize
        {
            get => this.displaySize;

            set
            {
                this.displaySize = value;
                this.NotifyOfPropertyChange(() => this.DisplaySize);
            }
        }

        public bool HeightControlEnabled
        {
            get => this.heightControlEnabled;

            set
            {
                this.heightControlEnabled = value;
                this.NotifyOfPropertyChange(() => this.HeightControlEnabled);
            }
        }

        public IEnumerable<int> ModulusValues { get; } = new List<int> { 16, 8, 4, 2 };

        public bool ShowCustomAnamorphicControls
        {
            get => this.showCustomAnamorphicControls;

            set
            {
                this.showCustomAnamorphicControls = value;
                this.NotifyOfPropertyChange(() => this.ShowCustomAnamorphicControls);
            }
        }

        public string SourceInfo
        {
            get => this.sourceInfo;

            set
            {
                this.sourceInfo = value;
                this.NotifyOfPropertyChange(() => this.SourceInfo);
            }
        }

        public EncodeTask Task { get; set; }

        public bool WidthControlEnabled
        {
            get => this.widthControlEnabled;

            set
            {
                this.widthControlEnabled = value;
                this.NotifyOfPropertyChange(() => this.WidthControlEnabled);
            }
        }

        public bool ShowModulus
        {
            get => this.showModulus;
            set
            {
                this.showModulus = value;
                this.NotifyOfPropertyChange(() => this.ShowModulus);
            }
        }

        public bool ShowDisplaySize
        {
            get => this.showDisplaySize;
            set
            {
                this.showDisplaySize = value;
                this.NotifyOfPropertyChange(() => this.ShowDisplaySize);
            }
        }

        public int MaxHeight
        {
            get => this.maxHeight;
            set
            {
                this.maxHeight = value;
                this.NotifyOfPropertyChange(() => this.MaxHeight);
            }
        }

        public int MaxWidth
        {
            get => this.maxWidth;
            set
            {
                this.maxWidth = value;
                this.NotifyOfPropertyChange(() => this.MaxWidth);
            }
        }

        public bool ShowKeepAR
        {
            get => this.showKeepAr;
            set
            {
                this.showKeepAr = value;
                this.NotifyOfPropertyChange(() => this.ShowKeepAR);
            }
        }

        /* Task Properties */

        public int CropBottom
        {
            get => this.Task.Cropping.Bottom;

            set
            {
                this.Task.Cropping.Bottom = value;
                this.NotifyOfPropertyChange(() => this.CropBottom);
                this.RecaulcatePictureSettingsProperties(ChangedPictureField.Crop);
            }
        }

        public int CropLeft
        {
            get => this.Task.Cropping.Left;

            set
            {
                this.Task.Cropping.Left = value;
                this.NotifyOfPropertyChange(() => this.CropLeft);
                this.RecaulcatePictureSettingsProperties(ChangedPictureField.Crop);
            }
        }

        public int CropRight
        {
            get => this.Task.Cropping.Right;

            set
            {
                this.Task.Cropping.Right = value;
                this.NotifyOfPropertyChange(() => this.CropRight);
                this.RecaulcatePictureSettingsProperties(ChangedPictureField.Crop);
            }
        }

        public int CropTop
        {
            get => this.Task.Cropping.Top;

            set
            {
                this.Task.Cropping.Top = value;
                this.NotifyOfPropertyChange(() => this.CropTop);
                this.RecaulcatePictureSettingsProperties(ChangedPictureField.Crop);
            }
        }

        public bool IsCustomCrop
        {
            get => this.Task.HasCropping;

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

        public long DisplayWidth
        {
            get => this.Task.DisplayWidth.HasValue ? int.Parse(Math.Round(this.Task.DisplayWidth.Value, 0).ToString(CultureInfo.InvariantCulture)) : 0;

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

        public int Width
        {
            get => this.Task.Width.HasValue ? this.Task.Width.Value : 0;

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

        public int Height
        {
            get => this.Task.Height.HasValue ? this.Task.Height.Value : 0;

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

        public bool MaintainAspectRatio
        {
            get => this.Task.KeepDisplayAspect;

            set
            {
                this.Task.KeepDisplayAspect = value;
                this.NotifyOfPropertyChange(() => this.MaintainAspectRatio);
                this.RecaulcatePictureSettingsProperties(ChangedPictureField.MaintainAspectRatio);
            }
        }

        public int ParHeight
        {
            get => this.Task.PixelAspectY;

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

        public int ParWidth
        {
            get => this.Task.PixelAspectX;

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

        public Anamorphic SelectedAnamorphicMode
        {
            get => this.Task.Anamorphic;

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

        public int? SelectedModulus
        {
            get => this.Task.Modulus;

            set
            {
                this.Task.Modulus = value;
                this.NotifyOfPropertyChange(() => this.SelectedModulus);
                this.RecaulcatePictureSettingsProperties(ChangedPictureField.Modulus);
                this.OnTabStatusChanged(null);
            }
        }

        /* Public Tab API Implementation */

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

            // Padding
            this.PaddingFilter.SetPreset(preset, task);
            this.RotateFlipFilter?.SetPreset(preset, task);
            
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

        public void UpdateTask(EncodeTask task)
        {
            this.Task = task;
            this.PaddingFilter.UpdateTask(task);
            this.RotateFlipFilter?.UpdateTask(task);

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

        public void SetSource(Source source, Title title, Preset preset, EncodeTask task)
        {
            this.currentTitle = title;
            this.Task = task;
            this.PaddingFilter.SetSource(source, title, preset, task);
            this.RotateFlipFilter?.SetSource(source, title, preset, task);

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

            if (!PaddingFilter.MatchesPreset(preset))
            {
                return false;
            }

            if (!RotateFlipFilter.MatchesPreset(preset))
            {
                return false;
            }

            return true;
        }

         /* Protected and Private Methods */

        protected virtual void OnTabStatusChanged(TabStatusEventArgs e)
        {
            this.TabStatusChanged?.Invoke(this, e);
        }

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

        private PictureSize.PictureSettingsJob GetPictureSettings(ChangedPictureField changedField)
        {
            PictureSize.PictureSettingsJob job = new PictureSize.PictureSettingsJob
            {
                Width = this.Width,
                Height = this.Height,
                ItuPar = false,
                Modulus = this.SelectedModulus,
                ParW = this.SelectedAnamorphicMode == Anamorphic.None ? 1 : this.ParWidth,
                ParH = this.SelectedAnamorphicMode == Anamorphic.None ? 1 : this.ParHeight,
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

        private int GetRes(int value, int? max)
        {
            return max.HasValue ? (value > max.Value ? max.Value : value) : value;
        }
    }

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