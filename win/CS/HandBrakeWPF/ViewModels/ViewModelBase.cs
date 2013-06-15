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

    using HandBrake.ApplicationServices.Services.Interfaces;

    using HandBrakeWPF.Helpers;
    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    /// A Base Class for the View Models which contains reusable code.
    /// </summary>
    public class ViewModelBase : Screen, IViewModelBase
    {
        #region Constants and Fields

        /// <summary>
        /// Backing Field to prevent the Load method being called more than once.
        /// </summary>
        private bool hasLoaded;

        /// <summary>
        /// The title.
        /// </summary>
        private string title;

        #endregion

        #region Constructors and Destructors

        /// <summary>
        /// Initializes a new instance of the <see cref="ViewModelBase"/> class.
        /// </summary>
        public ViewModelBase()
        {
        }

        #endregion

        #region Properties

        /// <summary>
        /// Gets or sets Details.
        /// </summary>
        public string Title
        {
            get
            {
                return this.title;
            }

            set
            {
                this.title = value;
                this.NotifyOfPropertyChange("Title");
            }
        }

        /// <summary>
        /// Gets a value indicating whether use system colours.
        /// </summary>
        public bool UseSystemColours
        {
            get
            {
                return AppStyleHelper.UseSystemColours;
            }
        }

        /// <summary>
        /// Gets or sets WindowManager.
        /// </summary>
        public IWindowManager WindowManager { get; set; }

        /// <summary>
        /// Gets or sets UserSettingService.
        /// </summary>
        public IUserSettingService UserSettingService { get; set; }

        #endregion

        #region Public Methods

        /// <summary>
        /// Perform any Initialisation for this ViewModelBase.
        /// </summary>
        public void Load()
        {
            if (!this.hasLoaded)
            {
                this.hasLoaded = true;

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

        #endregion
    }
}