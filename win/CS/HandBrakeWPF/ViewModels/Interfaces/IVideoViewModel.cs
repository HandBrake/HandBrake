// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IVideoViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the IVideoViewModel type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels.Interfaces
{
    /// <summary>
    /// The Video View Model Interface
    /// </summary>
    public interface IVideoViewModel : ITabInterface
    {
        /// <summary>
        /// Trigger a Notify Property Changed on the Task to force various UI elements to update.
        /// </summary>
        void RefreshTask();
    }
}
