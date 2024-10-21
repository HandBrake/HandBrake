// --------------------------------------------------------------------------------------------------------------------
// <copyright file="WindowHelper.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A helper to store a windows current state as part of the user settings service.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.AttachedProperties
{
    using System;
    using System.ComponentModel;
    using System.Diagnostics.CodeAnalysis;
    using System.Text.Json;
    using System.Windows;

    using HandBrake.Interop.Utilities;

    using HandBrakeWPF.Helpers;
    using HandBrakeWPF.Services.Interfaces;

    public class WindowExtensions
    {
        public static readonly DependencyProperty SaveProperty = DependencyProperty.RegisterAttached("SaveState", typeof(bool), typeof(WindowExtensions), new FrameworkPropertyMetadata(SaveState));

        private readonly Window appWindow;
        private readonly IUserSettingService userSettingService = IoCHelper.Get<IUserSettingService>();

        public WindowExtensions(Window appWindow)
        {
            this.appWindow = appWindow;
        }

        public static void SetSaveState(DependencyObject sender, bool isEnabled)
        {
            sender.SetValue(SaveProperty, isEnabled);
        }

        private static void SaveState(DependencyObject sender, DependencyPropertyChangedEventArgs eventArgs)
        {
            if (sender is Window window)
            {
                if ((bool)eventArgs.NewValue)
                {
                    var settings = new WindowExtensions(window);
                    settings.Attach();
                }
            }
        }

        private void Attach()
        {
            if (this.appWindow != null)
            {
                this.appWindow.Closing += this.WindowClosing;
                this.appWindow.Initialized += this.WindowInitialized;
                this.appWindow.Loaded += this.WindowLoaded;
            }
        }

        private void WindowClosing(object sender, CancelEventArgs cancelEventArgs)
        {
            string key = string.Format("{0}.Settings", this.appWindow.Name);
            WindowInformation information = new WindowInformation(this.appWindow.Name, this.appWindow.WindowState, this.appWindow.RestoreBounds);

            string json = JsonSerializer.Serialize(information, JsonSettings.Options);
            if (!string.IsNullOrEmpty(json))
            {
                this.userSettingService.SetUserSetting(key, json);
            }
        }

        private void WindowInitialized(object sender, EventArgs eventArgs)
        {
            string key = string.Format("{0}.Settings", this.appWindow.Name);
            string json = this.userSettingService.GetUserSetting<string>(key);
            if (!string.IsNullOrEmpty(json))
            {
                WindowInformation settings = JsonSerializer.Deserialize<WindowInformation>(json, JsonSettings.Options);

                if (settings.Location != Rect.Empty)
                {
                    // We might use these in the future
                    this.appWindow.Left = settings.Location.Left;
                    this.appWindow.Top = settings.Location.Top;
                    this.appWindow.Width = settings.Location.Width;
                    this.appWindow.Height = settings.Location.Height;
                }

                if (settings.WindowState != WindowState.Maximized)
                {
                    this.appWindow.WindowState = settings.WindowState;
                }
            }
        }

        private void WindowLoaded(object sender, RoutedEventArgs routedEventArgs)
        {
            string key = string.Format("{0}.Settings", this.appWindow.Name);
            string json = this.userSettingService.GetUserSetting<string>(key);
            if (!string.IsNullOrEmpty(json))
            {
                WindowInformation settings = JsonSerializer.Deserialize<WindowInformation>(json, JsonSettings.Options);
                this.appWindow.WindowState = settings.WindowState;
            }
        }
    }

    /// <summary>
    /// An object we can store as JSON against a user setting key.
    /// </summary>
    public class WindowInformation
    {
        public WindowInformation(string windowName, WindowState windowState, Rect location)
        {
            this.WindowName = windowName;
            this.WindowState = windowState;
            this.Location = location;
        }

        public string WindowName { get; set; }
        public WindowState WindowState { get; set; }
        public Rect Location { get; set; }
    }
}