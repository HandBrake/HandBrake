// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IChaptersViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the IChaptersViewModel type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels.Interfaces
{
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Parsing;

    /// <summary>
    /// The Chapters View Model Interface
    /// </summary>
    public interface IChaptersViewModel
    {
        /// <summary>
        /// Set the selected preset
        /// </summary>
        /// <param name="currentTitle">
        /// The current Title.
        /// </param>
        /// <param name="preset">
        /// The preset.
        /// </param>
        /// <param name="task">
        /// The task.
        /// </param>
        void SetSource(Title currentTitle, Preset preset, EncodeTask task);
    }
}
