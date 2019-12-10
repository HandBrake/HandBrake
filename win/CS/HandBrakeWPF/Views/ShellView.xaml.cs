// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ShellView.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Interaction logic for ShellView.xaml
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Views
{
    using System;
    using System.ComponentModel;
    using System.Drawing;
    using System.IO;
    using System.Windows;
    using System.Windows.Forms;
    using System.Windows.Input;
    using System.Windows.Resources;

    using Caliburn.Micro;

    using HandBrakeWPF.Commands;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Utilities;
    using HandBrakeWPF.ViewModels.Interfaces;

    using Application = System.Windows.Application;
    using Execute = Caliburn.Micro.Execute;

    /// <summary>
    /// Interaction logic for ShellView.xaml
    /// </summary>
    public partial class ShellView
    {
        /// <summary>
        /// The my notify icon.
        /// </summary>
        private readonly NotifyIcon notifyIcon;

        /// <summary>
        /// Initializes a new instance of the <see cref="ShellView"/> class.
        /// </summary>
        public ShellView()
        {
            this.InitializeComponent();

            IUserSettingService userSettingService = IoC.Get<IUserSettingService>();
            bool minimiseToTray = userSettingService.GetUserSetting<bool>(UserSettingConstants.MainWindowMinimize);

            if (minimiseToTray)
            {
                INotifyIconService notifyIconService = IoC.Get<INotifyIconService>();
                this.notifyIcon = new NotifyIcon();
                this.notifyIcon.ContextMenu = new ContextMenu(new[] { new MenuItem("Restore", NotifyIconClick), new MenuItem("Mini Status Display", ShowMiniStatusDisplay) });
                notifyIconService.RegisterNotifyIcon(this.notifyIcon);

                StreamResourceInfo streamResourceInfo = Application.GetResourceStream(new Uri("pack://application:,,,/handbrakepineapple.ico"));
                if (streamResourceInfo != null)
                {
                    Stream iconStream = streamResourceInfo.Stream;
                    this.notifyIcon.Icon = new Icon(iconStream);
                }

                this.notifyIcon.Click += this.NotifyIconClick;
                this.StateChanged += this.ShellViewStateChanged;
            }

            this.InputBindings.Add(new InputBinding(new ProcessShortcutCommand(new KeyGesture(Key.E, ModifierKeys.Control)), new KeyGesture(Key.E, ModifierKeys.Control))); // Start Encode
            this.InputBindings.Add(new InputBinding(new ProcessShortcutCommand(new KeyGesture(Key.K, ModifierKeys.Control)), new KeyGesture(Key.K, ModifierKeys.Control))); // Stop Encode
            this.InputBindings.Add(new InputBinding(new ProcessShortcutCommand(new KeyGesture(Key.L, ModifierKeys.Control)), new KeyGesture(Key.L, ModifierKeys.Control))); // Open Log Window
            this.InputBindings.Add(new InputBinding(new ProcessShortcutCommand(new KeyGesture(Key.Q, ModifierKeys.Control)), new KeyGesture(Key.Q, ModifierKeys.Control))); // Open Queue Window
            this.InputBindings.Add(new InputBinding(new ProcessShortcutCommand(new KeyGesture(Key.A, ModifierKeys.Control)), new KeyGesture(Key.A, ModifierKeys.Control))); // Add to Queue
            this.InputBindings.Add(new InputBinding(new ProcessShortcutCommand(new KeyGesture(Key.A, ModifierKeys.Alt)), new KeyGesture(Key.A, ModifierKeys.Alt))); // Add all to Queue
            this.InputBindings.Add(new InputBinding(new ProcessShortcutCommand(new KeyGesture(Key.A, ModifierKeys.Control | ModifierKeys.Shift)), new KeyGesture(Key.A, ModifierKeys.Control | ModifierKeys.Shift))); // Add selection to Queue

            this.InputBindings.Add(new InputBinding(new ProcessShortcutCommand(new KeyGesture(Key.O, ModifierKeys.Control)), new KeyGesture(Key.O, ModifierKeys.Control))); // File Scan
            this.InputBindings.Add(new InputBinding(new ProcessShortcutCommand(new KeyGesture(Key.O, ModifierKeys.Alt)), new KeyGesture(Key.O, ModifierKeys.Alt)));     // Scan Window
            this.InputBindings.Add(new InputBinding(new ProcessShortcutCommand(new KeyGesture(Key.O, ModifierKeys.Control | ModifierKeys.Shift)), new KeyGesture(Key.O, ModifierKeys.Control | ModifierKeys.Shift))); // Scan a Folder
            this.InputBindings.Add(new InputBinding(new ProcessShortcutCommand(new KeyGesture(Key.G, ModifierKeys.Control | ModifierKeys.Shift)), new KeyGesture(Key.G, ModifierKeys.Control | ModifierKeys.Shift))); // Garbage Colleciton
            this.InputBindings.Add(new InputBinding(new ProcessShortcutCommand(new KeyGesture(Key.F1, ModifierKeys.None)), new KeyGesture(Key.F1, ModifierKeys.None))); // Help
            this.InputBindings.Add(new InputBinding(new ProcessShortcutCommand(new KeyGesture(Key.S, ModifierKeys.Control)), new KeyGesture(Key.S, ModifierKeys.Control))); // Browse Destination

            // Tabs Switching
            this.InputBindings.Add(new InputBinding(new ProcessShortcutCommand(new KeyGesture(Key.D1, ModifierKeys.Control)), new KeyGesture(Key.D1, ModifierKeys.Control))); 
            this.InputBindings.Add(new InputBinding(new ProcessShortcutCommand(new KeyGesture(Key.D2, ModifierKeys.Control)), new KeyGesture(Key.D2, ModifierKeys.Control))); 
            this.InputBindings.Add(new InputBinding(new ProcessShortcutCommand(new KeyGesture(Key.D3, ModifierKeys.Control)), new KeyGesture(Key.D3, ModifierKeys.Control))); 
            this.InputBindings.Add(new InputBinding(new ProcessShortcutCommand(new KeyGesture(Key.D4, ModifierKeys.Control)), new KeyGesture(Key.D4, ModifierKeys.Control))); 
            this.InputBindings.Add(new InputBinding(new ProcessShortcutCommand(new KeyGesture(Key.D5, ModifierKeys.Control)), new KeyGesture(Key.D5, ModifierKeys.Control)));
            this.InputBindings.Add(new InputBinding(new ProcessShortcutCommand(new KeyGesture(Key.D6, ModifierKeys.Control)), new KeyGesture(Key.D6, ModifierKeys.Control)));
            this.InputBindings.Add(new InputBinding(new ProcessShortcutCommand(new KeyGesture(Key.D7, ModifierKeys.Control)), new KeyGesture(Key.D7, ModifierKeys.Control)));

            // Enable Windows 7 Taskbar progress indication.
            if (this.TaskbarItemInfo == null)
            {
                this.TaskbarItemInfo = Win7.WindowsTaskbar;
            }
        }

        /// <summary>
        /// Check with the user before closing.
        /// </summary>
        /// <param name="e">
        /// The CancelEventArgs.
        /// </param>
        protected override void OnClosing(CancelEventArgs e)
        {
            IShellViewModel shellViewModel = this.DataContext as IShellViewModel;

            if (shellViewModel != null)
            {
                bool canClose = shellViewModel.CanClose();
                if (!canClose)
                {
                    e.Cancel = true;
                }
            }

            if (this.notifyIcon != null)
            {
                this.notifyIcon.Visible = false;
            }

            base.OnClosing(e);
        }

        /// <summary>
        /// The show mini status display.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void ShowMiniStatusDisplay(object sender, EventArgs e)
        {
            IMiniViewModel titleSpecificView = IoC.Get<IMiniViewModel>();
            IWindowManager windowManager = IoC.Get<IWindowManager>();
            Execute.OnUIThread(
                () =>
                {
                    titleSpecificView.Activate();
                    windowManager.ShowWindow(titleSpecificView);
                });
        }

        /// <summary>
        /// The notify icon_ click.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void NotifyIconClick(object sender, EventArgs e)
        {
            this.WindowState = WindowState.Normal;

            // Bit of a hack but does work
            this.Topmost = true; 
            this.Topmost = false;
        }

        /// <summary>
        /// The shell view state changed.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void ShellViewStateChanged(object sender, EventArgs e)
        {
            if (this.notifyIcon != null)
            {
                if (this.WindowState == WindowState.Minimized)
                {
                    this.ShowInTaskbar = false;
                    notifyIcon.Visible = true;      
                }
                else if (this.WindowState == WindowState.Normal)
                {
                    notifyIcon.Visible = false;
                    this.ShowInTaskbar = true;
                }
            }
        }
    }
}
