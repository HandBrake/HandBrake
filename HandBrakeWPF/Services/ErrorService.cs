// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ErrorService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Error Service
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services
{
    using System.Windows;
    using Interfaces;
    using Caliburn.Micro;
    using ViewModels.Interfaces;

    /// <summary>
    /// The Error Service
    /// </summary>
    public class ErrorService : IErrorService
    {
        /// <summary>
        /// Show an Exception Error Window.
        /// </summary>
        /// <param name="message"></param>
        /// <param name="solution"></param>
        /// <param name="details"></param>
        public void ShowError(string message, string solution, string details)
        {
            IWindowManager windowManager = IoC.Get<IWindowManager>();
            IErrorViewModel errorViewModel = IoC.Get<IErrorViewModel>();

            if (windowManager != null && errorViewModel != null)
            {
                errorViewModel.ErrorMessage = message;
                errorViewModel.Solution = solution;
                errorViewModel.Details = details;
                windowManager.ShowDialog(errorViewModel);
            }
        }

        /// <summary>
        /// Show a Message Box. 
        /// It is good practice to use this, so that if we ever introduce unit testing, the message boxes won't cause issues.
        /// </summary>
        /// <param name="message"></param>
        /// <param name="header"></param>
        /// <param name="image"></param>
        /// <param name="buttons"></param>
        /// <returns></returns>
        public MessageBoxResult ShowMessageBox(string message, string header, MessageBoxButton buttons, MessageBoxImage image)
        {
            return MessageBox.Show(message, header, buttons, image);
        }
    }
}
