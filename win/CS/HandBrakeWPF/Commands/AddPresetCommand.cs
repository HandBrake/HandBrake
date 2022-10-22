// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AddPresetCommand.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Commands
{
    using System;
    using System.Windows.Input;

    using HandBrakeWPF.Helpers;
    using HandBrakeWPF.ViewModels.Interfaces;

    public class AddPresetCommand : ICommand
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
            viewModel.PresetAdd();
        }

        protected virtual void OnCanExecuteChanged()
        {
            this.CanExecuteChanged?.Invoke(this, EventArgs.Empty);
        }
#nullable disable
    }
}
