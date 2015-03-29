// --------------------------------------------------------------------------------------------------------------------
// <copyright file="OpenOverlayPanelCommand.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The open overlay panel command.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Commands
{
    using System;
    using System.Windows.Input;

    using Caliburn.Micro;

    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    /// The open overlay panel command.
    /// </summary>
    public class OpenOverlayPanelCommand : ICommand
    {
        /// <summary>
        /// The execute.
        /// </summary>
        /// <param name="parameter">
        /// The parameter.
        /// </param>
        public void Execute(object parameter)
        {
            IOverlayPanel overlayPanel = parameter as IOverlayPanel;
            if (overlayPanel != null)
            {
                IShellViewModel shellViewModel = IoC.Get<IShellViewModel>();
                if (shellViewModel != null)
                {
                    shellViewModel.ShowOverlay(overlayPanel);
                }
            }
        }

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
        /// The can execute changed.
        /// </summary>
        public event EventHandler CanExecuteChanged;
    }
}
