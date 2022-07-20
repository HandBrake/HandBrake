// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IManagePresetViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Manage Preset View Model Interface
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels.Interfaces
{
    using HandBrakeWPF.Services.Presets.Model;

    /// <summary>
    /// The Add Preset View Model
    /// </summary>
    public interface IManagePresetViewModel : IViewModelBase
    {
        /// <summary>
        /// Prepare the Preset window 
        /// </summary>
        /// <param name="preset">
        /// The preset.
        /// </param>
        void Setup(Preset preset);

        /// <summary>
        /// Get the managed preset.
        /// </summary>
        Preset Preset { get; }
    }
}
