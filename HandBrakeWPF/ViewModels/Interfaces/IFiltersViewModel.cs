// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IFiltersViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the IFiltersViewModel type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels.Interfaces
{
    using HandBrake.ApplicationServices.Model;

    /// <summary>
    /// The Filters View Model Interface
    /// </summary>
    public interface IFiltersViewModel
    {
        /// <summary>
        /// Setup a selected preset.
        /// </summary>
        /// <param name="preset">
        /// The Current Preset.
        /// </param>
        void SetPreset(Preset preset);
    }
}
