// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ITaskBarService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   An interface for TaskBar Progress.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Utilities.Interfaces
{
    /// <summary>
    /// An interface for TaskBar Progress.
    /// </summary>
    public interface ITaskBarService
    {
        /// <summary>
        /// Set the Task Bar Percentage.
        /// </summary>
        /// <param name="percentage">
        /// The percentage.
        /// </param>
        void SetTaskBarProgress(int percentage);

        /// <summary>
        /// Disable Task Bar Progress
        /// </summary>
        void DisableTaskBarProgress();
    }
}