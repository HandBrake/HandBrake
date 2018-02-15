﻿// --------------------------------------------------------------------------------------------------------------------
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
            // Implement in the ViewModel to perform viewmodel specific code.
        }

        #endregion
    }
}