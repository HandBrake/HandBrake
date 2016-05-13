// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IQueueViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Queue View Model Interface
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels.Interfaces
{
    /// <summary>
    /// The Queue View Model Interface
    /// </summary>
    public interface IQueueViewModel
    {
        /// <summary>
        /// The when done action after a queue completes.
        /// </summary>
        /// <param name="action">
        /// The action.
        /// </param>
        void WhenDone(string action);

        /// <summary>
        /// The import.
        /// </summary>
        void Import();

        /// <summary>
        /// The export.
        /// </summary>
        void Export();

        /// <summary>
        /// The clear completed.
        /// </summary>
        void ClearCompleted();

        /// <summary>
        /// The clear.
        /// </summary>
        void Clear();

        /// <summary>
        /// The remove selected jobs.
        /// </summary>
        void RemoveSelectedJobs();
    }
}