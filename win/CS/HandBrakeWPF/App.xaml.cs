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
    using System.Windows.Interop;
    using System.Windows.Media;

    using HandBrake.App.Core.Utilities;
    using HandBrake.Interop.Interop;

    using HandBrakeWPF.Helpers;
    using HandBrakeWPF.Instance;
    using HandBrakeWPF.Model;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Startup;
    using HandBrakeWPF.Utilities;
    using HandBrakeWPF.ViewModels;
    using HandBrakeWPF.ViewModels.Interfaces;
    using HandBrakeWPF.Views;

    using GeneralApplicationException = HandBrake.App.Core.Exceptions.GeneralApplicationException;
    using IWindowManager = Services.Interfaces.IWindowManager;

    /// <summary>
    /// Interaction logic for App.xaml
    /// </summary>
    public partial class App
    {
        private bool isStarted = false;

        /// <summary>
        /// Initializes a new instance of the <see cref="App"/> class.
        /// </summary>
        public App()
        {
            Application.Current.Dispatcher.UnhandledException += this.Dispatcher_UnhandledException;
            AppDomain.CurrentDomain.UnhandledException += this.CurrentDomain_UnhandledException;
            AppDomain.CurrentDomain.ProcessExit += this.CurrentDomain_ProcessExit;
            
            ToolTipService.ShowDurationProperty.OverrideMetadata(typeof(DependencyObject), new FrameworkPropertyMetadata(15000));
        }

        private void Init(StartupEventArgs e)
        {
            if (!File.Exists("hb.dll") && !File.Exists(Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "hb.dll")))
            {
                MessageBox.Show("hb.dll file not found. Application will not run correctly without this. Please re-install HandBrake.", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                this.Shutdown();
                Environment.Exit(-1);
                return;
            }

            // We don't support Windows earlier than 10.
            if (!SystemInfo.IsWindows10OrLater())
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
                int portableInit = Portable.Initialise();
                if (portableInit != 0)
                {
                    switch (portableInit)
                    {
                        case -1:
                            MessageBox.Show(
                                HandBrakeWPF.Properties.Resources.Portable_IniFileError,
                                HandBrakeWPF.Properties.Resources.Error,
                                MessageBoxButton.OK,
                                MessageBoxImage.Error);
                            break;

                        case -2:
                            MessageBox.Show(
                                HandBrakeWPF.Properties.Resources.Portable_TmpNotWritable,
                                HandBrakeWPF.Properties.Resources.Error,
                                MessageBoxButton.OK,
                                MessageBoxImage.Error);
                            break;
                        case -3:
                            MessageBox.Show(
                                HandBrakeWPF.Properties.Resources.Portable_StorageNotWritable,
                                HandBrakeWPF.Properties.Resources.Error,
                                MessageBoxButton.OK,
                                MessageBoxImage.Error);
                            break;
                    }

                    Application.Current.Shutdown();
                    return;
                }
            }

            // Setup the UI
            this.StartupUri = new Uri("Views/ShellView.xaml", UriKind.Relative);
            this.isStarted = true;

            // Setup the UI Language
            IUserSettingService userSettingService = IoCHelper.Get<IUserSettingService>();
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

            int runCounter = userSettingService.GetUserSetting<int>(UserSettingConstants.RunCounter);

            // Software Rendering 
            if (e.Args.Any(f => f.Equals("--force-software-rendering")) || Portable.IsForcingSoftwareRendering() || userSettingService.GetUserSetting<bool>(UserSettingConstants.ForceSoftwareRendering))
            {
                RenderOptions.ProcessRenderMode = RenderMode.SoftwareOnly;
            }

            // Check if the user would like to check for updates AFTER the first run, but only once. 
            if (runCounter == 1)
            {
                CheckForUpdateCheckPermission(userSettingService);
            }

            // Increment the counter so we can change startup behavior for the above warning and update check question.
            userSettingService.SetUserSetting(UserSettingConstants.RunCounter, runCounter + 1); // Only display once.

            // App Theme
            DarkThemeMode useDarkTheme = (DarkThemeMode)userSettingService.GetUserSetting<int>(UserSettingConstants.DarkThemeMode);
            ResourceDictionary dark = new ResourceDictionary { Source = new Uri("pack://application:,,,/MahApps.Metro;component/Styles/Themes/Dark.Blue.xaml") };
            ResourceDictionary light = new ResourceDictionary { Source = new Uri("pack://application:,,,/MahApps.Metro;component/Styles/Themes/Light.Blue.xaml") };

            Application.Current.Resources.MergedDictionaries.Add(new ResourceDictionary { Source = new Uri("Themes/Generic.xaml", UriKind.Relative) });
            bool themed = false;
            if (SystemParameters.HighContrast || !Portable.IsThemeEnabled())
            {
                Application.Current.Resources["Ui.Light"] = new SolidColorBrush(SystemColors.HighlightTextColor);
                Application.Current.Resources["Ui.ContrastLight"] = new SolidColorBrush(SystemColors.ActiveBorderBrush.Color);
                useDarkTheme = DarkThemeMode.None;
            }

            switch (useDarkTheme)
            {
                case DarkThemeMode.System:
                    if (SystemInfo.IsAppsUsingDarkTheme())
                    {
                        Application.Current.Resources.MergedDictionaries.Add(dark);
                        Application.Current.Resources.MergedDictionaries.Add(new ResourceDictionary { Source = new Uri("Themes/Dark.xaml", UriKind.Relative) });
                    }
                    else
                    {
                        Application.Current.Resources.MergedDictionaries.Add(light);
                        Application.Current.Resources.MergedDictionaries.Add(new ResourceDictionary { Source = new Uri("Themes/Light.xaml", UriKind.Relative) });
                    }

                    themed = true;
                    break;
                case DarkThemeMode.Dark:
                    Application.Current.Resources.MergedDictionaries.Add(dark);
                    Application.Current.Resources.MergedDictionaries.Add(new ResourceDictionary { Source = new Uri("Themes/Dark.xaml", UriKind.Relative) });
                    themed = true;
                    break;
                case DarkThemeMode.Light:
                    Application.Current.Resources.MergedDictionaries.Add(light);
                    Application.Current.Resources.MergedDictionaries.Add(new ResourceDictionary { Source = new Uri("Themes/Light.xaml", UriKind.Relative) });
                    themed = true;
                    break;

                case DarkThemeMode.None:
                    Application.Current.Resources["Ui.Light"] = new SolidColorBrush(SystemColors.HighlightTextColor);
                    Application.Current.Resources["Ui.ContrastLight"] = new SolidColorBrush(SystemColors.ActiveBorderBrush.Color);
                    themed = false;
                    break;
            }

            Application.Current.Resources.MergedDictionaries.Add(new ResourceDictionary { Source = new Uri("Views/Styles/Styles.xaml", UriKind.Relative) });

            if (themed)
            {
                Application.Current.Resources.MergedDictionaries.Add(new ResourceDictionary { Source = new Uri("Views/Styles/ThemedStyles.xaml", UriKind.Relative) });
            }

            // NO-Hardware Mode
            bool noHardware = e.Args.Any(f => f.Equals("--no-hardware")) || (Portable.IsPortable() && !Portable.IsHardwareEnabled());

            // Initialise the Engine
            Services.Logging.GlobalLoggingManager.Init();
            HandBrakeInstanceManager.Init(noHardware, userSettingService);

            // If we have a file dropped on the icon, try scanning it.
            string[] args = e.Args;
            if (args.Any() && (File.Exists(args[0]) || Directory.Exists(args[0])))
            {
                IMainViewModel mvm = IoCHelper.Get<IMainViewModel>();
                mvm.StartScan(new List<string> { args[0] }, 0);
            }
        }
        
        private static void CheckForUpdateCheckPermission(IUserSettingService userSettingService)
        {
            if (Portable.IsPortable() && !Portable.IsUpdateCheckEnabled())
            {
                return; // If Portable Mode has disabled it, don't bother the user. Just accept it's disabled. 
            }

            MessageBoxResult result = MessageBox.Show(HandBrakeWPF.Properties.Resources.FirstRun_EnableUpdateCheck, HandBrakeWPF.Properties.Resources.FirstRun_EnableUpdateCheckHeader, MessageBoxButton.YesNo, MessageBoxImage.Question);
            // Be explicit setting it to true/false as it may have been turned on during first-run.
            userSettingService.SetUserSetting(UserSettingConstants.UpdateStatus, result == MessageBoxResult.Yes);
        }

        private void CurrentDomain_ProcessExit(object sender, System.EventArgs e)
        {
            if (this.isStarted)
            {
                HandBrakeUtils.DisposeGlobal();
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
            ThreadHelper.OnUIThread(
                () => 
            {
                if (e.ExceptionObject.GetType() == typeof(FileNotFoundException))
                {
                    GeneralApplicationException exception = new GeneralApplicationException(
                        "A file appears to be missing.",
                        "Try re-installing Microsoft .NET 8 Desktop Runtime",
                        (Exception)e.ExceptionObject);
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
                GeneralApplicationException exception = new GeneralApplicationException("A file appears to be missing.", "Try re-installing Microsoft .NET 8 Desktop Runtime", e.Exception);
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
                IWindowManager windowManager = IoCHelper.Get<IWindowManager>();
                IErrorService errorService = IoCHelper.Get<IErrorService>();
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
                        windowManager.ShowDialog<ErrorView>(errorView);
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
                MessageBox.Show("An Unknown Error has occurred. \n\n Exception:" + exception, "Unhandled Exception", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        /// <summary>
        /// Override the startup behavior to handle files dropped on the app icon.
        /// </summary>
        /// <param name="e">
        /// The StartupEventArgs.
        /// </param>
        private void App_OnStartup(object sender, StartupEventArgs e)
        {
            this.Init(e);
        }
    }
}
