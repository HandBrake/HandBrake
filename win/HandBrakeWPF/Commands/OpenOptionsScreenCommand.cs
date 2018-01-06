// --------------------------------------------------------------------------------------------------------------------
// <copyright file="OpenOptionsScreenCommand.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A Command to display the options window.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Commands
{
    using System;
    using System.Windows.Input;

    using Caliburn.Micro;

    using HandBrakeWPF.Model;
    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    ///     A Command to display the options window.
    /// </summary>
    public class OpenOptionsScreenCommand : ICommand
    {
        /// <summary>
        /// The can execute changed.
        /// </summary>
        public event EventHandler CanExecuteChanged;

        /// <summary>
        /// The can execute.
        /// </summary>
        /// <param name="parameter">
        /// The parameter.
        /// </param>
        /// <returns>
        /// The <see cref="bool"/>.
        /// </returns>
        public bool CanExecute(object parameter)
        {
            return true;
        }

        /// <summary>
        /// The execute.
        /// </summary>
        /// <param name="parameter">
        /// The parameter.
        /// </param>
        public void Execute(object parameter)
        {
            var shellViewModel = IoC.Get<IShellViewModel>();
            shellViewModel.DisplayWindow(ShellWindow.OptionsWindow);

            if (parameter != null && parameter.GetType() == typeof(OptionsTab))
            {
                var optionsViewModel = IoC.Get<IOptionsViewModel>();
                optionsViewModel.GotoTab((OptionsTab)parameter);
            }
        }
    }
}