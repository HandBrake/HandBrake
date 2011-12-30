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
    using System.ComponentModel.Composition;
    using System.Windows;

    using Caliburn.Micro;

    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Services.Interfaces;
    using HandBrake.ApplicationServices.Utilities;

    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    /// The Add Preset View Model
    /// </summary>
    [Export(typeof(IAddPresetViewModel))]
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
        /// Initializes a new instance of the <see cref="AddPresetViewModel"/> class.
        /// </summary>
        /// <param name="windowManager">
        /// The window manager.
        /// </param>
        /// <param name="presetService">
        /// The Preset Service
        /// </param>
        /// <param name="errorService">
        /// The Error Service
        /// </param>
        public AddPresetViewModel(IWindowManager windowManager, IPresetService presetService, IErrorService errorService)
        {
            this.presetService = presetService;
            this.errorService = errorService;
            this.Title = "Add Preset";
            this.Preset = new Preset {IsBuildIn = false, IsDefault = false, Category = "User Presets"};
        }

        /// <summary>
        /// Gets or sets the Preset
        /// </summary>
        public Preset Preset { get; private set; }

        /// <summary>
        /// Prepare the Preset window to create a Preset Object later.
        /// </summary>
        /// <param name="task">
        /// The Encode Task.
        /// </param>
        public void Setup(EncodeTask task)
        {
            task.UsesPictureFilters = this.Preset.UsePictureFilters;
            task.UsesMaxPictureSettings = false; // TODO
            task.UsesPictureSettings = false; // TODO
            this.Preset.Task = task;
            this.Preset.Query = QueryGeneratorUtility.GenerateQuery(task);
        }

        /// <summary>
        /// Add a Preset
        /// </summary>
        public void Add()
        {
            if (string.IsNullOrEmpty(this.Preset.Name))
            {
                this.errorService.ShowMessageBox("A Preset must have a Name. Please fill out the Preset Name field.", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            if (this.presetService.CheckIfPresetExists(this.Preset.Name))
            {
                this.errorService.ShowMessageBox("A Preset with this name already exists. Please choose a new name", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

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
