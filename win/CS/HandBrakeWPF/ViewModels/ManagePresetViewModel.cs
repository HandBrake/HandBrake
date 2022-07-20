// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ManagePresetViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
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
    using HandBrakeWPF.ViewModels.Interfaces;

    public class ManagePresetViewModel : ViewModelBase, IManagePresetViewModel
    {
        private readonly IPresetService presetService;
        private readonly IErrorService errorService;

        private readonly IWindowManager windowManager;
        private Preset existingPreset;
        private PictureSettingsResLimitModes selectedPictureSettingsResLimitMode;
        private string originalPresetName;

        public ManagePresetViewModel(IPresetService presetService, IErrorService errorService, IWindowManager windowManager)
        {
            this.presetService = presetService;
            this.errorService = errorService;
            this.windowManager = windowManager;
            this.Title = Resources.MainView_PresetManage;
            this.Preset = new Preset { IsBuildIn = false, IsDefault = false, Category = PresetService.UserPresetCategoryName };
        }

        public Preset Preset { get; private set; }

        public IEnumerable<IPresetObject> PresetsCategories { get; set; }

        public List<PresetDisplayCategory> UserPresetCategories { get; set; }

        public PresetDisplayCategory SelectedUserPresetCategory
        {
            get
            {
                if (this.Preset != null && this.PresetsCategories != null)
                {
                    return this.PresetsCategories.FirstOrDefault(s => s.Category == this.Preset.Category) as PresetDisplayCategory;
                }

                return null;
            }

            set
            {
                if (this.Preset != null && value != null && value.Category != this.Preset.Category)
                {
                    this.Preset.Category = value.Category;
                }
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

        public int? CustomWidth
        {
            get => this.Preset?.Task.MaxWidth ?? null;
            set
            {
                if (value == this.Preset.Task.MaxWidth)
                {
                    return;
                }

                this.Preset.Task.MaxWidth = value;
                this.NotifyOfPropertyChange(() => this.CustomWidth);
            }
        }

        public int? CustomHeight
        {
            get => this.Preset?.Task.MaxHeight ?? null;
            set
            {
                if (value == this.Preset.Task.MaxHeight)
                {
                    return;
                }

                this.Preset.Task.MaxHeight = value;
                this.NotifyOfPropertyChange(() => this.CustomHeight);
            }
        }

        public bool IsCustomMaxRes { get; private set; }

        public void Setup(Preset presetToEdit)
        {
            this.existingPreset = presetToEdit;
            this.originalPresetName = presetToEdit.Name;

            this.Preset = new Preset(presetToEdit); // Clone. We will not touch the existing object.
            
            this.UserPresetCategories = presetService.GetPresetCategories(true).ToList();
            this.NotifyOfPropertyChange(() => this.UserPresetCategories);
            this.PresetsCategories = this.presetService.Presets;
            this.NotifyOfPropertyChange(() => this.PresetsCategories);

            this.SetSelectedPictureSettingsResLimitMode();
        }

        public void EditAudioDefaults()
        {
            IAudioDefaultsViewModel audioDefaultsViewModel = new AudioDefaultsViewModel(this.windowManager);
            audioDefaultsViewModel.Setup(this.Preset.AudioTrackBehaviours, this.Preset.Task.OutputFormat);
            audioDefaultsViewModel.ShowWindow();

            if (audioDefaultsViewModel.IsApplied)
            {
                this.Preset.AudioTrackBehaviours = new AudioBehaviours(audioDefaultsViewModel.AudioBehaviours);
            }
        }

        public void EditSubtitleDefaults()
        {
            ISubtitlesDefaultsViewModel subtitlesDefaultsViewModel = new SubtitlesDefaultsViewModel(windowManager);
            subtitlesDefaultsViewModel.SetupPreset(this.Preset);
            subtitlesDefaultsViewModel.ShowWindow();

            if (subtitlesDefaultsViewModel.IsApplied)
            {
                this.Preset.SubtitleTrackBehaviours = new SubtitleBehaviours(subtitlesDefaultsViewModel.SubtitleBehaviours);
            }
        }

        public void Save()
        {
            if (!this.Preset.IsBuildIn)
            {
                if (string.IsNullOrEmpty(this.Preset.Name))
                {
                    this.errorService.ShowMessageBox(
                        Resources.AddPresetViewModel_PresetMustProvideName,
                        Resources.Error,
                        MessageBoxButton.OK,
                        MessageBoxImage.Error);
                    return;
                }

                if (originalPresetName != this.Preset.Name && this.presetService.CheckIfPresetExists(this.Preset.Name))
                {
                    MessageBoxResult result =
                        this.errorService.ShowMessageBox(
                            Resources.AddPresetViewModel_PresetWithSameNameOverwriteWarning,
                            Resources.Error,
                            MessageBoxButton.YesNo,
                            MessageBoxImage.Error);
                    if (result == MessageBoxResult.No)
                    {
                        return;
                    }
                }

                // Save the Preset
                this.presetService.Replace(this.existingPreset.Name, this.Preset);
            }

            this.Close();
        }

        public void Cancel()
        {
            this.Close();
        }

        public void Close()
        {
            this.TryClose();
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
