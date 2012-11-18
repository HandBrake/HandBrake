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
    using System.IO;
    using System.Linq;
    using System.Windows;

    using Caliburn.Micro;

    using HandBrake.ApplicationServices.Exceptions;

    using HandBrakeWPF.ViewModels;
    using HandBrakeWPF.ViewModels.Interfaces;

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
                this.CurrentDomain_UnhandledException;
        }

        /// <summary>
        /// Override the startup behavior to handle files dropped on the app icon.
        /// </summary>
        /// <param name="e">
        /// The StartupEventArgs.
        /// </param>
        protected override void OnStartup(StartupEventArgs e)
        {
            base.OnStartup(e);

            // If we have a file dropped on the icon, try scanning it.
            string[] fileNames = e.Args;
            if (fileNames.Any() && (File.Exists(fileNames[0]) || Directory.Exists(fileNames[0])))
            {
                IMainViewModel mvm = IoC.Get<IMainViewModel>();
                mvm.StartScan(fileNames[0], 0);
            }
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
            if (e.ExceptionObject.GetType() == typeof(FileNotFoundException))
            {
                GeneralApplicationException exception = new GeneralApplicationException("A file appears to be missing.", "Try re-installing Microsoft .NET Framework 4.0", (Exception)e.ExceptionObject);
                this.ShowError(exception);
            }
            else
            {
                this.ShowError(e.ExceptionObject);
            }
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
            if (e.Exception.GetType() == typeof(FileNotFoundException))
            {
                GeneralApplicationException exception = new GeneralApplicationException("A file appears to be missing.", "Try re-installing Microsoft .NET Framework 4.0", e.Exception);
                this.ShowError(exception);
            }
            else if (e.Exception.GetType() == typeof(GeneralApplicationException))
            {
                this.ShowError(e.Exception);
            }
            else if (e.Exception.InnerException != null && e.Exception.InnerException.GetType() == typeof(GeneralApplicationException))
            {
                this.ShowError(e.Exception.InnerException);
            }
            else
            {
                this.ShowError(e.Exception);
            }

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
                            string details = string.Format(
                                "{0}{1}{2}{3}{4}",
                                applicationException.Error,
                                Environment.NewLine,
                                applicationException.Solution,
                                Environment.NewLine,
                                applicationException.ActualException != null
                                    ? applicationException.ActualException.ToString()
                                    : "No additional exception information available.");

                            errorView.ErrorMessage = applicationException.Error;
                            errorView.Solution = applicationException.Solution;
                            errorView.Details = details;
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
