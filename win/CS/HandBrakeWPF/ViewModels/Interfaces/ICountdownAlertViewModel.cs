// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ICountdownAlertViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Countdown Alert View Model Interface
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels.Interfaces
{
    using HandBrakeWPF.Model.Options;

    /// <summary>
    /// The Countdown Alert View Model Interface
    /// </summary>
    public interface ICountdownAlertViewModel : IViewModelBase
    {
        /// <summary>
        /// Gets a value indicating whether is cancelled.
        /// </summary>
        bool IsCancelled { get; }

        /// <summary>
        /// The set action.
        /// </summary>
        /// <param name="action">
        /// The action.
        /// </param>
        void SetAction(WhenDone action);
    }
}
