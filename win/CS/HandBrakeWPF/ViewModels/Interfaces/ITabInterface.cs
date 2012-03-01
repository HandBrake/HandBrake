// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ITabInterface.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the ITabInterface type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels.Interfaces
{
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Parsing;

    /// <summary>
    /// Common interface for all the main tab panels
    /// </summary>
    public interface ITabInterface
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

        /// <summary>
        /// Set the selected preset
        /// </summary>
        /// <param name="preset">
        /// The preset.
        /// </param>
        void SetPreset(Preset preset);
    }
}
