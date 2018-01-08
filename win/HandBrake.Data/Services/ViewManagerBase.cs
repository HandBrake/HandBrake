// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ViewManagerBase.cs" company="HandBrake Project (http://handbrake.fr)">
// This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Services.Interfaces
{
    /// <summary>
    /// Handles Queue functions
    /// </summary>
    public abstract class ViewManagerBase
    {
        /// <summary>
        /// Gets a value indicating whether Window Mode is supported (Might not be on some platforms).
        /// </summary>
        public abstract bool SupportsWindow { get; }

        /// <summary>
        /// Shows a Window based on the ViewModel, if supported.
        /// </summary>
        /// <typeparam name="TViewModel">
        /// The Viewmodel Type.
        /// </typeparam>
        /// <param name="viewmodel">
        /// Viewmodel to use for the Window.
        /// </param>
        public abstract void ShowWindow<TViewModel>(TViewModel viewmodel = default(TViewModel));

        /// <summary>
        /// Shows a Dialog based on the ViewModel.
        /// </summary>
        /// <typeparam name="TViewModel">
        /// The Viewmodel Type.
        /// </typeparam>
        /// <param name="viewmodel">
        /// Viewmodel to use for the Dialog.
        /// </param>
        public abstract void ShowDialog<TViewModel>(TViewModel viewmodel = default(TViewModel));
    }
}