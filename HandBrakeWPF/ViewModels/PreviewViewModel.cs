// --------------------------------------------------------------------------------------------------------------------
// <copyright file="PreviewViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The About View Model
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels
{
    using Interfaces;
    using Caliburn.Micro;

    /// <summary>
    /// The About View Model
    /// </summary>
    public class PreviewViewModel : ViewModelBase, IPreviewViewModel
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="PreviewViewModel"/> class.
        /// </summary>
        /// <param name="windowManager">
        /// The window manager.
        /// </param>
        public PreviewViewModel(IWindowManager windowManager)
        {
            this.Title = "Preview";
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
