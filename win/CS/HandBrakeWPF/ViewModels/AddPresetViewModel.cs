// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AddPresetViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Add Preset View Model
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels
{
    using Caliburn.Micro;

    /// <summary>
    /// The Add Preset View Model
    /// </summary>
    public class AddPresetViewModel : ViewModelBase
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="AddPresetViewModel"/> class.
        /// </summary>
        /// <param name="windowManager">
        /// The window manager.
        /// </param>
        public AddPresetViewModel(IWindowManager windowManager) : base(windowManager)
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
