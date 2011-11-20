// --------------------------------------------------------------------------------------------------------------------
// <copyright file="App.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Interaction logic for App.xaml
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF
{
    using System;
    using System.Windows;

    using Caliburn.Micro;

    using HandBrake.ApplicationServices.Exceptions;

    using HandBrakeWPF.ViewModels;

    /// <summary>
    /// Interaction logic for App.xaml
    /// </summary>
    public partial class App
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="App"/> class.
        /// </summary>
        public App()
        {
            Application.Current.Dispatcher.UnhandledException += this.Dispatcher_UnhandledException;
            AppDomain.CurrentDomain.UnhandledException +=
                new UnhandledExceptionEventHandler(CurrentDomain_UnhandledException);
        }

        /// <summary>
        /// Non-UI Thread expection handler.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The UnhandledExceptionEventArgs.
        /// </param>
        private void CurrentDomain_UnhandledException(object sender, UnhandledExceptionEventArgs e)
        {
            this.ShowError(e.ExceptionObject);
        }

        /// <summary>
        /// Handle unhandled exceptions. UI thread only.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The DispatcherUnhandledExceptionEventArgs.
        /// </param>
        private void Dispatcher_UnhandledException(
            object sender, System.Windows.Threading.DispatcherUnhandledExceptionEventArgs e)
        {
            this.ShowError(e.Exception);
            e.Handled = true;
        }

        /// <summary>
        /// Show an error dialog for the user.
        /// </summary>
        /// <param name="exception">
        /// The exception.
        /// </param>
        private void ShowError(object exception)
        {
            try
            {
                IWindowManager windowManager = IoC.Get<IWindowManager>();
                if (windowManager != null)
                {
                    ErrorViewModel errorView = new ErrorViewModel();

                    if (exception.GetType() == typeof(GeneralApplicationException))
                    {
                        GeneralApplicationException applicationException = exception as GeneralApplicationException;
                        if (applicationException != null)
                        {
                            errorView.ErrorMessage = applicationException.Error;
                            errorView.Solution = applicationException.Solution;
                            errorView.Details = applicationException.ActualException.ToString();
                        }
                    }
                    else
                    {
                        errorView.Details = exception.ToString();
                    }

                    windowManager.ShowDialog(errorView);
                }
            }
            catch (Exception)
            {
                MessageBox.Show("An Unknown Error has occured. \n\n Exception:" + exception, "Unhandled Exception",
                     MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }
    }
}
