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
    using System.ComponentModel.Composition;
    using System.Windows;

    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    /// The Error View Model
    /// </summary>
    [Export(typeof(IErrorViewModel))]
    public class ErrorViewModel : ViewModelBase, IErrorViewModel
    {
        #region Constants and Fields

        /// <summary>
        /// The details.
        /// </summary>
        private string details;

        /// <summary>
        /// The error message.
        /// </summary>
        private string errorMessage;

        /// <summary>
        /// The solution.
        /// </summary>
        private string solution;

        #endregion

        #region Constructors and Destructors

        /// <summary>
        /// Initializes a new instance of the <see cref="ErrorViewModel"/> class.
        /// </summary>
        public ErrorViewModel()
        {
            this.Title = "Error";
            this.ErrorMessage = "An Unknown Error has occured.";
            this.Details = "There is no further information available about this error.";
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
                return string.IsNullOrEmpty(this.details) ? "There is no further information available about this error." : this.details;
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
                return string.IsNullOrEmpty(this.solution) ? "If the problem presists, please try restarting HandBrake." : this.solution;
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
            Clipboard.SetDataObject(this.ErrorMessage + Environment.NewLine + this.Details, true);
        }
    }
}