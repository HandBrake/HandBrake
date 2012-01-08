// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IAudioViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the IAudioViewModel type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels.Interfaces
{
    using HandBrake.ApplicationServices.Model;

    /// <summary>
    /// The Audio View Model Interface
    /// </summary>
    public interface IAudioViewModel
    {
        /// <summary>
        /// Set the selected preset
        /// </summary>
        /// <param name="preset">
        /// The preset.
        /// </param>
        void SetPreset(Preset preset);
    }
}
