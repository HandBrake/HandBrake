// --------------------------------------------------------------------------------------------------------------------
// <copyright file="PresetMenuSelectCommand.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A Command to handle the Preset Menu Clicks.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Commands.Presets
{
    using System;
    using System.Windows.Input;

    using HandBrakeWPF.Helpers;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    /// The preset menu select command.
    /// </summary>
    public class PresetMenuSelectCommand : ICommand
    {
        private readonly Preset preset;

        /// <summary>
        /// Initializes a new instance of the <see cref="PresetMenuSelectCommand"/> class. 
        /// </summary>
        /// <param name="preset">
        /// The preset.
        /// </param>
        public PresetMenuSelectCommand(Preset preset)
        {
            this.preset = preset;
        }

        /// <summary>Defines the method that determines whether the command can execute in its current state.</summary>
        /// <returns>true if this command can be executed; otherwise, false.</returns>
        /// <param name="parameter">Data used by the command.  If the command does not require data to be passed, this object can be set to null.</param>
        public bool CanExecute(object parameter)
        {
            return true;
        }

        /// <summary>Defines the method to be called when the command is invoked.</summary>
        /// <param name="parameter">Data used by the command.  If the command does not require data to be passed, this object can be set to null.</param>
        public void Execute(object parameter)
        {
            IMainViewModel mvm = IoCHelper.Get<IMainViewModel>();
            mvm.PresetSelect(this.preset);
        }

        /// <summary>
        /// The can execute changed.
        /// </summary>
        public event EventHandler CanExecuteChanged { add { } remove { } }
    }
}
