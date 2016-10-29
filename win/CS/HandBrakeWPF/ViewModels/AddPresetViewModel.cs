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

    using Caliburn.Micro;

    using HandBrake.ApplicationServices.Interop.Model.Encoding;

    using HandBrakeWPF.Model.Audio;
    using HandBrakeWPF.Model.Subtitles;
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Presets;
    using HandBrakeWPF.Services.Presets.Interfaces;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.Services.Scan.Model;
    using HandBrakeWPF.Utilities;
    using HandBrakeWPF.ViewModels.Interfaces;

    using EncodeTask = HandBrakeWPF.Services.Encode.Model.EncodeTask;
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
        /// The window manager.
        /// </summary>
        private readonly IWindowManager windowManager;

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

        private IAudioDefaultsViewModel audioDefaultsViewModel;
        private ISubtitlesDefaultsViewModel subtitlesDefaultsViewModel;

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
            this.Title = "Add Preset";
            this.Preset = new Preset { IsBuildIn = false, IsDefault = false, Category = PresetService.UserPresetCatgoryName};
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
                this.errorService.ShowMessageBox(Resources.AddPresetViewModel_PresetMustProvideName, Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            if (this.presetService.CheckIfPresetExists(this.Preset.Name))
            {
                MessageBoxResult result = this.errorService.ShowMessageBox(Resources.AddPresetViewModel_PresetWithSameNameOverwriteWarning, Resources.Error, MessageBoxButton.YesNo, MessageBoxImage.Error);
                if (result == MessageBoxResult.No)
                {
                    return;
                }
            }

            if (this.SelectedPictureSettingMode == PresetPictureSettingsMode.SourceMaximum && this.selectedTitle == null)
            {
                this.errorService.ShowMessageBox(Resources.AddPresetViewModel_YouMustFirstScanSource, Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            if (this.CustomWidth == null && this.CustomHeight == null && this.SelectedPictureSettingMode == PresetPictureSettingsMode.Custom)
            {
                this.errorService.ShowMessageBox(Resources.AddPresetViewModel_CustomWidthHeightFieldsRequired, Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
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
                this.Preset.Task.MaxWidth = selectedTitle.Resolution.Width;
                this.Preset.Task.MaxHeight = selectedTitle.Resolution.Height;
            }

            // Add the Preset
            bool added = this.presetService.Add(this.Preset);
            if (!added)
            {
                this.errorService.ShowMessageBox(Resources.AddPresetViewModel_UnableToAddPreset, Resources.UnknownError, MessageBoxButton.OK,
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
            IPopupWindowViewModel popup = new PopupWindowViewModel(this.audioDefaultsViewModel, ResourcesUI.Preset_AudioDefaults_Title, ResourcesUI.Preset_AudioDefaults_SubText);
            if (this.windowManager.ShowDialog(popup) == true)
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
            
            if (this.windowManager.ShowDialog(popup) == true)
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
