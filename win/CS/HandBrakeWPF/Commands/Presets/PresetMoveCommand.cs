// --------------------------------------------------------------------------------------------------------------------
// <copyright file="PresetMoveCommand.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Commands.Presets
{
    using System;
    using System.Windows.Input;

    using HandBrakeWPF.Services.Presets.Interfaces;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.ViewModels;

    public class PresetMoveCommand : ViewModelBase, ICommand
    {
        private readonly MainViewModel viewModel;

        private readonly IPresetService presetService;

        public PresetMoveCommand(MainViewModel viewModel, IPresetService presetService)
        {
            this.viewModel = viewModel;
            this.presetService = presetService;
        }

        public bool CanExecute(object parameter)
        {
            return true;
        }

        public void Execute(object parameter)
        {
            string command = (string)parameter;
            Preset preset = this.viewModel.SelectedPreset;

            switch (command)
            {
                case "top":
                    this.presetService.MoveToTopOfGroup(preset);
                    return;
                case "bottom":
                    this.presetService.MoveToBottomOfGroup(preset);
                    return;
                case "up":
                    this.presetService.MoveUp(preset);
                    return;
                case "down":
                    this.presetService.MoveDown(preset);
                    return;
            }

            this.NotifyOfPropertyChange(() => this.viewModel.PresetsCategories);
        }

        public event EventHandler CanExecuteChanged;
    }
}
