// --------------------------------------------------------------------------------------------------------------------
// <copyright file="PresetClone.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------


namespace HandBrakeWPF.Commands.Presets
{
    using System;
    using System.Windows;
    using System.Windows.Input;

    using HandBrakeWPF.Helpers;
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Presets.Interfaces;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.ViewModels;
    using HandBrakeWPF.ViewModels.Interfaces;
    using HandBrakeWPF.Views;

    public class PresetClone : ViewModelBase, ICommand
    {
        private readonly IPresetService presetService;
        private readonly IErrorService errorService;
        private readonly IWindowManager windowManager;

        public PresetClone(IPresetService presetService, IErrorService errorService, IWindowManager windowManager)
        {
            this.presetService = presetService;
            this.errorService = errorService;
            this.windowManager = windowManager;
        }

        public bool CanExecute(object parameter)
        {
            return true;
        }

        public void Execute(object parameter)
        {
            MainViewModel mainViewModel = parameter as MainViewModel;
            if (mainViewModel == null || mainViewModel.SelectedPreset == null)
            {
                this.errorService.ShowMessageBox(
                    Resources.Main_SelectPresetForUpdate, Resources.Main_NoPresetSelected, MessageBoxButton.OK, MessageBoxImage.Warning); // TODO Update
                return;
            }

            Preset clonedPreset = new Preset(mainViewModel.SelectedPreset);
            clonedPreset.Name = this.presetService.GenerateUniqueName(mainViewModel.SelectedPreset.Name);


            IAddPresetViewModel presetViewModel = IoCHelper.Get<IAddPresetViewModel>();
            presetViewModel.Setup(clonedPreset.Task, clonedPreset.AudioTrackBehaviours, clonedPreset.SubtitleTrackBehaviours, clonedPreset.Name);
            bool? result = this.windowManager.ShowDialog<AddPresetView>(presetViewModel);

            if (result.HasValue && result.Value)
            {
                this.NotifyOfPropertyChange(() => mainViewModel.PresetsCategories);
                mainViewModel.SelectedPreset = this.presetService.GetPreset(presetViewModel.PresetName);
                mainViewModel.IsModifiedPreset = false;
            }
        }

        public event EventHandler CanExecuteChanged;
    }
}
