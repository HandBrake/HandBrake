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
    using System.ComponentModel;
    using System.Linq;
    using System.Windows;

    using HandBrake.App.Core.Utilities;

    using HandBrakeWPF.Model.Audio;
    using HandBrakeWPF.Model.Picture;
    using HandBrakeWPF.Model.Subtitles;
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Presets;
    using HandBrakeWPF.Services.Presets.Interfaces;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.Services.Scan.Model;
    using HandBrakeWPF.ViewModels.Interfaces;

    using EncodeTask = HandBrakeWPF.Services.Encode.Model.EncodeTask;

    public class AddPresetViewModel : ViewModelBase, IAddPresetViewModel
    {
        private readonly IPresetService presetService;
        private readonly IErrorService errorService;
        private readonly IWindowManager windowManager;
        private readonly PresetDisplayCategory addNewCategory = new PresetDisplayCategory(Resources.AddPresetView_AddNewCategory, true, null);

        private bool showCustomInputs;
        private IAudioDefaultsViewModel audioDefaultsViewModel;
        private ISubtitlesDefaultsViewModel subtitlesDefaultsViewModel;
        private PresetDisplayCategory selectedPresetCategory;
        private bool canAddNewPresetCategory;

        private PictureSettingsResLimitModes selectedPictureSettingsResLimitMode;

        public AddPresetViewModel(IPresetService presetService, IErrorService errorService, IWindowManager windowManager)
        {
            this.presetService = presetService;
            this.errorService = errorService;
            this.windowManager = windowManager;
            this.Title = Resources.AddPresetView_AddPreset;
            this.Preset = new Preset { IsBuildIn = false, IsDefault = false, Category = PresetService.UserPresetCategoryName };
            this.PresetCategories = presetService.GetPresetCategories(true).Union(new List<PresetDisplayCategory> { addNewCategory }).ToList();
            this.SelectedPresetCategory = this.PresetCategories.FirstOrDefault(n => n.Category == PresetService.UserPresetCategoryName);

            this.CustomHeight = 0;
            this.CustomWidth = 0;
        }

        public Preset Preset { get; private set; }

        public string PresetName => this.Preset.Name;

        public bool ShowCustomInputs
        {
            get => this.showCustomInputs;
            set
            {
                this.showCustomInputs = value;
                this.NotifyOfPropertyChange(() => this.ShowCustomInputs);
            }
        }

        public List<PresetDisplayCategory> PresetCategories { get; set; }

        public PresetDisplayCategory SelectedPresetCategory
        {
            get => this.selectedPresetCategory;
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
                    this.PresetCategory = PresetService.UserPresetCategoryName;
                }
            }
        }

        public string PresetCategory
        {
            get => this.Preset.Category;
            set
            {
                this.Preset.Category = value;
                this.NotifyOfPropertyChange(() => this.PresetCategory);
            }
        }

        public bool CanAddNewPresetCategory
        {
            get => this.canAddNewPresetCategory;
            set
            {
                if (value == this.canAddNewPresetCategory)
                {
                    return;
                }

                this.canAddNewPresetCategory = value;
                this.NotifyOfPropertyChange(() => this.CanAddNewPresetCategory);
            }
        }

        public BindingList<PictureSettingsResLimitModes> ResolutionLimitModes => new BindingList<PictureSettingsResLimitModes>
                                                                                 {
                                                                                     PictureSettingsResLimitModes.None,
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
                if (limit != null)
                {
                    this.CustomWidth = limit.Width;
                    this.CustomHeight = limit.Height;
                    this.NotifyOfPropertyChange(() => this.CustomWidth);
                    this.NotifyOfPropertyChange(() => this.CustomHeight);
                }
                
                if (value == PictureSettingsResLimitModes.None)
                {
                    this.CustomWidth = null;
                    this.CustomHeight = null;
                }
            }
        }

        public bool IsCustomMaxRes { get; private set; }

        public int? CustomWidth { get; set; }

        public int? CustomHeight { get; set; }

        public void Setup(EncodeTask task, Title title, AudioBehaviours audioBehaviours, SubtitleBehaviours subtitleBehaviours)
        {
            this.Preset = new Preset { IsBuildIn = false, IsDefault = false, Category = PresetService.UserPresetCategoryName };
            this.Preset.Task = new EncodeTask(task);
            this.Preset.AudioTrackBehaviours = new AudioBehaviours(audioBehaviours); 
            this.Preset.SubtitleTrackBehaviours = new SubtitleBehaviours(subtitleBehaviours);

            this.audioDefaultsViewModel = new AudioDefaultsViewModel(this.windowManager);
            this.audioDefaultsViewModel.Setup(audioBehaviours, task.OutputFormat);

            this.subtitlesDefaultsViewModel = new SubtitlesDefaultsViewModel(this.windowManager);
            this.subtitlesDefaultsViewModel.SetupPreset(subtitleBehaviours);

            // Resolution Limits
            this.CustomWidth = task.MaxWidth;
            this.CustomHeight = task.MaxHeight;
            this.NotifyOfPropertyChange(() => this.CustomWidth);
            this.NotifyOfPropertyChange(() => this.CustomHeight);

            this.SetSelectedPictureSettingsResLimitMode();
        }

        public void Add()
        {
            if (string.IsNullOrEmpty(this.Preset.Name))
            {
                this.errorService.ShowMessageBox(Resources.AddPresetViewModel_PresetMustProvideName, Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            if (this.presetService.CheckIfPresetExists(this.Preset.Name))
            {
                Preset currentPreset = this.presetService.GetPreset(this.Preset.Name);
                if (currentPreset != null && currentPreset.IsBuildIn)
                {
                    this.errorService.ShowMessageBox(Resources.Main_NoUpdateOfBuiltInPresets, Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
                    return;
                }

                MessageBoxResult result = this.errorService.ShowMessageBox(Resources.AddPresetViewModel_PresetWithSameNameOverwriteWarning, Resources.Question, MessageBoxButton.YesNo, MessageBoxImage.Question);
                if (result == MessageBoxResult.No)
                {
                    return;
                }
            }

            if (this.SelectedPictureSettingsResLimitMode != PictureSettingsResLimitModes.None)
            {
                if (this.CustomWidth != null)
                {
                    this.Preset.Task.MaxWidth = this.CustomWidth;
                }

                if (this.CustomHeight != null)
                {
                    this.Preset.Task.MaxHeight = this.CustomHeight;
                }
            }

            // Add the Preset
            bool added = this.presetService.Add(this.Preset);
            if (!added)
            {
                this.errorService.ShowMessageBox(
                    Resources.AddPresetViewModel_UnableToAddPreset,
                    Resources.UnknownError,
                    MessageBoxButton.OK,
                    MessageBoxImage.Error);
            }
            else
            {
                this.Close();
            }
        }

        public void EditAudioDefaults()
        {
            this.audioDefaultsViewModel.ShowWindow();

            if (audioDefaultsViewModel.IsApplied)
            {
                this.Preset.AudioTrackBehaviours = new AudioBehaviours(this.audioDefaultsViewModel.AudioBehaviours);
            }
        }

        public void EditSubtitleDefaults()
        {
            this.subtitlesDefaultsViewModel.ShowWindow();

            if (subtitlesDefaultsViewModel.IsApplied)
            {
                this.Preset.SubtitleTrackBehaviours = new SubtitleBehaviours(this.subtitlesDefaultsViewModel.SubtitleBehaviours);
            }
        }

        public void Cancel()
        {
            this.TryClose();
        }

        private void Close()
        {
            this.TryClose(true);
        }

        private void SetSelectedPictureSettingsResLimitMode()
        {
            // Look for a matching resolution.
            foreach (PictureSettingsResLimitModes limit in EnumHelper<PictureSettingsResLimitModes>.GetEnumList())
            {
                ResLimit resLimit = EnumHelper<PictureSettingsResLimitModes>.GetAttribute<ResLimit, PictureSettingsResLimitModes>(limit);
                if (resLimit != null)
                {
                    if (resLimit.Width == this.CustomWidth && resLimit.Height == this.CustomHeight)
                    {
                        this.SelectedPictureSettingsResLimitMode = limit;
                        return;
                    }
                }
            }

            if (this.CustomWidth.HasValue || this.CustomHeight.HasValue)
            {
                this.SelectedPictureSettingsResLimitMode = PictureSettingsResLimitModes.Custom;
            }
            else
            {
                this.SelectedPictureSettingsResLimitMode = PictureSettingsResLimitModes.None;
            }
        }
    }
}
