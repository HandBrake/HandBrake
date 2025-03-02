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
    using System;
    using System.Windows;

    using HandBrakeWPF.Controls;
    using HandBrakeWPF.Helpers;
    using HandBrakeWPF.Model;
    using HandBrakeWPF.Utilities;
    using HandBrakeWPF.Views;

    using Interfaces;
    using ViewModels.Interfaces;

    /// <summary>
    /// The Error Service
    /// </summary>
    public class ErrorService : IErrorService
    {
        private readonly IUserSettingService userSettingService;

        public ErrorService(IUserSettingService userSettingService)
        {
            this.userSettingService = userSettingService;
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
            IWindowManager windowManager = IoCHelper.Get<IWindowManager>();
            IErrorViewModel errorViewModel = IoCHelper.Get<IErrorViewModel>();

            if (windowManager != null && errorViewModel != null)
            {
                errorViewModel.ErrorMessage = message;
                errorViewModel.Solution = solution;
                errorViewModel.Details = details;
                windowManager.ShowDialog<ErrorView>(errorViewModel);
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
            IWindowManager windowManager = IoCHelper.Get<IWindowManager>();
            IErrorViewModel errorViewModel = IoCHelper.Get<IErrorViewModel>();

            if (windowManager != null && errorViewModel != null)
            {
                errorViewModel.ErrorMessage = message;
                errorViewModel.Solution = solution;
                errorViewModel.Details = exception.ToString();
                windowManager.ShowDialog<ErrorView>(errorViewModel);
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
        /// <param name="image">
        /// The image.
        /// </param>
        /// <returns>
        /// The MessageBoxResult Object
        /// </returns>
        public MessageBoxResult ShowMessageBox(string message, string header, MessageBoxButton buttons, MessageBoxImage image)
        {
            DarkThemeMode mode = userSettingService.GetUserSetting<DarkThemeMode>(UserSettingConstants.DarkThemeMode);
            if (mode == DarkThemeMode.Dark || (mode == DarkThemeMode.System && SystemInfo.IsAppsUsingDarkTheme()))
            {
                MessageBoxWindow window = new MessageBoxWindow();
                window.Setup(header, message, buttons, image);
                if (Application.Current.MainWindow != null && Application.Current.MainWindow.IsActive)
                {
                    window.Owner = Application.Current.MainWindow;
                    window.WindowStartupLocation = WindowStartupLocation.CenterOwner;
                }
            
                window.ShowDialog();

                return window.MessageBoxResult;
            }

            return MessageBox.Show(message, header, buttons, image);
        }
    }
}
