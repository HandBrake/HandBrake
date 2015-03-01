// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AddPresetViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Add Preset View Model
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels
{
    using System.Collections.Generic;
    using System.Windows;

    using HandBrake.ApplicationServices.Services.Encode.Model;
    using HandBrake.ApplicationServices.Services.Scan.Model;
    using HandBrake.ApplicationServices.Utilities;
    using HandBrake.ApplicationServices.Interop.Model.Encoding;

    using HandBrakeWPF.Model.Audio;
    using HandBrakeWPF.Model.Subtitles;
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Presets;
    using HandBrakeWPF.Services.Presets.Interfaces;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.ViewModels.Interfaces;

    using PresetPictureSettingsMode = HandBrakeWPF.Model.Picture.PresetPictureSettingsMode;

    /// <summary>
    /// The Add Preset View Model
    /// </summary>
    public class AddPresetViewModel : ViewModelBase, IAddPresetViewModel
    {
        /// <summary>
        /// Backing field for the Preset Service
        /// </summary>
        private readonly IPresetService presetService;

        /// <summary>
        /// Backing field for the error service
        /// </summary>
        private readonly IErrorService errorService;

        /// <summary>
        /// Backing fields for Selected Picture settings mode.
        /// </summary>
        private PresetPictureSettingsMode selectedPictureSettingMode;

        /// <summary>
        /// Backging field for show custom inputs
        /// </summary>
        private bool showCustomInputs;

        /// <summary>
        /// The source.
        /// </summary>
        private Title selectedTitle;

        /// <summary>
        /// Initializes a new instance of the <see cref="AddPresetViewModel"/> class.
        /// </summary>
        /// <param name="presetService">
        /// The Preset Service
        /// </param>
        /// <param name="errorService">
        /// The Error Service
        /// </param>
        public AddPresetViewModel(IPresetService presetService, IErrorService errorService)
        {
            this.presetService = presetService;
            this.errorService = errorService;
            this.Title = "Add Preset";
            this.Preset = new Preset { IsBuildIn = false, IsDefault = false, Category = PresetService.UserPresetCatgoryName, UsePictureFilters = true };
            this.PictureSettingsModes = EnumHelper<PresetPictureSettingsMode>.GetEnumList();
        }

        /// <summary>
        /// Gets the Preset
        /// </summary>
        public Preset Preset { get; private set; }

        /// <summary>
        /// Gets or sets PictureSettingsModes.
        /// </summary>
        public IEnumerable<PresetPictureSettingsMode> PictureSettingsModes { get; set; }

        /// <summary>
        /// Gets or sets CustomWidth.
        /// </summary>
        public int? CustomWidth { get; set; }

        /// <summary>
        /// Gets or sets CustomHeight.
        /// </summary>
        public int? CustomHeight { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether ShowCustomInputs.
        /// </summary>
        public bool ShowCustomInputs
        {
            get
            {
                return this.showCustomInputs;
            }
            set
            {
                this.showCustomInputs = value;
                this.NotifyOfPropertyChange(() => this.ShowCustomInputs);
            }
        }

        /// <summary>
        /// Gets or sets SelectedPictureSettingMode.
        /// </summary>
        public PresetPictureSettingsMode SelectedPictureSettingMode
        {
            get
            {
                return this.selectedPictureSettingMode;
            }
            set
            {
                this.selectedPictureSettingMode = value;
                this.ShowCustomInputs = value == PresetPictureSettingsMode.Custom;
            }
        }

        /// <summary>
        /// Prepare the Preset window to create a Preset Object later.
        /// </summary>
        /// <param name="task">
        /// The Encode Task.
        /// </param>
        /// <param name="title">
        /// The title.
        /// </param>
        /// <param name="audioBehaviours">
        /// The audio Behaviours.
        /// </param>
        /// <param name="subtitleBehaviours">
        /// The subtitle Behaviours.
        /// </param>
        public void Setup(EncodeTask task, Title title, AudioBehaviours audioBehaviours, SubtitleBehaviours subtitleBehaviours)
        {
            this.Preset.Task = new EncodeTask(task);
            this.Preset.AudioTrackBehaviours = audioBehaviours.Clone();
            this.Preset.SubtitleTrackBehaviours = subtitleBehaviours.Clone();
            this.selectedTitle = title;

            switch (task.Anamorphic)
            {
                default:
                    this.SelectedPictureSettingMode = PresetPictureSettingsMode.Custom;
                    if (title != null && title.Resolution != null)
                    {
                        this.CustomWidth = title.Resolution.Width;
                        this.CustomHeight = title.Resolution.Height;
                    }
                    break;
                case Anamorphic.Strict:
                    this.SelectedPictureSettingMode = PresetPictureSettingsMode.SourceMaximum;
                    break;
            }
        }

        /// <summary>
        /// Add a Preset
        /// </summary>
        public void Add()
        {
            if (string.IsNullOrEmpty(this.Preset.Name))
            {
                this.errorService.ShowMessageBox("A Preset must have a Name. Please fill out the Preset Name field.", Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            if (this.presetService.CheckIfPresetExists(this.Preset.Name))
            {
                MessageBoxResult result = this.errorService.ShowMessageBox("A Preset with this name already exists. Would you like to overwrite it?", Resources.Error, MessageBoxButton.YesNo, MessageBoxImage.Error);
                if (result == MessageBoxResult.No)
                {
                    return;
                }
            }

            if (this.SelectedPictureSettingMode == PresetPictureSettingsMode.SourceMaximum && this.selectedTitle == null)
            {
                this.errorService.ShowMessageBox("You must first scan a source to use the 'Source Maximum' Option.", Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            if (this.CustomWidth == null && this.CustomHeight == null && this.SelectedPictureSettingMode == PresetPictureSettingsMode.Custom)
            {
                this.errorService.ShowMessageBox("The Custom Width or Height fields must be filled in for the 'Custom' option.", Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            this.Preset.UsePictureFilters = this.Preset.UsePictureFilters;
            this.Preset.PictureSettingsMode = this.SelectedPictureSettingMode;

            // Setting W, H, MW and MH
            if (this.SelectedPictureSettingMode == PresetPictureSettingsMode.None)
            {
                this.Preset.Task.MaxHeight = null;
                this.Preset.Task.MaxWidth = null;
            }

            if (this.SelectedPictureSettingMode == PresetPictureSettingsMode.Custom)
            {
                this.Preset.Task.MaxWidth = this.CustomWidth;
                this.Preset.Task.MaxHeight = this.CustomHeight;
                this.Preset.Task.Width = null;
                this.Preset.Task.Height = null;
            }

            if (this.SelectedPictureSettingMode == PresetPictureSettingsMode.SourceMaximum)
            {
                this.Preset.Task.MaxWidth = selectedTitle.Resolution.Width;
                this.Preset.Task.MaxHeight = selectedTitle.Resolution.Height;
            }

            // Add the Preset
            bool added = this.presetService.Add(this.Preset);
            if (!added)
            {
                this.errorService.ShowMessageBox("Unable to add preset", "Unknown Error", MessageBoxButton.OK,
                                                 MessageBoxImage.Error);
            }
            else
            {
                this.Close();
            }
        }

        /// <summary>
        /// Cancel adding a preset
        /// </summary>
        public void Cancel()
        {
            this.Close();
        }

        /// <summary>
        /// Close this window.
        /// </summary>
        public void Close()
        {
            this.TryClose();
        }
    }
}
