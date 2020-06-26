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
    using System.Linq;
    using System.Windows;

    using Caliburn.Micro;

    using HandBrakeWPF.Model.Audio;
    using HandBrakeWPF.Model.Subtitles;
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Presets;
    using HandBrakeWPF.Services.Presets.Interfaces;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.Services.Scan.Model;
    using HandBrakeWPF.ViewModels.Interfaces;
    using HandBrakeWPF.Views;

    using EncodeTask = HandBrakeWPF.Services.Encode.Model.EncodeTask;

    /// <summary>
    /// The Add Preset View Model
    /// </summary>
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

        /// <summary>
        /// Initializes a new instance of the <see cref="AddPresetViewModel"/> class.
        /// </summary>
        /// <param name="presetService">
        /// The Preset Service
        /// </param>
        /// <param name="errorService">
        /// The Error Service
        /// </param>
        /// <param name="windowManager">
        /// The window Manager.
        /// </param>
        public AddPresetViewModel(IPresetService presetService, IErrorService errorService, IWindowManager windowManager)
        {
            this.presetService = presetService;
            this.errorService = errorService;
            this.windowManager = windowManager;
            this.Title = Resources.AddPresetView_AddPreset;
            this.Preset = new Preset { IsBuildIn = false, IsDefault = false, Category = PresetService.UserPresetCatgoryName };
            this.PresetCategories = presetService.GetPresetCategories(true).Union(new List<PresetDisplayCategory> { addNewCategory }).ToList();
            this.SelectedPresetCategory = this.PresetCategories.FirstOrDefault(n => n.Category == PresetService.UserPresetCatgoryName);
        }

        public Preset Preset { get; }

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
                    this.PresetCategory = PresetService.UserPresetCatgoryName;
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
                this.NotifyOfPropertyChange();
            }
        }

        public void Setup(EncodeTask task, Title title, AudioBehaviours audioBehaviours, SubtitleBehaviours subtitleBehaviours)
        {
            this.Preset.Task = new EncodeTask(task);
            this.Preset.AudioTrackBehaviours = audioBehaviours.Clone();
            this.Preset.SubtitleTrackBehaviours = subtitleBehaviours.Clone();

            this.audioDefaultsViewModel = new AudioDefaultsViewModel(this.Preset.Task);
            this.audioDefaultsViewModel.Setup(this.Preset, this.Preset.Task);

            this.subtitlesDefaultsViewModel = new SubtitlesDefaultsViewModel();
            this.subtitlesDefaultsViewModel.SetupLanguages(subtitleBehaviours);
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

        /// <summary>
        /// The edit audio defaults.
        /// </summary>
        public void EditAudioDefaults()
        {
            this.audioDefaultsViewModel.ResetApplied();
            this.windowManager.ShowDialog(this.audioDefaultsViewModel);
            if (audioDefaultsViewModel.IsApplied)
            {
                this.Preset.AudioTrackBehaviours = this.audioDefaultsViewModel.AudioBehaviours.Clone();
            }
        }

        /// <summary>
        /// The edit subtitle defaults.
        /// </summary>
        public void EditSubtitleDefaults()
        {
            this.subtitlesDefaultsViewModel.ResetApplied();
            SubtitlesDefaultsView view = new SubtitlesDefaultsView();
            view.DataContext = this.subtitlesDefaultsViewModel;
            view.ShowDialog();

            if (subtitlesDefaultsViewModel.IsApplied)
            {
                this.Preset.SubtitleTrackBehaviours = this.subtitlesDefaultsViewModel.SubtitleBehaviours.Clone();
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
