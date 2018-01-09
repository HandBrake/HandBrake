﻿// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AddPresetViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Add Preset View Model
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ViewModels
{
    using System.Collections.Generic;
    using System.Linq;
    using HandBrake;
    using HandBrake.CoreLibrary.Interop.Model.Encoding;
    using HandBrake.Model.Prompts;
    using HandBrake.Properties;
    using HandBrake.Services.Interfaces;
    using HandBrake.Model.Audio;
    using HandBrake.Model.Subtitles;
    using HandBrake.Services.Presets;
    using HandBrake.Services.Presets.Interfaces;
    using HandBrake.Services.Presets.Model;
    using HandBrake.Services.Scan.Model;
    using HandBrake.Utilities;
    using HandBrake.ViewModels.Interfaces;
    using EncodeTask = HandBrake.Services.Encode.Model.EncodeTask;
    using PresetPictureSettingsMode = HandBrake.Model.Picture.PresetPictureSettingsMode;

    /// <summary>
    /// The Add Preset View Model
    /// </summary>
    public class AddPresetViewModel : ViewModelBase, IAddPresetViewModel
    {
        private readonly IPresetService presetService;
        private readonly IErrorService errorService;
        private readonly ViewManagerBase viewManager;

        private PresetPictureSettingsMode selectedPictureSettingMode;
        private bool showCustomInputs;
        private Title selectedTitle;

        private IAudioDefaultsViewModel audioDefaultsViewModel;
        private ISubtitlesDefaultsViewModel subtitlesDefaultsViewModel;

        private PresetDisplayCategory selectedPresetCategory;
        private readonly PresetDisplayCategory addNewCategory = new PresetDisplayCategory(ResourcesUI.AddPresetView_AddNewCategory, true, null);
        private bool canAddNewPresetCategory;

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
            this.viewManager = HandBrakeServices.Current?.ViewManager;

            this.Title = ResourcesUI.AddPresetView_AddPreset;
            this.Preset = new Preset { IsBuildIn = false, IsDefault = false, Category = PresetService.UserPresetCatgoryName };
            this.PictureSettingsModes = EnumHelper<PresetPictureSettingsMode>.GetEnumList();
            this.PresetCategories = presetService.GetPresetCategories(true).Union(new List<PresetDisplayCategory> { addNewCategory }).ToList();
            this.SelectedPresetCategory = this.PresetCategories.FirstOrDefault(n => n.Category == PresetService.UserPresetCatgoryName);
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

        public List<PresetDisplayCategory> PresetCategories { get; set; }

        public PresetDisplayCategory SelectedPresetCategory
        {
            get
            {
                return this.selectedPresetCategory;
            }
            set
            {
                this.selectedPresetCategory = value;
                this.CanAddNewPresetCategory = Equals(value, this.addNewCategory);

                if (this.selectedPresetCategory != null
                    && !object.Equals(this.selectedPresetCategory, this.addNewCategory))
                {
                    this.PresetCategory = this.selectedPresetCategory.Category;
                }
                else
                {
                    this.PresetCategory = PresetService.UserPresetCatgoryName;
                }
            }
        }

        public string PresetCategory
        {
            get
            {
                return this.Preset.Category;
            }
            set
            {
                this.Preset.Category = value;
                this.NotifyOfPropertyChange(() => this.PresetCategory);
            }
        }

        public bool CanAddNewPresetCategory
        {
            get
            {
                return this.canAddNewPresetCategory;
            }
            set
            {
                if (value == this.canAddNewPresetCategory) return;
                this.canAddNewPresetCategory = value;
                this.NotifyOfPropertyChange();
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

            this.audioDefaultsViewModel = new AudioDefaultsViewModel(this.Preset.Task);
            this.audioDefaultsViewModel.Setup(this.Preset, this.Preset.Task);

            this.subtitlesDefaultsViewModel = new SubtitlesDefaultsViewModel();
            this.subtitlesDefaultsViewModel.SetupLanguages(subtitleBehaviours);

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

                case Anamorphic.Automatic:
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
                this.errorService.ShowMessageBox(Resources.AddPresetViewModel_PresetMustProvideName, Resources.Error, DialogButtonType.OK, DialogType.Error);
                return;
            }

            if (this.presetService.CheckIfPresetExists(this.Preset.Name))
            {
                var result = this.errorService.ShowMessageBox(Resources.AddPresetViewModel_PresetWithSameNameOverwriteWarning, Resources.Error, DialogButtonType.YesNo, DialogType.Error);
                if (result == DialogResult.No)
                {
                    return;
                }
            }

            if (this.SelectedPictureSettingMode == PresetPictureSettingsMode.SourceMaximum && this.selectedTitle == null)
            {
                this.errorService.ShowMessageBox(Resources.AddPresetViewModel_YouMustFirstScanSource, Resources.Error, DialogButtonType.OK, DialogType.Error);
                return;
            }

            if (this.CustomWidth == null && this.CustomHeight == null && this.SelectedPictureSettingMode == PresetPictureSettingsMode.Custom)
            {
                this.errorService.ShowMessageBox(Resources.AddPresetViewModel_CustomWidthHeightFieldsRequired, Resources.Error, DialogButtonType.OK, DialogType.Error);
                return;
            }

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
                this.Preset.Task.MaxHeight = null;
                this.Preset.Task.MaxWidth = null;
            }

            // Add the Preset
            bool added = this.presetService.Add(this.Preset);
            if (!added)
            {
                this.errorService.ShowMessageBox(Resources.AddPresetViewModel_UnableToAddPreset, Resources.UnknownError, DialogButtonType.OK,
                                                 DialogType.Error);
            }
            else
            {
                this.Close();
            }
        }

        /// <summary>
        /// The edit audio defaults.
        /// </summary>
        public void EditAudioDefaults()
        {
            IPopupWindowViewModel popup = new PopupWindowViewModel(this.audioDefaultsViewModel, ResourcesUI.Preset_AudioDefaults_Title, ResourcesUI.Preset_AudioDefaults_SubText);
            if (this.viewManager.ShowDialog(popup) == true)
            {
                this.Preset.AudioTrackBehaviours = this.audioDefaultsViewModel.AudioBehaviours.Clone();
            }
            else
            {
                // Handle other case(s)
            }
        }

        /// <summary>
        /// The edit subtitle defaults.
        /// </summary>
        public void EditSubtitleDefaults()
        {
            IPopupWindowViewModel popup = new PopupWindowViewModel(this.subtitlesDefaultsViewModel, ResourcesUI.Preset_SubtitleDefaults_Title, ResourcesUI.Preset_SubtitleDefaults_SubText);

            if (this.viewManager.ShowDialog(popup) == true)
            {
                this.Preset.SubtitleTrackBehaviours = this.subtitlesDefaultsViewModel.SubtitleBehaviours.Clone();
            }
            else
            {
                // Handle other case(s)
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