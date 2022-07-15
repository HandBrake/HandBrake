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

    using HandBrakeWPF.Helpers;
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
        public event EventHandler CanExecuteChanged { add { } remove { } }

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
            var shellViewModel = IoCHelper.Get<IShellViewModel>();
            var optionsViewModel = IoCHelper.Get<IOptionsViewModel>();

            shellViewModel.DisplayWindow(ShellWindow.OptionsWindow);

            optionsViewModel.UpdateSettings();

            if (parameter == null && optionsViewModel.SelectedTab == OptionsTab.About)
            {
                optionsViewModel.GotoTab(OptionsTab.General);
            }

            if (parameter != null && parameter.GetType() == typeof(OptionsTab))
            {
                optionsViewModel.GotoTab((OptionsTab)parameter);
                if (((OptionsTab)parameter).Equals(OptionsTab.Updates))
                {
                    optionsViewModel.PerformUpdateCheck();
                }
            }
        }
    }
}