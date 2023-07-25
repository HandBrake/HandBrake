// --------------------------------------------------------------------------------------------------------------------
// <copyright file="PresetManagerViewModel.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the PresetManagerViewModel type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Linq;
    using System.Windows;

    using HandBrake.App.Core.Utilities;

    using HandBrakeWPF.Model.Audio;
    using HandBrakeWPF.Model.Picture;
    using HandBrakeWPF.Model.Subtitles;
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Presets.Interfaces;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.Utilities.FileDialogs;
    using HandBrakeWPF.ViewModels.Interfaces;

    public class PresetManagerViewModel : ViewModelBase, IPresetManagerViewModel
    {
        private readonly IPresetService presetService;
        private readonly IErrorService errorService;
        private readonly IWindowManager windowManager;

        private IPresetObject selectedPresetCategory;
        private Preset selectedPreset;
        private PictureSettingsResLimitModes selectedPictureSettingsResLimitMode;
        private Action<Preset> mainWindowCallback;
        private string presetNameKey;

        public PresetManagerViewModel(IPresetService presetService, IErrorService errorService, IWindowManager windowManager)
        {
            this.presetService = presetService;
            this.errorService = errorService;
            this.windowManager = windowManager;
            this.Title = Resources.PresetManger_Title;
        }

        public bool IsOpen { get; set; }

        public IEnumerable<IPresetObject> PresetsCategories { get; set; }

        public List<PresetDisplayCategory> UserPresetCategories { get; set; }

        public PresetDisplayCategory SelectedUserPresetCategory
        {
            get
            {
                if (this.selectedPreset != null && this.PresetsCategories != null)
                {
                    return this.PresetsCategories.FirstOrDefault(s => s.Category == this.selectedPreset.Category) as PresetDisplayCategory;
                }

                return null;
            }

            set
            {
                if (this.selectedPreset != null && value != null && value.Category != this.selectedPreset.Category)
                {
                    this.presetService.ChangePresetCategory(this.selectedPreset.Name, value.Category);
                }
            }
        }

        public IPresetObject SelectedPresetCategory
        {
            get => this.selectedPresetCategory;
            set
            {
                if (!object.Equals(this.selectedPresetCategory, value))
                {
                    this.selectedPresetCategory = value;
                    this.NotifyOfPropertyChange(() => this.SelectedPresetCategory);

                    this.selectedPreset = null;
                    this.NotifyOfPropertyChange(() => this.SelectedPreset);
                }
            }
        }

        public Preset SelectedPreset
        {
            get => this.selectedPreset;

            set
            {
                if (!object.Equals(this.selectedPreset, value))
                {
                    this.selectedPreset = value;
                    this.NotifyOfPropertyChange(() => this.SelectedPreset);

                    if (value != null)
                    {
                        this.PresetName = value?.Name;
                        this.presetNameKey = value?.Name;
                        this.CustomWidth = value.Task.MaxWidth;
                        this.CustomHeight = value.Task.MaxHeight;
                        this.SetSelectedPictureSettingsResLimitMode();
                    }
                    else
                    {
                        this.PresetName = null;
                        this.presetNameKey = null;
                        this.selectedPresetCategory = null;
                        this.NotifyOfPropertyChange(() => this.SelectedPresetCategory);
                    }
                }

                this.NotifyOfPropertyChange(() => this.PresetName);
                this.NotifyOfPropertyChange(() => this.IsBuildIn);
                this.NotifyOfPropertyChange(() => this.SelectedUserPresetCategory);
                this.NotifyOfPropertyChange(() => this.IsPresetSelected);
                this.NotifyOfPropertyChange(() => this.UserPresetCategories);
            }
        }

        public bool IsBuildIn => this.SelectedPreset?.IsBuildIn ?? true;

        public bool IsPresetSelected => this.SelectedPreset != null;

        public BindingList<PictureSettingsResLimitModes> ResolutionLimitModes => new BindingList<PictureSettingsResLimitModes>(EnumHelper<PictureSettingsResLimitModes>.GetEnumList().ToList());

        public bool IsPresetNameChanged => this.PresetName != this.presetNameKey;

        public string PresetName { get; set; }

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
            get => this.selectedPreset?.Task.MaxWidth ?? null;
            set
            {
                if (value == this.selectedPreset.Task.MaxWidth)
                {
                    return;
                }

                this.selectedPreset.Task.MaxWidth = value;
                this.NotifyOfPropertyChange(() => this.CustomWidth);
            }
        }

        public int? CustomHeight
        {
            get => this.selectedPreset?.Task.MaxHeight ?? null;
            set
            {
                if (value == this.selectedPreset.Task.MaxHeight)
                {
                    return;
                }

                this.selectedPreset.Task.MaxHeight = value;
                this.NotifyOfPropertyChange(() => this.CustomHeight);
            }
        }

        public bool IsCustomMaxRes { get; private set; }

        public void SetupWindow(Action<Preset> mainwindowCallback)
        {
            this.mainWindowCallback = mainwindowCallback;
            this.PresetsCategories = this.presetService.Presets;
            this.NotifyOfPropertyChange(() => this.PresetsCategories);
            this.presetService.LoadCategoryStates();
            this.UserPresetCategories = presetService.GetPresetCategories(true).ToList(); // .Union(new List<PresetDisplayCategory> { addNewCategory }).ToList();
            this.presetService.PresetCollectionChanged += this.PresetService_PresetCollectionChanged;
        }

        public void RenamePreset()
        {
            if (this.SelectedPreset != null)
            {
                this.SelectedPreset.Name = this.PresetName;

                this.presetService.Update(this.presetNameKey, this.SelectedPreset);

                this.PresetsCategories = null;
                this.NotifyOfPropertyChange(() => this.PresetsCategories);

                this.PresetsCategories = this.presetService.Presets;
                this.NotifyOfPropertyChange(() => this.PresetsCategories);

                this.SelectedPreset = this.presetService.GetPresetByName(this.SelectedPreset.Name);

                this.NotifyOfPropertyChange(() => this.IsPresetNameChanged);

                if (this.mainWindowCallback != null)
                {
                    mainWindowCallback(this.selectedPreset);
                }
            }
        }

        public void DeletePreset()
        {
            if (this.selectedPreset != null)
            {
                if (this.selectedPreset.IsDefault)
                {
                    this.errorService.ShowMessageBox(
                      Resources.MainViewModel_CanNotDeleteDefaultPreset,
                      Resources.Warning,
                      MessageBoxButton.OK,
                      MessageBoxImage.Information);

                    return;
                }

                MessageBoxResult result =
                this.errorService.ShowMessageBox(
                   Resources.MainViewModel_PresetRemove_AreYouSure + this.selectedPreset.Name + " ?",
                   Resources.Question,
                   MessageBoxButton.YesNo,
                   MessageBoxImage.Question);

                if (result == MessageBoxResult.No)
                {
                    return;
                }

                this.presetService.Remove(this.selectedPreset.Name);
                this.NotifyOfPropertyChange(() => this.PresetsCategories);
                this.SelectedPreset = this.presetService.GetDefaultPreset();
            }
            else
            {
                this.errorService.ShowMessageBox(Resources.Main_SelectPreset, Resources.Warning, MessageBoxButton.OK, MessageBoxImage.Warning);
            }
        }

        public void SetDefault()
        {
            if (this.selectedPreset != null)
            {
                this.presetService.SetDefault(this.selectedPreset.Name);
                this.errorService.ShowMessageBox(string.Format(Resources.Main_NewDefaultPreset, this.selectedPreset.Name), Resources.Main_Presets, MessageBoxButton.OK, MessageBoxImage.Information);
            }
            else
            {
                this.errorService.ShowMessageBox(Resources.Main_SelectPreset, Resources.Warning, MessageBoxButton.OK, MessageBoxImage.Warning);
            }
        }

        public void Import()
        {
            OpenFileDialog dialog = new OpenFileDialog { Filter = "Preset Files|*.json;*.plist", CheckFileExists = true };
            bool? dialogResult = dialog.ShowDialog();
            if (dialogResult.HasValue && dialogResult.Value)
            {
                this.presetService.Import(dialog.FileName);
                this.NotifyOfPropertyChange(() => this.PresetsCategories);
            }
        }

        public void Export()
        {
            if (this.selectedPreset != null && !this.selectedPreset.IsBuildIn)
            {
                SaveFileDialog savefiledialog = new SaveFileDialog
                {
                    Filter = "json|*.json",
                    CheckPathExists = true,
                    AddExtension = true,
                    DefaultExt = ".json",
                    OverwritePrompt = true,
                    FilterIndex = 0
                };

                savefiledialog.ShowDialog();
                string filename = savefiledialog.FileName;

                if (!string.IsNullOrEmpty(filename))
                {
                    this.presetService.Export(savefiledialog.FileName, this.selectedPreset.Name);
                }
            }
            else
            {
                this.errorService.ShowMessageBox(Resources.Main_SelectPreset, Resources.Warning, MessageBoxButton.OK, MessageBoxImage.Warning);
            }
        }

        public void ExportUserPresets()
        {
            SaveFileDialog savefiledialog = new SaveFileDialog
            {
                Filter = "json|*.json",
                CheckPathExists = true,
                AddExtension = true,
                DefaultExt = ".json",
                OverwritePrompt = true,
                FilterIndex = 0
            };

            savefiledialog.ShowDialog();
            string filename = savefiledialog.FileName;

            if (!string.IsNullOrEmpty(filename))
            {
                IList<PresetDisplayCategory> userPresets = this.presetService.GetPresetCategories(true);
                this.presetService.ExportCategories(savefiledialog.FileName, userPresets);
            }
        }

        public void DeleteBuiltInPresets()
        {
            this.presetService.DeleteBuiltInPresets();
            if (this.presetService.GetDefaultPreset() != null)
            {
                this.SelectedPreset = this.presetService.GetDefaultPreset();
            }
        }

        public void ResetBuiltInPresets()
        {
            this.presetService.UpdateBuiltInPresets();

            this.NotifyOfPropertyChange(() => this.PresetsCategories);

            this.SetDefaultPreset();
            
            this.errorService.ShowMessageBox(Resources.Presets_ResetComplete, Resources.Presets_ResetHeader, MessageBoxButton.OK, MessageBoxImage.Information);
        }

        public void EditAudioDefaults()
        {
            if (this.selectedPreset == null)
            {
                return;
            }

            IAudioDefaultsViewModel audioDefaultsViewModel = new AudioDefaultsViewModel(this.windowManager);
            audioDefaultsViewModel.Setup(this.selectedPreset.AudioTrackBehaviours, this.selectedPreset.Task.OutputFormat);
            audioDefaultsViewModel.ShowWindow();

            if (audioDefaultsViewModel.IsApplied)
            {
                this.SelectedPreset.AudioTrackBehaviours = new AudioBehaviours(audioDefaultsViewModel.AudioBehaviours);
            }
        }

        public void EditSubtitleDefaults()
        {
            if (this.selectedPreset == null)
            {
                return;
            }

            ISubtitlesDefaultsViewModel subtitlesDefaultsViewModel = new SubtitlesDefaultsViewModel(windowManager);
            subtitlesDefaultsViewModel.SetupPreset(this.selectedPreset);
            subtitlesDefaultsViewModel.ShowWindow();

            if (subtitlesDefaultsViewModel.IsApplied)
            {
                this.SelectedPreset.SubtitleTrackBehaviours = new SubtitleBehaviours(subtitlesDefaultsViewModel.SubtitleBehaviours);
            }
        }

        public void Close()
        {
            this.presetService.Save();
            this.IsOpen = false;
            this.presetService.PresetCollectionChanged -= this.PresetService_PresetCollectionChanged;

            if (this.mainWindowCallback != null)
            {
                mainWindowCallback(this.selectedPreset);
            }
        }

        public void SetCurrentPresetAsDefault()
        {
            if (this.SelectedPreset != null)
            {
                this.presetService.SetDefault(this.SelectedPreset.Name);
            }
        }

        public void LaunchHelp()
        {
            Process.Start("explorer.exe", "https://handbrake.fr/docs/en/latest/advanced/custom-presets.html");
        }

        public void SetPresetNameChanged()
        {
            this.NotifyOfPropertyChange(() => this.IsPresetNameChanged);
        }

        private void SetDefaultPreset()
        {
            // Preset Selection
            if (this.presetService.GetDefaultPreset() != null)
            {
                PresetDisplayCategory category =
                    (PresetDisplayCategory)this.PresetsCategories.FirstOrDefault(
                        p => p.Category == this.presetService.GetDefaultPreset().Category);

                this.SelectedPresetCategory = category;
                this.SelectedPreset = this.presetService.GetDefaultPreset();
            }
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

        private void PresetService_PresetCollectionChanged(object sender, System.EventArgs e)
        {
            string presetName = this.selectedPreset?.Name; // Recording such that we can re-select

            this.PresetsCategories = this.presetService.Presets;
            this.UserPresetCategories = presetService.GetPresetCategories(true).ToList(); // .Union(new List<PresetDisplayCategory> { addNewCategory }).ToList();

            this.NotifyOfPropertyChange(() => this.PresetsCategories);
            this.NotifyOfPropertyChange(() => this.UserPresetCategories);
            this.NotifyOfPropertyChange(() => this.SelectedUserPresetCategory);

            // Reselect the preset as the object has changed due to the reload that occurred.
            if (!string.IsNullOrEmpty(presetName))
            {
                this.SelectedPreset = this.presetService.GetPresetByName(presetName);
            }
        }
    }
}
