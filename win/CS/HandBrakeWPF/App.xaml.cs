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
    using System.Collections.Generic;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Threading;
    using System.Windows;
    using System.Windows.Controls;

    using Caliburn.Micro;

    using HandBrake.Interop.Interop;

    using HandBrakeWPF.Helpers;
    using HandBrakeWPF.Instance;
    using HandBrakeWPF.Model;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Startup;
    using HandBrakeWPF.Utilities;
    using HandBrakeWPF.ViewModels;
    using HandBrakeWPF.ViewModels.Interfaces;

    using GeneralApplicationException = HandBrakeWPF.Exceptions.GeneralApplicationException;

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

            ToolTipService.ShowDurationProperty.OverrideMetadata(typeof(DependencyObject), new FrameworkPropertyMetadata(15000));
        }

        /// <summary>
        /// Override the startup behavior to handle files dropped on the app icon.
        /// </summary>
        /// <param name="e">
        /// The StartupEventArgs.
        /// </param>
        protected override void OnStartup(StartupEventArgs e)
        {
            // We don't support Windows XP / 2003 / 2003 R2 / Vista / 2008
            OperatingSystem os = Environment.OSVersion;
            if (((os.Platform == PlatformID.Win32NT) && (os.Version.Major == 5)) || ((os.Platform == PlatformID.Win32NT) && (os.Version.Major == 6 && os.Version.Minor < 1)))
            {
                MessageBox.Show(HandBrakeWPF.Properties.Resources.OsVersionWarning, HandBrakeWPF.Properties.Resources.Warning, MessageBoxButton.OK, MessageBoxImage.Warning);
                Application.Current.Shutdown();
                return;
            }

            if (!Environment.Is64BitOperatingSystem)
            {
                MessageBox.Show(HandBrakeWPF.Properties.Resources.OsBitnessWarning, HandBrakeWPF.Properties.Resources.Warning, MessageBoxButton.OK, MessageBoxImage.Warning);
                Application.Current.Shutdown();
                return;
            }

            if (e.Args.Any(f => f.Equals("--reset")))
            {
                HandBrakeApp.ResetToDefaults();
                Application.Current.Shutdown();
                return;
            }

            if (e.Args.Any(f => f.StartsWith("--recover-queue-ids")))
            {
                string command = e.Args.FirstOrDefault(f => f.StartsWith("--recover-queue-ids"));
                if (!string.IsNullOrEmpty(command))
                {
                    command = command.Replace("--recover-queue-ids=", string.Empty);
                    List<string> processIds = command.Split(',').ToList();
                    StartupOptions.QueueRecoveryIds = processIds;
                }
            }
            
            if (e.Args.Any(f => f.Equals("--auto-start-queue")))
            {
                StartupOptions.AutoRestartQueue = true;
            }

            // Portable Mode
            if (Portable.IsPortable())
            {
                if (!Portable.Initialise())
                {
                    Application.Current.Shutdown();
                    return;
                }
            }

            // Setup the UI Language
            IUserSettingService userSettingService = IoC.Get<IUserSettingService>();
            string culture = userSettingService.GetUserSetting<string>(UserSettingConstants.UiLanguage);
            if (!string.IsNullOrEmpty(culture))
            {
                InterfaceLanguage language = InterfaceLanguageUtilities.FindInterfaceLanguage(culture);
                if (language != null)
                {
                    CultureInfo ci = new CultureInfo(language.Culture);
                    Thread.CurrentThread.CurrentUICulture = ci;
                }
            }

            bool useDarkTheme = userSettingService.GetUserSetting<bool>(UserSettingConstants.UseDarkTheme);
            if (useDarkTheme && SystemInfo.IsWindows10())
            {
                ResourceDictionary darkTheme = new ResourceDictionary();
                darkTheme.Source = new Uri("Themes/Dark.xaml", UriKind.Relative);
                Application.Current.Resources.MergedDictionaries.Add(darkTheme);
            }

            // NO-Hardware Mode
            bool noHardware = e.Args.Any(f => f.Equals("--no-hardware"))  || (Portable.IsPortable() && !Portable.IsHardwareEnabled());

            // Initialise the Engine
            HandBrakeWPF.Helpers.LogManager.Init();

            try
            {
                HandBrakeInstanceManager.Init(noHardware);
            }
            catch (Exception exception)
            {
                if (!noHardware)
                {
                    MessageBox.Show(HandBrakeWPF.Properties.Resources.Startup_InitFailed, HandBrakeWPF.Properties.Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
                }

                throw exception;
            }

            // Initialise the GUI
            base.OnStartup(e);

            // If we have a file dropped on the icon, try scanning it.
            string[] args = e.Args;
            if (args.Any() && (File.Exists(args[0]) || Directory.Exists(args[0])))
            {
                IMainViewModel mvm = IoC.Get<IMainViewModel>();
                mvm.StartScan(args[0], 0);
            }
        }

        /// <summary>
        /// Non-UI Thread exception handler.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The UnhandledExceptionEventArgs.
        /// </param>
        private void CurrentDomain_UnhandledException(object sender, UnhandledExceptionEventArgs e)
        {
            Caliburn.Micro.Execute.OnUIThreadAsync(() => {
                if (e.ExceptionObject.GetType() == typeof(FileNotFoundException))
                {
                    GeneralApplicationException exception = new GeneralApplicationException("A file appears to be missing.", "Try re-installing Microsoft .NET Framework 4.8", (Exception)e.ExceptionObject);
                    this.ShowError(exception);
                }
                else
                {
                    this.ShowError(e.ExceptionObject);
                }
            });
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
                GeneralApplicationException exception = new GeneralApplicationException("A file appears to be missing.", "Try re-installing Microsoft .NET Framework 4.7.1", e.Exception);
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
                IErrorService errorService = IoC.Get<IErrorService>();
                if (windowManager != null)
                {
                    ErrorViewModel errorView = new ErrorViewModel(errorService);
                    GeneralApplicationException applicationException = null;
                    if (exception.GetType() == typeof(GeneralApplicationException))
                    {
                        applicationException = exception as GeneralApplicationException;
                        if (applicationException != null)
                        {
                            string details = string.Format(
                                "{0}{1}{2}{3}{4}",
                                applicationException.Error,
                                Environment.NewLine,
                                applicationException.Solution,
                                Environment.NewLine,
                                applicationException.ActualException != null ? applicationException.ActualException.ToString() : "No additional exception information available.");

                            errorView.ErrorMessage = applicationException.Error;
                            errorView.Solution = applicationException.Solution;
                            errorView.Details = details;
                        }
                    }
                    else
                    {
                        errorView.Details = exception.ToString();
                    }

                    try
                    {
                        windowManager.ShowDialog(errorView);
                    }
                    catch (Exception)
                    {
                        if (applicationException != null)
                        {
                            MessageBox.Show(applicationException.Error + Environment.NewLine + Environment.NewLine + applicationException.Solution, "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                        }
                    }
                }
            }
            catch (Exception)
            {
                MessageBox.Show("An Unknown Error has occurred. \n\n Exception:" + exception, "Unhandled Exception",
                     MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }
    }
}
