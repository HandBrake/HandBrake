// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IViewManager.cs" company="HandBrake Project (http://handbrake.fr)">
// This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Services.Interfaces
{
    using HandBrake.Model;

    /// <summary>
    /// Handles Queue functions
    /// </summary>
    public interface IViewManager
    {
        /// <summary>
        /// Gets a value indicating whether Window Mode is supported (Might not be on some platforms).
        /// </summary>
        bool SupportsWindow { get; }

        /// <summary>
        /// Shows a Window based on the ViewModel, if supported.
        /// </summary>
        /// <typeparam name="TViewModel">
        /// The Viewmodel Type.
        /// </typeparam>
        /// <param name="viewmodel">
        /// Viewmodel to use for the Window.
        /// </param>
        void ShowWindow<TViewModel>(TViewModel viewmodel = default(TViewModel));

        /// <summary>
        /// Shows a Dialog based on the ViewModel.
        /// </summary>
        /// <typeparam name="TViewModel">
        /// The Viewmodel Type.
        /// </typeparam>
        /// <param name="viewmodel">
        /// Viewmodel to use for the Dialog.
        /// </param>
        /// <returns>
        /// Response
        /// </returns>
        bool? ShowDialog<TViewModel>(TViewModel viewmodel = default(TViewModel));

        /// <summary>
        /// Opens the Options View.
        /// </summary>
        /// <param name="tab">Tab to open to.</param>
        void OpenOptions(OptionsTab tab);
    }
}