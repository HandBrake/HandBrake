// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ErrorService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Error Service
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Services
{
    using System;
    using Caliburn.Micro;
    using HandBrake.Model.Prompts;
    using HandBrake.Services.Interfaces;
    using HandBrake.Utilities.Interfaces;
    using ViewModels.Interfaces;

    /// <summary>
    /// The Error Service
    /// </summary>
    public class ErrorService : IErrorService
    {
        private readonly IViewManager viewManager;
        private readonly IDialogService dialogService;

        /// <summary>
        /// Initializes a new instance of the <see cref="ErrorService"/> class.
        /// </summary>
        /// <param name="viewManager">
        /// The View Manager.
        /// </param>
        /// <param name="dialogService">
        /// The Dialog service.
        /// </param>
        public ErrorService(IViewManager viewManager, IDialogService dialogService)
        {
            this.viewManager = viewManager;
            this.dialogService = dialogService;
        }

        /// <summary>
        /// Show an Exception Error Window
        /// </summary>
        /// <param name="message">
        /// The message.
        /// </param>
        /// <param name="solution">
        /// The solution.
        /// </param>
        /// <param name="details">
        /// The details.
        /// </param>
        public void ShowError(string message, string solution, string details)
        {
            IErrorViewModel errorViewModel = IoC.Get<IErrorViewModel>();

            if (this.viewManager != null && errorViewModel != null)
            {
                errorViewModel.ErrorMessage = message;
                errorViewModel.Solution = solution;
                errorViewModel.Details = details;
                this.viewManager.ShowDialog(errorViewModel);
            }
        }

        /// <summary>
        /// Show an Exception Error Window
        /// </summary>
        /// <param name="message">
        /// The message.
        /// </param>
        /// <param name="solution">
        /// The solution.
        /// </param>
        /// <param name="exception">
        /// The exception.
        /// </param>
        public void ShowError(string message, string solution, Exception exception)
        {
            IErrorViewModel errorViewModel = IoC.Get<IErrorViewModel>();

            if (this.viewManager != null && errorViewModel != null)
            {
                errorViewModel.ErrorMessage = message;
                errorViewModel.Solution = solution;
                errorViewModel.Details = exception.ToString();
                this.viewManager.ShowDialog(errorViewModel);
            }
        }

        /// <summary>
        /// Show a Message Box.
        /// It is good practice to use this, so that if we ever introduce unit testing, the message boxes won't cause issues.
        /// </summary>
        /// <param name="message">
        /// The message.
        /// </param>
        /// <param name="header">
        /// The header.
        /// </param>
        /// <param name="buttons">
        /// The buttons.
        /// </param>
        /// <param name="type">
        /// The type.
        /// </param>
        /// <returns>
        /// The MessageBoxResult Object
        /// </returns>
        public DialogResult ShowMessageBox(string message, string header, DialogButtonType buttons, DialogType type)
        {
            return this.dialogService.Show(message, header, buttons, type);
        }
    }
}