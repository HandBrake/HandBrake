// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ITitleSpecificViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the ITitleSpecificViewModel type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels.Interfaces
{
    /// <summary>
    /// The Title Specific View Model Interface
    /// </summary>
    public interface ITitleSpecificViewModel
    {
        /// <summary>
        /// Gets SelectedTitle.
        /// </summary>
        int? SelectedTitle { get; }
    }
}
