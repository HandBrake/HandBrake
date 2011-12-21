// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IMainViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Main Window View Model
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels.Interfaces
{
    using HandBrake.ApplicationServices.Model;

    /// <summary>
    /// The Main Window View Model
    /// </summary>
    public interface IMainViewModel
    {
        /// <summary>
        /// Sets SelectedPreset.
        /// </summary>
        Preset SelectedPreset { set; }

        /// <summary>
        /// Shutdown the Application
        /// </summary>
        void ExitApplication();
    }
}