// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ViewModelBase.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A Base Class for the View Models which contains reusable code.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels
{
    using Caliburn.Micro;

    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    /// A Base Class for the View Models which contains reusable code.
    /// </summary>
    public class ViewModelBase : Screen, IViewModelBase
    {
        /// <summary>
        /// Backing Field to prevent the Load method being called more than once.
        /// </summary>
        private bool hasLoaded;

        /// <summary>
        /// Initializes a new instance of the <see cref="ViewModelBase"/> class.
        /// </summary>
        /// <param name="windowManager">
        /// The window manager.
        /// </param>
        public ViewModelBase(IWindowManager windowManager)
        {
            this.WindowManager = windowManager;
        }

        /// <summary>
        /// Gets WindowManager.
        /// </summary>
        public IWindowManager WindowManager { get; private set; }

        /// <summary>
        /// Perform any Initialisation for this ViewModelBase.
        /// </summary>
        public void Load()
        {
            if (!hasLoaded)
            {
                hasLoaded = true;

                // Initialise the ViewModels OnLoad method if it exists.
                this.OnLoad();              
            }
        }

        /// <summary>
        /// Load Method for the ViewModel
        /// </summary>
        public virtual void OnLoad()
        {
            // Impliment in the ViewModel to perform viewmodel specific code.
        }
    }
}
