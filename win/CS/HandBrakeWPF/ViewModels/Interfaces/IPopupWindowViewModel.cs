// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IPopupWindowViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The PopupWindowViewModel interface.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels.Interfaces
{
    /// <summary>
    /// The PopupWindowViewModel interface.
    /// </summary>
    public interface IPopupWindowViewModel
    {
        /// <summary>
        /// Gets the content view model.
        /// </summary>
        IViewModelBase ContentViewModel { get; }
    }
}