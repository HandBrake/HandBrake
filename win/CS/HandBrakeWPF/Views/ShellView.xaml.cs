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
    using System.Windows.Controls;
    using System.Windows.Input;
    using System.Windows.Resources;

    using HandBrakeWPF.Commands;
    using HandBrakeWPF.Helpers;
    using HandBrakeWPF.Model.Options;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Utilities;
    using HandBrakeWPF.ViewModels;
    using HandBrakeWPF.ViewModels.Interfaces;

    using Application = System.Windows.Application;
    using FlowDirection = System.Windows.FlowDirection;

    /// <summary>
    /// Interaction logic for ShellView.xaml
    /// </summary>
    public partial class ShellView : Window
    {
        private INotifyIconService notifyIconService;

        protected override void OnSourceInitialized(EventArgs e)
        {
            base.OnSourceInitialized(e);
            WindowHelper.SetDarkMode(this);
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ShellView"/> class.
        /// </summary>
        public ShellView()
        {
            this.DataContext = IoCHelper.Get<IShellViewModel>();

            this.InitializeComponent();

            IUserSettingService userSettingService = IoCHelper.Get<IUserSettingService>();
            bool minimiseToTray = userSettingService.GetUserSetting<bool>(UserSettingConstants.MainWindowMinimize);

            if (minimiseToTray)
            {
                StreamResourceInfo streamResourceInfo = Application.GetResourceStream(new Uri("pack://application:,,,/handbrakepineapple.ico"));
                if (streamResourceInfo != null)
                {
                    Stream iconStream = streamResourceInfo.Stream;

                    notifyIconService = IoCHelper.Get<INotifyIconService>();
                    notifyIconService.Setup(new Icon(iconStream));
                    this.notifyIconService.SetClickCallback(() => this.NotifyIconClick());
                }

                this.StateChanged += this.ShellViewStateChanged;
            }

            this.InputBindings.Add(new InputBinding(new ProcessShortcutCommand(new KeyGesture(Key.E, ModifierKeys.Control)), new KeyGesture(Key.E, ModifierKeys.Control))); // Start Encode
            this.InputBindings.Add(new InputBinding(new ProcessShortcutCommand(new KeyGesture(Key.P, ModifierKeys.Alt)), new KeyGesture(Key.P, ModifierKeys.Alt))); // Pause Encode
            this.InputBindings.Add(new InputBinding(new ProcessShortcutCommand(new KeyGesture(Key.K, ModifierKeys.Control)), new KeyGesture(Key.K, ModifierKeys.Control))); // Stop Encode
            this.InputBindings.Add(new InputBinding(new ProcessShortcutCommand(new KeyGesture(Key.L, ModifierKeys.Control)), new KeyGesture(Key.L, ModifierKeys.Control))); // Open Log Window
            this.InputBindings.Add(new InputBinding(new ProcessShortcutCommand(new KeyGesture(Key.Q, ModifierKeys.Control)), new KeyGesture(Key.Q, ModifierKeys.Control))); // Open Queue Window
            this.InputBindings.Add(new InputBinding(new ProcessShortcutCommand(new KeyGesture(Key.A, ModifierKeys.Alt)), new KeyGesture(Key.A, ModifierKeys.Alt))); // Add to Queue
            this.InputBindings.Add(new InputBinding(new ProcessShortcutCommand(new KeyGesture(Key.A, ModifierKeys.Control | ModifierKeys.Alt)), new KeyGesture(Key.A, ModifierKeys.Control | ModifierKeys.Alt))); // Add all to Queue
            this.InputBindings.Add(new InputBinding(new ProcessShortcutCommand(new KeyGesture(Key.A, ModifierKeys.Control | ModifierKeys.Shift)), new KeyGesture(Key.A, ModifierKeys.Control | ModifierKeys.Shift))); // Add selection to Queue

            this.InputBindings.Add(new InputBinding(new ProcessShortcutCommand(new KeyGesture(Key.O, ModifierKeys.Control)), new KeyGesture(Key.O, ModifierKeys.Control))); // File Scan
            this.InputBindings.Add(new InputBinding(new ProcessShortcutCommand(new KeyGesture(Key.O, ModifierKeys.Alt)), new KeyGesture(Key.O, ModifierKeys.Alt))); // Scan Window
            this.InputBindings.Add(new InputBinding(new ProcessShortcutCommand(new KeyGesture(Key.O, ModifierKeys.Control | ModifierKeys.Shift)), new KeyGesture(Key.O, ModifierKeys.Control | ModifierKeys.Shift))); // Scan a Folder
            this.InputBindings.Add(new InputBinding(new ProcessShortcutCommand(new KeyGesture(Key.G, ModifierKeys.Control | ModifierKeys.Shift)), new KeyGesture(Key.G, ModifierKeys.Control | ModifierKeys.Shift))); // Garbage Collection
            this.InputBindings.Add(new InputBinding(new ProcessShortcutCommand(new KeyGesture(Key.F1, ModifierKeys.None)), new KeyGesture(Key.F1, ModifierKeys.None))); // Help
            this.InputBindings.Add(new InputBinding(new ProcessShortcutCommand(new KeyGesture(Key.S, ModifierKeys.Control)), new KeyGesture(Key.S, ModifierKeys.Control))); // Browse Destination

            this.InputBindings.Add(new InputBinding(new ProcessShortcutCommand(new KeyGesture(Key.OemPlus, ModifierKeys.Control)), new KeyGesture(Key.OemPlus, ModifierKeys.Control))); // Next Title
            this.InputBindings.Add(new InputBinding(new ProcessShortcutCommand(new KeyGesture(Key.OemMinus, ModifierKeys.Control)), new KeyGesture(Key.OemMinus, ModifierKeys.Control))); // Previous Title
            this.InputBindings.Add(new InputBinding(new ProcessShortcutCommand(new KeyGesture(Key.Add, ModifierKeys.Control)), new KeyGesture(Key.Add, ModifierKeys.Control))); // Next Title
            this.InputBindings.Add(new InputBinding(new ProcessShortcutCommand(new KeyGesture(Key.Subtract, ModifierKeys.Control)), new KeyGesture(Key.Subtract, ModifierKeys.Control))); // Previous Title

            // Tabs Switching
            this.InputBindings.Add(new InputBinding(new ProcessShortcutCommand(new KeyGesture(Key.D1, ModifierKeys.Control)), new KeyGesture(Key.D1, ModifierKeys.Control))); 
            this.InputBindings.Add(new InputBinding(new ProcessShortcutCommand(new KeyGesture(Key.D2, ModifierKeys.Control)), new KeyGesture(Key.D2, ModifierKeys.Control))); 
            this.InputBindings.Add(new InputBinding(new ProcessShortcutCommand(new KeyGesture(Key.D3, ModifierKeys.Control)), new KeyGesture(Key.D3, ModifierKeys.Control))); 
            this.InputBindings.Add(new InputBinding(new ProcessShortcutCommand(new KeyGesture(Key.D4, ModifierKeys.Control)), new KeyGesture(Key.D4, ModifierKeys.Control))); 
            this.InputBindings.Add(new InputBinding(new ProcessShortcutCommand(new KeyGesture(Key.D5, ModifierKeys.Control)), new KeyGesture(Key.D5, ModifierKeys.Control)));
            this.InputBindings.Add(new InputBinding(new ProcessShortcutCommand(new KeyGesture(Key.D6, ModifierKeys.Control)), new KeyGesture(Key.D6, ModifierKeys.Control)));
            this.InputBindings.Add(new InputBinding(new ProcessShortcutCommand(new KeyGesture(Key.D7, ModifierKeys.Control)), new KeyGesture(Key.D7, ModifierKeys.Control)));

            // Enable Windows Taskbar progress indication.
            if (this.TaskbarItemInfo == null)
            {
                this.TaskbarItemInfo = WindowsTaskbar.GetTaskBar();
            }

            // Setup the Right To Left Mode
            RightToLeftMode rightToLeft = (RightToLeftMode)userSettingService.GetUserSetting<int>(UserSettingConstants.RightToLeftUi);
            switch (rightToLeft)
            {
                case RightToLeftMode.EntireInterface:
                    if (Application.Current.MainWindow != null)
                    {
                        Application.Current.MainWindow.FlowDirection = FlowDirection.RightToLeft;
                    }
                    break;
                case RightToLeftMode.TextOnly:
                    FrameworkElement.FlowDirectionProperty.OverrideMetadata(typeof(TextBlock), new FrameworkPropertyMetadata(FlowDirection.RightToLeft));
                    FrameworkElement.FlowDirectionProperty.OverrideMetadata(typeof(TextBox), new FrameworkPropertyMetadata(FlowDirection.RightToLeft));
                    break;
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

            this.notifyIconService?.SetVisibility(false);

            base.OnClosing(e);
        }

        private void NotifyIconClick()
        {
            this.WindowState = WindowState.Normal;

            // Bit of a hack but does work
            this.Topmost = true; 
            this.Topmost = false;
        }

        private void ShellViewStateChanged(object sender, EventArgs e)
        {
            if (this.WindowState == WindowState.Minimized)
            {
                this.ShowInTaskbar = false;
                this.notifyIconService?.SetVisibility(true);
            }
            else if (this.WindowState == WindowState.Normal)
            {
                this.notifyIconService?.SetVisibility(false);
                this.ShowInTaskbar = true;
            }
        }

        private void ShellView_OnDrop(object sender, DragEventArgs e)
        {
            ((ShellViewModel)this.DataContext).FilesDroppedOnWindow(e);
        }
    }
}
