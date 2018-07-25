// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ErrorViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the ErrorViewModel type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels
{
    using System;
    using System.Windows;

    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    /// The Error View Model
    /// </summary>
    public class ErrorViewModel : ViewModelBase, IErrorViewModel
    {
        private readonly IErrorService errorService;

        private string details;
        private string errorMessage;
        private string solution;

        #region Constructors and Destructors

        /// <summary>
        /// Initializes a new instance of the <see cref="ErrorViewModel"/> class.
        /// </summary>
        public ErrorViewModel(IErrorService errorService)
        {
            this.errorService = errorService;
            this.Title = Resources.Error;
            this.ErrorMessage = Resources.ErrorViewModel_UnknownError;
            this.Details = Resources.ErrorViewModel_NoFurtherInformation;
        }

        #endregion

        #region Properties

        /// <summary>
        /// Gets or sets Details.
        /// </summary>
        public string Details
        {
            get
            {
                return string.IsNullOrEmpty(this.details) ? Resources.ErrorViewModel_NoFurtherInformation : this.details;
            }

            set
            {
                this.details = value;
                this.NotifyOfPropertyChange("Details");
            }
        }

        /// <summary>
        /// Gets or sets ErrorMessage.
        /// </summary>
        public string ErrorMessage
        {
            get
            {
                return this.errorMessage;
            }

            set
            {
                this.errorMessage = value;
                this.NotifyOfPropertyChange("ErrorMessage");
            }
        }

        /// <summary>
        /// Gets or sets Solution.
        /// </summary>
        public string Solution
        {
            get
            {
                return string.IsNullOrEmpty(this.solution) ? Resources.ErrorViewModel_IfTheProblemPersists : this.solution;
            }

            set
            {
                this.solution = value;
                this.NotifyOfPropertyChange("Solution");
            }
        }

        #endregion

        /// <summary>
        /// Close this window.
        /// </summary>
        public void Close()
        {
            try
            {
                this.TryClose();
            }
            catch (Exception e)
            {
                Console.WriteLine(e);
            }
        }

        /// <summary>
        /// Copy the Error to the clipboard.
        /// </summary>
        public void Copy()
        {
            try
            {
                Clipboard.SetDataObject(this.ErrorMessage + Environment.NewLine + this.Details, true);
            }
            catch (Exception exc)
            {
                this.errorService.ShowError(Resources.Clipboard_Unavailable, Resources.Clipboard_Unavailable_Solution, exc);
            }
        }
    }
}