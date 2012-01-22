// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IPictureSettingsViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the IPictureSettingsViewModel type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels.Interfaces
{
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Parsing;

    /// <summary>
    /// The Picture Settings View Model Interface
    /// </summary>
    public interface IPictureSettingsViewModel
    {
        /// <summary>
        /// Setup the window after a scan.
        /// </summary>
        /// <param name="selectedTitle">
        /// The selected title.
        /// </param>
        /// <param name="currentPreset">
        /// The Current preset
        /// </param>
        /// <param name="task">
        /// The task.
        /// </param>
        void SetSource(Title selectedTitle, Preset currentPreset, EncodeTask task);
    }
}
