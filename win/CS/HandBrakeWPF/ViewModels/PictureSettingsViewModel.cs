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
    using System.ComponentModel;
    using System.Globalization;
    using System.Linq;
    using System.Windows;

    using HandBrake.App.Core.Utilities;
    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.Interfaces.Model.Picture;

    using HandBrakeWPF.EventArgs;
    using HandBrakeWPF.Model;
    using HandBrakeWPF.Model.Filters;
    using HandBrakeWPF.Model.Picture;
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.Services.Scan.Model;
    using HandBrakeWPF.ViewModelItems.Filters;
    using HandBrakeWPF.ViewModels.Interfaces;
    using HandBrakeWPF.Views;

    using EncodeTask = Services.Encode.Model.EncodeTask;
    using Size = Model.Picture.Size;

    public class PictureSettingsViewModel : ViewModelBase, IPictureSettingsViewModel
    {
        private readonly IWindowManager windowManager;
        private bool heightControlEnabled = true;
        private string sourceInfo;
        private Size sourceParValues;
        private Size sourceResolution;
        private bool widthControlEnabled = true;
        private DelayedActionProcessor delayedPreviewprocessor = new DelayedActionProcessor();
        private Title currentTitle;
        private Source scannedSource;
        private PictureSettingsResLimitModes selectedPictureSettingsResLimitMode;

        public PictureSettingsViewModel(IStaticPreviewViewModel staticPreviewViewModel, IWindowManager windowManager)
        {
            this.windowManager = windowManager;
            this.StaticPreviewViewModel = staticPreviewViewModel;
            this.StaticPreviewViewModel.SetPictureSettingsInstance(this);
            this.sourceResolution = new Size(0, 0);
            this.Task = new EncodeTask();
            this.PaddingFilter = new PadFilter(this.Task, () => this.OnFilterChanged(null));
            this.RotateFlipFilter = new RotateFlipFilter(this.Task, e => this.OnFlipRotateChanged(e));
            this.Init();
        }

        /* Events */

        public event EventHandler<TabStatusEventArgs> TabStatusChanged;

        /* Properties */

        public IStaticPreviewViewModel StaticPreviewViewModel { get; set; }

        public BindingList<AnamorphicMode> AnamorphicModes { get; } = new BindingList<AnamorphicMode> { AnamorphicMode.None, AnamorphicMode.Automatic, AnamorphicMode.Custom };

        public PadFilter PaddingFilter { get; set; }

        public RotateFlipFilter RotateFlipFilter { get; set; }

        public string DisplaySize { get; set; }

        public string OutputAspect { get; set; }

        public bool HeightControlEnabled
        {
            get => this.heightControlEnabled;

            set
            {
                this.heightControlEnabled = value;
                this.NotifyOfPropertyChange(() => this.HeightControlEnabled);
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
                }

                if (value == PictureSettingsResLimitModes.None)
                {
                    this.Task.MaxWidth = null;
                    this.Task.MaxHeight = null;
                }

                this.NotifyOfPropertyChange(() => this.MaxWidth);
                this.NotifyOfPropertyChange(() => this.MaxHeight);
                this.RecalculatePictureSettingsProperties(ChangedPictureField.ResolutionLimit);

                this.OnTabStatusChanged(null);
            }
        }

        public bool IsCustomMaxRes { get; private set; }

        public bool OptimalSize
        {
            get => this.Task.OptimalSize;
            set
            {
                this.Task.OptimalSize = value;
                this.UpdateVisibleControls();
                this.Task.Width = this.sourceResolution.Width;

                this.NotifyOfPropertyChange(() => this.OptimalSize);
                this.RecalculatePictureSettingsProperties(ChangedPictureField.OptimalSize);
            }
        }

        public bool AllowUpscaling
        {
            get => this.Task.AllowUpscaling;
            set
            {
                this.Task.AllowUpscaling = value;
                this.UpdateVisibleControls();
                this.RecalculatePictureSettingsProperties(ChangedPictureField.AllowUpscale);
                this.NotifyOfPropertyChange(() => this.AllowUpscaling);
            }
        }

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
                this.RecalculatePictureSettingsProperties(ChangedPictureField.Crop);
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
                this.RecalculatePictureSettingsProperties(ChangedPictureField.Crop);
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
                this.RecalculatePictureSettingsProperties(ChangedPictureField.Crop);
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
                this.RecalculatePictureSettingsProperties(ChangedPictureField.Crop);
            }
        }

        public int MaxCropLR => currentTitle?.Resolution.Width - 8 ?? 0;
        
        public int MaxCropTB => currentTitle?.Resolution.Height - 8 ?? 0;

        public BindingList<CropMode> CropModes { get; } = new BindingList<CropMode> { CropMode.Automatic, CropMode.Loose, CropMode.None, CropMode.Custom };

        public CropMode SelectedCropMode
        {
            get
            {
                return (CropMode)this.Task.Cropping.CropMode;
            }

            set
            {
                this.Task.Cropping.CropMode = (int)value;
                this.NotifyOfPropertyChange(() => this.SelectedCropMode);
                this.OnTabStatusChanged(null);

                if (value != CropMode.Custom && this.currentTitle != null)
                {
                    if (value == CropMode.Automatic)
                    {
                        this.Task.Cropping.Top = currentTitle.AutoCropDimensions.Top;
                        this.Task.Cropping.Bottom = currentTitle.AutoCropDimensions.Bottom;
                        this.Task.Cropping.Left = currentTitle.AutoCropDimensions.Left;
                        this.Task.Cropping.Right = currentTitle.AutoCropDimensions.Right;
                    } 
                    else if (value == CropMode.Loose)
                    {
                        this.Task.Cropping.Top = currentTitle.LooseCropDimensions.Top;
                        this.Task.Cropping.Bottom = currentTitle.LooseCropDimensions.Bottom;
                        this.Task.Cropping.Left = currentTitle.LooseCropDimensions.Left;
                        this.Task.Cropping.Right = currentTitle.LooseCropDimensions.Right;
                    }
                    else
                    {
                        this.Task.Cropping.Top = 0;
                        this.Task.Cropping.Bottom = 0;
                        this.Task.Cropping.Left = 0;
                        this.Task.Cropping.Right = 0;
                    }
                }

                this.NotifyOfPropertyChange(() => this.CropLeft);
                this.NotifyOfPropertyChange(() => this.CropRight);
                this.NotifyOfPropertyChange(() => this.CropTop);
                this.NotifyOfPropertyChange(() => this.CropBottom);
                this.NotifyOfPropertyChange(() => this.IsCustomCrop);

                this.RecalculatePictureSettingsProperties(ChangedPictureField.Crop);
            }
        }

        public bool IsCustomCrop => this.SelectedCropMode == CropMode.Custom;

        public int DisplayWidth
        {
            get
            {
                if (this.Task.DisplayWidth.HasValue && !double.IsInfinity(this.Task.DisplayWidth.Value))
                {
                   if (int.TryParse(Math.Round(this.Task.DisplayWidth.Value, 0).ToString(CultureInfo.InvariantCulture), out int value))
                   {
                       return value;
                   }
                }

                return 0;
            } 

            set
            {
                if (!object.Equals(this.Task.DisplayWidth, value))
                {
                    this.Task.DisplayWidth = value;
                    this.NotifyOfPropertyChange(() => this.DisplayWidth);
                    this.RecalculatePictureSettingsProperties(ChangedPictureField.DisplayWidth);
                }
            }
        }

        public int DisplayHeight { get; set; }

        public int Width
        {
            get => this.Task.Width.HasValue ? this.Task.Width.Value : 0;

            set
            {
                if (!object.Equals(this.Task.Width, value))
                {
                    this.Task.Width = value;
                    this.NotifyOfPropertyChange(() => this.Width);
                    this.RecalculatePictureSettingsProperties(ChangedPictureField.Width);
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
                    this.RecalculatePictureSettingsProperties(ChangedPictureField.Height);
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
                this.RecalculatePictureSettingsProperties(ChangedPictureField.MaintainAspectRatio);
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
                    this.RecalculatePictureSettingsProperties(ChangedPictureField.ParH);
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
                    this.RecalculatePictureSettingsProperties(ChangedPictureField.ParW);
                }
            }
        }

        public AnamorphicMode SelectedAnamorphicMode
        {
            get => (AnamorphicMode)this.Task.Anamorphic;

            set
            {
                if (!object.Equals(this.SelectedAnamorphicMode, value))
                {
                    this.Task.Anamorphic = (Anamorphic)value;
                    this.NotifyOfPropertyChange(() => this.SelectedAnamorphicMode);
                    this.RecalculatePictureSettingsProperties(ChangedPictureField.Anamorphic);
                    this.OnTabStatusChanged(null);
                    this.IsPixelAspectSettable = value == AnamorphicMode.Custom;
                    this.NotifyOfPropertyChange(() => this.IsPixelAspectSettable);
                }
            }
        }

        public bool IsPixelAspectSettable { get; set; }

        /* Public Tab API Implementation */

        public void SetPreset(Preset preset, EncodeTask task)
        {
            this.Task = task;

            // Cropping
            if ((CropMode)preset.Task.Cropping.CropMode == CropMode.Custom)
            {
                this.SelectedCropMode = CropMode.Custom;
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
                this.SelectedCropMode = (CropMode)preset.Task.Cropping.CropMode;
            }

            // Padding and Rotate Filters
            this.PaddingFilter.SetPreset(preset, task);
            this.RotateFlipFilter?.SetPreset(preset, task);

            // Picture Sizes and Anamorphic
            this.SelectedAnamorphicMode = (AnamorphicMode)preset.Task.Anamorphic;
            this.OptimalSize = preset.Task.OptimalSize;
            this.AllowUpscaling = preset.Task.AllowUpscaling;

            // Set the Maintain Aspect ratio.
            this.MaintainAspectRatio = preset.Task.KeepDisplayAspect;

            // Setup the Maximum Width / Height
            this.MaxWidth = preset.Task.MaxWidth;
            this.MaxHeight = preset.Task.MaxHeight;
            this.SetSelectedPictureSettingsResLimitMode();

            // Set the width, then check the height doesn't breach the max height and correct if necessary.
            int width = this.GetModulusValue(this.GetRes((this.sourceResolution.Width - this.CropLeft - this.CropRight), this.MaxWidth));
            int height = this.GetModulusValue(this.GetRes((this.sourceResolution.Height - this.CropTop - this.CropBottom), this.MaxHeight));

            // Set the backing fields to avoid triggering re-calculation until both are set.
            this.Task.Width = width;
            this.Task.Height = height;

            // Trigger a calculation
            this.RecalculatePictureSettingsProperties(ChangedPictureField.Width);

            // Update the UI
            this.NotifyOfPropertyChange(() => this.Width);
            this.NotifyOfPropertyChange(() => this.Height);


            // Custom Anamorphic
            if (preset.Task.Anamorphic == Anamorphic.Custom)
            {
                this.DisplayWidth = preset.Task.DisplayWidth != null ? int.Parse(preset.Task.DisplayWidth.ToString()) : 0;
                this.ParWidth = preset.Task.PixelAspectX == 0 ? 1 : preset.Task.PixelAspectX;
                this.ParHeight = preset.Task.PixelAspectY == 0 ? 1 : preset.Task.PixelAspectY;
            }

            this.NotifyOfPropertyChange(() => this.Task);

            this.UpdateVisibleControls();
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
            this.NotifyOfPropertyChange(() => this.CropTop);
            this.NotifyOfPropertyChange(() => this.CropBottom);
            this.NotifyOfPropertyChange(() => this.CropLeft);
            this.NotifyOfPropertyChange(() => this.CropRight);
            this.NotifyOfPropertyChange(() => this.SelectedCropMode);
            this.NotifyOfPropertyChange(() => this.MaintainAspectRatio);
            this.NotifyOfPropertyChange(() => this.DisplayWidth);
            this.NotifyOfPropertyChange(() => this.ParWidth);
            this.NotifyOfPropertyChange(() => this.ParHeight);
            this.NotifyOfPropertyChange(() => this.MaxWidth);
            this.NotifyOfPropertyChange(() => this.MaxHeight);

            this.UpdateVisibleControls();
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
                if ((CropMode)preset.Task.Cropping.CropMode == CropMode.Custom)
                {
                    this.Task.Cropping.Left = preset.Task.Cropping.Left;
                    this.Task.Cropping.Right = preset.Task.Cropping.Right;
                    this.Task.Cropping.Top = preset.Task.Cropping.Top;
                    this.Task.Cropping.Bottom = preset.Task.Cropping.Bottom;
                    this.SelectedCropMode = CropMode.Custom;
                }
                else
                {
                    if ((CropMode)preset.Task.Cropping.CropMode == CropMode.Automatic)
                    {
                        this.Task.Cropping.Top = title.AutoCropDimensions.Top;
                        this.Task.Cropping.Bottom = title.AutoCropDimensions.Bottom;
                        this.Task.Cropping.Left = title.AutoCropDimensions.Left;
                        this.Task.Cropping.Right = title.AutoCropDimensions.Right;
                    } 
                    else if ((CropMode)preset.Task.Cropping.CropMode == CropMode.Loose)
                    {
                        this.Task.Cropping.Top = title.LooseCropDimensions.Top;
                        this.Task.Cropping.Bottom = title.LooseCropDimensions.Bottom;
                        this.Task.Cropping.Left = title.LooseCropDimensions.Left;
                        this.Task.Cropping.Right = title.LooseCropDimensions.Right;
                    }
                    else
                    {
                        this.Task.Cropping.Top = 0;
                        this.Task.Cropping.Bottom = 0;
                        this.Task.Cropping.Left = 0;
                        this.Task.Cropping.Right = 0;
                    }

                    this.SelectedCropMode = (CropMode)preset.Task.Cropping.CropMode;
                }
                
                // Set the W/H
                // Set the width, then check the height doesn't breach the max height and correct if necessary.
                this.Task.Width = this.GetModulusValue(this.GetRes((this.sourceResolution.Width - this.CropLeft - this.CropRight), this.MaxWidth));
                this.Task.Height = this.GetModulusValue(this.GetRes((this.sourceResolution.Height - this.CropTop - this.CropBottom), this.MaxHeight));

                // Set Screen Controls
                // "Storage Size: {0}x{1}     Display Size: {2}x{3}      Aspect Ratio: {4}",
                double sourceDisplayWidth = (double)title.Resolution.Width * title.ParVal.Width / title.ParVal.Height;
                this.SourceInfo = string.Format(
                    Resources.PictureSettingsViewModel_SourceInfo,
                    title.Resolution.Width,
                    title.Resolution.Height,
                    sourceDisplayWidth,
                    title.Resolution.Height,
                    HandBrakePictureHelpers.GetNiceDisplayAspect(sourceDisplayWidth, title.Resolution.Height)); 

                // Force a re-calc. This will handle MaxWidth / Height corrections.
                this.RecalculatePictureSettingsProperties(ChangedPictureField.Width);
            }

            this.NotifyOfPropertyChange(() => this.Task);
        }

        public bool MatchesPreset(Preset preset)
        {
            if ((AnamorphicMode)preset.Task.Anamorphic != this.SelectedAnamorphicMode)
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

            if ((CropMode)preset.Task.Cropping.CropMode != this.SelectedCropMode)
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

        public void OpenPreviewWindow()
        {
            if (!string.IsNullOrEmpty(this.Task.Source) && !this.StaticPreviewViewModel.IsOpen)
            {
                this.StaticPreviewViewModel.IsOpen = true;
                this.StaticPreviewViewModel.UpdatePreviewFrame(this.currentTitle, this.Task, this.scannedSource);
                this.windowManager.ShowWindow<StaticPreviewView>(this.StaticPreviewViewModel);
            }
            else if (this.StaticPreviewViewModel.IsOpen)
            {
                Window window = Application.Current.Windows.Cast<Window>().FirstOrDefault(x => x.GetType() == typeof(StaticPreviewView));
                window?.Focus();
            }
        }

        protected virtual void OnTabStatusChanged(TabStatusEventArgs e)
        {
            this.TabStatusChanged?.Invoke(this, e);
        }

        protected virtual void OnFilterChanged(TabStatusEventArgs e)
        {
            RecalculatePictureSettingsProperties(ChangedPictureField.Padding);

            this.TabStatusChanged?.Invoke(this, e);
        }

        protected virtual void OnFlipRotateChanged(FlipRotationCommand e)
        {
            this.HandleRotationFlipChange(e);
            this.TabStatusChanged?.Invoke(this, new TabStatusEventArgs(null));
        }

        private void Init()
        {
            this.Task.KeepDisplayAspect = true;

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
                ParW = this.SelectedAnamorphicMode == AnamorphicMode.None ? 1 : this.ParWidth,
                ParH = this.SelectedAnamorphicMode == AnamorphicMode.None ? 1 : this.ParHeight,
                MaxWidth = this.MaxWidth.HasValue ? this.MaxWidth.Value : 0,
                MaxHeight = this.MaxHeight.HasValue ? this.MaxHeight.Value : 0,
                KeepDisplayAspect = this.MaintainAspectRatio,
                AnamorphicMode = (Anamorphic)this.SelectedAnamorphicMode,
                Crop = new Cropping(this.CropTop, this.CropBottom, this.CropLeft, this.CropRight, (int)CropMode.Custom),
                Pad = new Padding(this.PaddingFilter.Top, this.PaddingFilter.Bottom, this.PaddingFilter.Left, this.PaddingFilter.Right),
                RotateAngle = this.RotateFlipFilter.SelectedRotation,
                Hflip = this.RotateFlipFilter.FlipVideo ? 1 : 0,
                DarWidth = this.DisplayWidth,
                DarHeight = this.DisplayHeight
            };

            if (this.SelectedAnamorphicMode == AnamorphicMode.Custom)
            {
                if (changedField == ChangedPictureField.DisplayWidth)
                {
                    var displayWidth = this.DisplayWidth;
                    job.ParW = (int)displayWidth; // num
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
        private void RecalculatePictureSettingsProperties(ChangedPictureField changedField)
        {
            // Sanity Check
            if (this.currentTitle == null)
            {
                return;
            }

            // Step 1, Update what controls are visible.
            this.UpdateVisibleControls();

            // Step 2, Set sensible defaults
            if (changedField == ChangedPictureField.Anamorphic && (this.SelectedAnamorphicMode == AnamorphicMode.None))
            {
                this.Task.Width = this.sourceResolution.Width > this.MaxWidth
                                      ? this.MaxWidth
                                      : this.sourceResolution.Width;
                this.Task.KeepDisplayAspect = true;
            }

            // Choose which setting to keep.
            HandBrakePictureHelpers.KeepSetting setting = 0;
            switch (changedField)
            {
                case ChangedPictureField.Width:
                    setting = HandBrakePictureHelpers.KeepSetting.HB_KEEP_WIDTH;
                    break;
                case ChangedPictureField.Height:
                    setting = HandBrakePictureHelpers.KeepSetting.HB_KEEP_HEIGHT;
                    break;
                case ChangedPictureField.DisplayWidth:
                    setting = HandBrakePictureHelpers.KeepSetting.HB_KEEP_DISPLAY_WIDTH;
                    break;
                case ChangedPictureField.ParW:
                case ChangedPictureField.ParH:
                    setting = HandBrakePictureHelpers.KeepSetting.HB_KEEP_HEIGHT;
                    break;
                case ChangedPictureField.Padding:
                    setting = HandBrakePictureHelpers.KeepSetting.HB_KEEP_PAD;
                    break;
            }

            if (this.MaintainAspectRatio)
            {
                setting |= HandBrakePictureHelpers.KeepSetting.HB_KEEP_DISPLAY_ASPECT;
            }

            // Step 2, For the changed field, call hb_set_anamorphic_size and process the results.
            HandBrakePictureHelpers.FlagsSetting flag = 0;
            if (this.AllowUpscaling)
            {
                flag = HandBrakePictureHelpers.FlagsSetting.HB_GEO_SCALE_UP;
            }

            if (this.OptimalSize)
            {
                flag |= HandBrakePictureHelpers.FlagsSetting.HB_GEO_SCALE_BEST;
            }

            AnamorphicResult result = HandBrakePictureHelpers.GetAnamorphicSize(this.GetPictureSettings(changedField), this.GetPictureTitleInfo(), setting, flag);
            
            this.Task.Width = result.OutputWidth;
            this.Task.Height = result.OutputHeight;
            this.Task.PixelAspectX = result.OutputParWidth;
            this.Task.PixelAspectY = result.OutputParHeight;

            if (this.Task.PixelAspectX == 0)
            {
                this.Task.PixelAspectX = 1;
            }

            if (this.Task.PixelAspectY == 0)
            {
                this.Task.PixelAspectY = 1;
            }
            
            this.ApplyPad(this.PaddingFilter.Mode, result); // Update for display purposes. 
            double dispWidth = Math.Round((double)result.OutputWidth * result.OutputParWidth / result.OutputParHeight, 0);
            this.Task.DisplayWidth = dispWidth;
            this.DisplayHeight = result.OutputHeight;
            

            // Step 3, Set the display width label to indicate the output.
            this.DisplaySize = this.sourceResolution == null || this.sourceResolution.IsEmpty
                           ? string.Empty
                           : string.Format(Resources.PictureSettingsViewModel_StorageDisplayLabel, result.OutputWidth, result.OutputHeight);
            this.NotifyOfPropertyChange(() => this.DisplaySize);

            this.OutputAspect = string.Format(Resources.PictureSettingsViewModel_AspectRatioLabel, HandBrakePictureHelpers.GetNiceDisplayAspect(dispWidth, result.OutputHeight));
            this.NotifyOfPropertyChange(() => this.OutputAspect);


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
            this.NotifyOfPropertyChange(() => this.MaintainAspectRatio);

            // Step 5, Update the Preview
            if (delayedPreviewprocessor != null && this.Task != null && this.StaticPreviewViewModel != null && this.StaticPreviewViewModel.IsOpen)
            {
                delayedPreviewprocessor.PerformTask(() => this.StaticPreviewViewModel.UpdatePreviewFrame(this.currentTitle, this.Task, this.scannedSource), 800);
            }

            this.OnTabStatusChanged(new TabStatusEventArgs("picture", ChangedOption.Dimensions));
        }

        private void ApplyPad(PaddingMode mode, AnamorphicResult result)
        {
            if (mode == PaddingMode.Custom)
            {
                result.OutputWidth = result.OutputWidth + this.PaddingFilter.Left + this.PaddingFilter.Right;
                result.OutputHeight = result.OutputHeight + this.PaddingFilter.Top + this.PaddingFilter.Bottom;
            }
        }

        private void UpdateVisibleControls()
        {
            this.WidthControlEnabled = true;
            this.HeightControlEnabled = true;

            if (OptimalSize)
            {
                this.WidthControlEnabled = false;
                this.HeightControlEnabled = false;
            }
        }

        private int GetModulusValue(double value)
        {
            double remainder = value % 2;

            if (remainder.Equals(0.0d))
            {
                return (int)Math.Abs(value);
            }

            double result = remainder >= 1
                       ? value + (2 - remainder)
                       : value - remainder;

            return (int)Math.Abs(result);
        }

        private int GetRes(int value, int? max)
        {
            return max.HasValue ? (value > max.Value ? max.Value : value) : value;
        }

        private void SetSelectedPictureSettingsResLimitMode()
        {
            if (this.MaxWidth == null && this.MaxHeight == null)
            {
                this.SelectedPictureSettingsResLimitMode = PictureSettingsResLimitModes.None;
                return;
            }

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

        private void HandleRotationFlipChange(FlipRotationCommand command)
        {
            if (this.currentTitle == null)
            {
                return;
            }

            PictureSettingsJob job = this.GetPictureSettings(command.ChangedField);
            job.PreviousHflip = command.PreviousHflip;
            job.PreviousRotation = command.PreviousRotation;
            RotateResult result = HandBrakePictureHelpers.RotateGeometry(job);

            this.Task.Cropping.Top = result.CropTop;
            this.Task.Cropping.Bottom = result.CropBottom;
            this.Task.Cropping.Left = result.CropLeft;
            this.Task.Cropping.Right = result.CropRight;
            this.NotifyOfPropertyChange(() => this.CropTop);
            this.NotifyOfPropertyChange(() => this.CropBottom);
            this.NotifyOfPropertyChange(() => this.CropLeft);
            this.NotifyOfPropertyChange(() => this.CropRight);

            this.PaddingFilter.SetRotationValues(result.PadTop, result.PadBottom, result.PadLeft, result.PadRight);

            this.Task.Width = result.Width;
            this.Task.Height = result.Height;
            this.Task.PixelAspectX = result.ParNum;
            this.Task.PixelAspectY = result.ParDen;

            this.NotifyOfPropertyChange(() => this.Width);
            this.NotifyOfPropertyChange(() => this.Height);
            this.NotifyOfPropertyChange(() => this.ParWidth);
            this.NotifyOfPropertyChange(() => this.ParHeight);

            RecalculatePictureSettingsProperties(ChangedPictureField.Rotate);
        }
    }
}