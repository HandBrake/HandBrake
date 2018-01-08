// --------------------------------------------------------------------------------------------------------------------
// <copyright file="NotificationService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the DPIAwareness type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services
{
    using System;
    using System.Drawing;
    using System.IO;
    using System.Windows;
    using System.Windows.Forms;
    using System.Windows.Resources;
    using Caliburn.Micro;
    using HandBrake;
    using HandBrake.Services.Interfaces;
    using HandBrakeWPF.ViewModels.Interfaces;

    public class NotificationService : INotificationService
    {
        public static bool Registered => Instance.registered;
        private bool registered;

        private static NotificationService Instance => (NotificationService)HandBrakeServices.Current?.NotificationManager;

        private NotifyIcon notifyIcon;
        private Window window;

        public static void Register(Window window)
        {
            Instance.RegisterInstance(window);
        }

        public static void UnRegister()
        {
            if (Instance.notifyIcon != null)
            {
                Instance.notifyIcon.Visible = false;
            }

            Instance.registered = false;
        }

        public static void ChangeVisibility(bool visibile)
        {
            Instance.notifyIcon.Visible = visibile;
        }

        public void Notify(string text)
        {
            if (this.notifyIcon != null)
            {
                this.notifyIcon.Text = text;
            }
        }

        private void RegisterInstance(Window window)
        {
            this.window = window;
            this.notifyIcon = new NotifyIcon();
            this.notifyIcon.ContextMenu = new ContextMenu(new[] { new MenuItem("Restore", NotifyIconClick), new MenuItem("Mini Status Display", ShowMiniStatusDisplay) });

            StreamResourceInfo streamResourceInfo = System.Windows.Application.GetResourceStream(new Uri("pack://application:,,,/handbrakepineapple.ico"));
            if (streamResourceInfo != null)
            {
                Stream iconStream = streamResourceInfo.Stream;
                this.notifyIcon.Icon = new Icon(iconStream);
            }
            this.notifyIcon.DoubleClick += NotifyIconClick;

            registered = true;
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
        private void NotifyIconClick(object sender, System.EventArgs e)
        {
            window.WindowState = WindowState.Normal;
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
            Execute.OnUIThread(
                () =>
                {
                    titleSpecificView.Activate();
                    HandBrakeServices.Current?.ViewManager?.ShowWindow(titleSpecificView);
                });
        }
    }
}