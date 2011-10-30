// --------------------------------------------------------------------------------------------------------------------
// <copyright file="LogViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the LogViewModel type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels
{
    using Caliburn.Micro;

    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    /// The Log View Model
    /// </summary>
    public class LogViewModel : ViewModelBase, ILogViewModel
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="LogViewModel"/> class.
        /// </summary>
        /// <param name="windowManager">
        /// The window manager.
        /// </param>
        public LogViewModel(IWindowManager windowManager)
            : base(windowManager)
        {
        }
    }
}
