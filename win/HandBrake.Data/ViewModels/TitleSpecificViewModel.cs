// --------------------------------------------------------------------------------------------------------------------
// <copyright file="TitleSpecificViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the TitleSpecificViewModel type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels
{
    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    /// The Title Specific View Model
    /// </summary>
    public class TitleSpecificViewModel : ViewModelBase, ITitleSpecificViewModel
    {
        #region Constants and Fields

        /// <summary>
        /// The selected title.
        /// </summary>
        private int? selectedTitle;

        #endregion

        #region Constructors and Destructors

        /// <summary>
        /// Initializes a new instance of the <see cref="TitleSpecificViewModel"/> class. 
        /// </summary>
        public TitleSpecificViewModel()
        {
            this.SelectedTitle = 0;
        }

        #endregion

        #region Properties

        /// <summary>
        /// Gets or sets SelectedTitle.
        /// </summary>
        public int? SelectedTitle
        {
            get
            {
                return this.selectedTitle;
            }

            set
            {
                this.selectedTitle = value;
                this.NotifyOfPropertyChange(() => this.SelectedTitle);
            }
        }

        #endregion

        #region Public Methods

        /// <summary>
        /// Cancel the request to scan.
        /// </summary>
        public void Cancel()
        {
            this.selectedTitle = null;
            this.TryClose();
        }

        /// <summary>
        /// Open the selected title.
        /// </summary>
        public void Open()
        {
            this.TryClose();
        }

        #endregion
    }
}