// --------------------------------------------------------------------------------------------------------------------
// <copyright file="QueueViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Preview View Model
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels
{
    using Caliburn.Micro;

    /// <summary>
    /// The Preview View Model
    /// </summary>
    public class QueueViewModel : ViewModelBase
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="QueueViewModel"/> class.
        /// </summary>
        /// <param name="windowManager">
        /// The window manager.
        /// </param>
        public QueueViewModel(IWindowManager windowManager) : base(windowManager)
        {
        }

        /// <summary>
        /// Close this window.
        /// </summary>
        public void Close()
        {
            this.TryClose();
        }
    }
}
