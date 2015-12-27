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
    }
}