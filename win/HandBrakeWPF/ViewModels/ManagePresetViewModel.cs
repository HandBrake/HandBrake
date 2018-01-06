// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ManagePresetViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Add Preset View Model
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels
{
    using System.Windows;

    using Caliburn.Micro;

    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Presets;
    using HandBrakeWPF.Services.Presets.Interfaces;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    /// The Add Preset View Model
    /// </summary>
    public class ManagePresetViewModel : ViewModelBase, IManagePresetViewModel
    {
        private readonly IPresetService presetService;
        private readonly IErrorService errorService;
        private readonly IWindowManager windowManager;
        private Preset existingPreset;

        /// <summary>
        /// Initializes a new instance of the <see cref="ManagePresetViewModel"/> class.
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
        public ManagePresetViewModel(IPresetService presetService, IErrorService errorService, IWindowManager windowManager)
        {
            this.presetService = presetService;
            this.errorService = errorService;
            this.windowManager = windowManager;
            this.Title = "Manage Preset";
            this.Preset = new Preset { IsBuildIn = false, IsDefault = false, Category = PresetService.UserPresetCatgoryName };
        }

        /// <summary>
        /// Gets the Preset
        /// </summary>
        public Preset Preset { get; private set; }

        /// <summary>
        /// Prepare the Preset window to create a Preset Object later.
        /// </summary>
        /// <param name="presetToEdit">
        /// The preset To Edit.
        /// </param>
        public void Setup(Preset presetToEdit)
        {
            this.Preset = new Preset(presetToEdit); // Clone. We will not touch the existing object.
            this.existingPreset = presetToEdit;
        }

        /// <summary>
        /// Add a Preset
        /// </summary>
        public void Save()
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

            if (this.presetService.CheckIfPresetExists(this.Preset.Name))
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
            this.presetService.Replace(this.existingPreset, this.Preset);
            this.Close();
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
