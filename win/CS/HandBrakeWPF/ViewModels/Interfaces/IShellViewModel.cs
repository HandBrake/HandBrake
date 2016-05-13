// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IShellViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Interface for the Shell View Model
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels.Interfaces
{
    using HandBrakeWPF.Model;

    /// <summary>
    /// The Interface for the Shell View Model
    /// </summary>
    public interface IShellViewModel
    {
        /// <summary>
        /// Change the page displayed on this window.
        /// </summary>
        /// <param name="window">
        /// The window.
        /// </param>
        void DisplayWindow(ShellWindow window);

        /// <summary>
        /// Checks with the use if this window can be closed.
        /// </summary>
        /// <returns>
        /// Returns true if the window can be closed.
        /// </returns>
        bool CanClose();
    }
}
