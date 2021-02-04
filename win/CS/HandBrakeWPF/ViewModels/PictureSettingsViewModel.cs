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
    using System.ComponentModel;
    using System.Globalization;

    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.Interfaces.Model.Picture;

    using HandBrakeWPF.EventArgs;
    using HandBrakeWPF.Model.Picture;
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.Services.Scan.Model;
    using HandBrakeWPF.Utilities;
    using HandBrakeWPF.ViewModelItems.Filters;
    using HandBrakeWPF.ViewModels.Interfaces;

    using EncodeTask = Services.Encode.Model.EncodeTask;
    using Size = Model.Picture.Size;

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
        private bool showKeepAr = true;

        private DelayedActionProcessor delayedPreviewprocessor = new DelayedActionProcessor();
        private Title currentTitle;
        private Source scannedSource;

        private PictureSettingsResLimitModes selectedPictureSettingsResLimitMode;

        public PictureSettingsViewModel(IStaticPreviewViewModel staticPreviewViewModel)
        {
            this.StaticPreviewViewModel = staticPreviewViewModel;
            this.StaticPreviewViewModel.SetPictureSettingsInstance(this);
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

        public int? MaxHeight
        {
            get => this.Task.MaxHeight;
            set
            {
                if (value != this.Task.MaxHeight)
                {
                    this.Task.MaxHeight = value;
                    this.NotifyOfPropertyChange(() => this.MaxHeight);
                    this.OnTabStatusChanged(null);
                }
            }
        }

        public int? MaxWidth
        {
            get => this.Task.MaxWidth;
            set
            {
                if (value != this.Task.MaxWidth)
                {
                    this.Task.MaxWidth = value;
                    this.NotifyOfPropertyChange(() => this.MaxWidth);
                    this.OnTabStatusChanged(null);
                }
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

        public BindingList<PictureSettingsResLimitModes> ResolutionLimitModes => new BindingList<PictureSettingsResLimitModes>
                                                                                 {
                                                                                     PictureSettingsResLimitModes.Size8K,
                                                                                     PictureSettingsResLimitModes.Size4K,
                                                                                     PictureSettingsResLimitModes.Size1080p,
                                                                                     PictureSettingsResLimitModes.Size720p,
                                                                                     PictureSettingsResLimitModes.Size576p,
                                                                                     PictureSettingsResLimitModes.Size480p,
                                                                                     PictureSettingsResLimitModes.Custom,
                                                                                 };

        public PictureSettingsResLimitModes SelectedPictureSettingsResLimitMode
        {
            get => this.selectedPictureSettingsResLimitMode;
            set
            {
                if (value == this.selectedPictureSettingsResLimitMode)
                {
                    return;
                }

                this.selectedPictureSettingsResLimitMode = value;
                this.NotifyOfPropertyChange(() => this.SelectedPictureSettingsResLimitMode);

                this.IsCustomMaxRes = value == PictureSettingsResLimitModes.Custom;
                this.NotifyOfPropertyChange(() => this.IsCustomMaxRes);

                // Enforce the new limit
                ResLimit limit = EnumHelper<PictureSettingsResLimitModes>.GetAttribute<ResLimit, PictureSettingsResLimitModes>(value);

                if (value == PictureSettingsResLimitModes.Custom && this.Task.MaxWidth == null && this.Task.MaxHeight == null)
                {
                    // Default to 4K if null!
                    limit = EnumHelper<PictureSettingsResLimitModes>.GetAttribute<ResLimit, PictureSettingsResLimitModes>(PictureSettingsResLimitModes.Size4K);
                }
                
                if (limit != null)
                {
                    this.Task.MaxWidth = limit.Width;
                    this.Task.MaxHeight = limit.Height;

                    if (this.MaxWidth.HasValue && this.Width > this.MaxWidth)
                    {
                        this.Width = this.MaxWidth.Value;
                    }

                    if (this.MaxHeight.HasValue && this.Height > this.MaxHeight)
                    {
                        this.Height = this.MaxHeight.Value;
                    }
                }


                if (value == PictureSettingsResLimitModes.None)
                {
                    this.Task.MaxWidth = null;
                    this.Task.MaxHeight = null;
                }

                this.NotifyOfPropertyChange(() => this.MaxWidth);
                this.NotifyOfPropertyChange(() => this.MaxHeight);
            }
        }

        public bool IsCustomMaxRes { get; private set; }

        /* Task Properties */

        public int CropBottom
        {
            get => this.Task.Cropping.Bottom;

            set
            {
                if (!ValidCropTB(value, this.CropTop))
                {
                    return; // Don't accept user input.
                }

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
                if (!ValidCropLR(value, this.CropRight))
                {
                    return; // Don't accept user input.
                }

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
                if (!ValidCropLR(value, this.CropLeft))
                {
                    return; // Don't accept user input.
                }

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
                if (!ValidCropTB(value, this.CropBottom))
                {
                    return; // Don't accept user input.
                }

                this.Task.Cropping.Top = value;
                this.NotifyOfPropertyChange(() => this.CropTop);
                this.RecaulcatePictureSettingsProperties(ChangedPictureField.Crop);
            }
        }

        public int MaxCropLR => currentTitle?.Resolution.Width - 8 ?? 0;
        
        public int MaxCropTB => currentTitle?.Resolution.Height - 8 ?? 0;
        
        public bool IsCustomCrop
        {
            get => this.Task.HasCropping;

            set
            {
                this.Task.HasCropping = value;
                this.NotifyOfPropertyChange(() => this.IsCustomCrop);
                this.OnTabStatusChanged(null);

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

            // Padding and Rotate Filters
            this.PaddingFilter.SetPreset(preset, task);
            this.RotateFlipFilter?.SetPreset(preset, task);

            // Picture Sizes and Anamorphic
            this.SelectedAnamorphicMode = preset.Task.Anamorphic;

            // Modulus
            if (preset.Task.Modulus.HasValue)
            {
                this.SelectedModulus = preset.Task.Modulus;
            }

            // Set the Maintain Aspect ratio.
            this.MaintainAspectRatio = preset.Task.KeepDisplayAspect;

            // Setup the Maximum Width / Height with sane 4K fallback.
            this.MaxWidth = preset.Task.MaxWidth ?? 3840;
            this.MaxHeight = preset.Task.MaxHeight ?? 2160;
            this.SetSelectedPictureSettingsResLimitMode();

            // Set the width, then check the height doesn't breach the max height and correct if necessary.
            int width = this.GetModulusValue(this.GetRes((this.sourceResolution.Width - this.CropLeft - this.CropRight), this.MaxWidth));
            int height = this.GetModulusValue(this.GetRes((this.sourceResolution.Height - this.CropTop - this.CropBottom), this.MaxHeight));

            // Set the backing fields to avoid triggering re-calculation until both are set.
            this.Task.Width = width;
            this.Task.Height = height;

            // Trigger a calculation
            this.RecaulcatePictureSettingsProperties(ChangedPictureField.Width);

            // Update the UI
            this.NotifyOfPropertyChange(() => this.Width);
            this.NotifyOfPropertyChange(() => this.Height);


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
            this.SetSelectedPictureSettingsResLimitMode();

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
            this.NotifyOfPropertyChange(() => this.MaxWidth);
            this.NotifyOfPropertyChange(() => this.MaxHeight);

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

                // Update the cropping values, preferring those in the presets.
                if (preset.Task.HasCropping)
                {
                    this.Task.Cropping.Left = preset.Task.Cropping.Left;
                    this.Task.Cropping.Right = preset.Task.Cropping.Right;
                    this.Task.Cropping.Top = preset.Task.Cropping.Top;
                    this.Task.Cropping.Bottom = preset.Task.Cropping.Bottom;
                    this.IsCustomCrop = true;
                }
                else if (!this.IsCustomCrop)  
                {
                    // Only set Auto-crop values if we are in Automatic mode. If it's custom, assume the user has taken control.
                    this.Task.Cropping.Top = title.AutoCropDimensions.Top;
                    this.Task.Cropping.Bottom = title.AutoCropDimensions.Bottom;
                    this.Task.Cropping.Left = title.AutoCropDimensions.Left;
                    this.Task.Cropping.Right = title.AutoCropDimensions.Right;
                    this.IsCustomCrop = false;
                }
                
                // Set the W/H
                // Set the width, then check the height doesn't breach the max height and correct if necessary.
                this.Task.Width = this.GetModulusValue(this.GetRes((this.sourceResolution.Width - this.CropLeft - this.CropRight), this.MaxWidth));
                this.Task.Height = this.GetModulusValue(this.GetRes((this.sourceResolution.Height - this.CropTop - this.CropBottom), this.MaxHeight));

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

            if (preset.Task.MaxHeight != this.MaxHeight)
            {
                return false;
            }

            if (preset.Task.MaxWidth != this.MaxWidth)
            {
                return false;
            }

            if (!preset.Task.HasCropping && this.IsCustomCrop)
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

            // Default the Max Width / Height to 4K format
            this.MaxHeight = 2160;
            this.MaxWidth = 3480;
            this.SetSelectedPictureSettingsResLimitMode();
        }

        private PictureSettingsTitle GetPictureTitleInfo()
        {
            PictureSettingsTitle title = new PictureSettingsTitle
            {
                Width = this.sourceResolution.Width,
                Height = this.sourceResolution.Height,
                ParW = this.sourceParValues.Width,
                ParH = this.sourceParValues.Height,
            };

            return title;
        }

        private PictureSettingsJob GetPictureSettings(ChangedPictureField changedField)
        {
            PictureSettingsJob job = new PictureSettingsJob
            {
                Width = this.Width,
                Height = this.Height,
                ItuPar = false,
                Modulus = this.SelectedModulus,
                ParW = this.SelectedAnamorphicMode == Anamorphic.None ? 1 : this.ParWidth,
                ParH = this.SelectedAnamorphicMode == Anamorphic.None ? 1 : this.ParHeight,
                MaxWidth = this.MaxWidth.HasValue ? this.MaxWidth.Value : 0,
                MaxHeight = this.MaxHeight.HasValue ? this.MaxHeight.Value : 0,
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
            HandBrakePictureHelpers.KeepSetting setting = HandBrakePictureHelpers.KeepSetting.HB_KEEP_WIDTH;
            switch (changedField)
            {
                case ChangedPictureField.Width:
                    setting = HandBrakePictureHelpers.KeepSetting.HB_KEEP_WIDTH;
                    break;
                case ChangedPictureField.Height:
                    setting = HandBrakePictureHelpers.KeepSetting.HB_KEEP_HEIGHT;
                    break;
            }

            // Step 2, For the changed field, call hb_set_anamorphic_size and process the results.
            AnamorphicResult result = HandBrakePictureHelpers.hb_set_anamorphic_size2(this.GetPictureSettings(changedField), this.GetPictureTitleInfo(), setting);
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

        private void SetSelectedPictureSettingsResLimitMode()
        {
            // Look for a matching resolution.
            foreach (PictureSettingsResLimitModes limit in EnumHelper<PictureSettingsResLimitModes>.GetEnumList())
            {
                ResLimit resLimit = EnumHelper<PictureSettingsResLimitModes>.GetAttribute<ResLimit, PictureSettingsResLimitModes>(limit);
                if (resLimit != null)
                {
                    if (resLimit.Width == this.MaxWidth && resLimit.Height == this.MaxHeight)
                    {
                        this.SelectedPictureSettingsResLimitMode = limit;
                        return;
                    }
                }
            }

            if (this.MaxWidth.HasValue || this.MaxHeight.HasValue)
            {
                this.SelectedPictureSettingsResLimitMode = PictureSettingsResLimitModes.Custom;
            }
        }

        private bool ValidCropTB(int value, int value2)
        {
            int totalCrop = value + value2;
            if (totalCrop >= this.currentTitle.Resolution.Height)
            {
                return false;
            }

            return true;
        }

        private bool ValidCropLR(int value, int value2)
        {
            int totalCrop = value + value2;
            if (totalCrop >= this.currentTitle.Resolution.Width)
            {
                return false;
            }

            return true;
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