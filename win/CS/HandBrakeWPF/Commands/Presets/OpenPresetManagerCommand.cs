// --------------------------------------------------------------------------------------------------------------------
// <copyright file="OpenPresetManagerCommand.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the OpenPresetManagerCommand type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Commands.Presets
{
    using System;
    using System.Windows.Input;

    using HandBrakeWPF.Helpers;
    using HandBrakeWPF.ViewModels.Interfaces;

    public class OpenPresetManagerCommand : ICommand
    {
#nullable enable
        public event EventHandler? CanExecuteChanged;

        public bool CanExecute(object? parameter)
        {
            return true;
        }

        public void Execute(object? parameter)
        {
            IMainViewModel viewModel = IoCHelper.Get<IMainViewModel>();
            viewModel.OpenPresetWindow();
        }

        protected virtual void OnCanExecuteChanged()
        {
            this.CanExecuteChanged?.Invoke(this, EventArgs.Empty);
        }
#nullable disable
    }
}
