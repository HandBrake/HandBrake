// --------------------------------------------------------------------------------------------------------------------
// <copyright file="NotificationService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services
{
    using System;
    using System.Diagnostics;
    using System.Linq;
    using System.Windows;

    using Windows.UI.Notifications;

    using HandBrakeWPF.Services.Interfaces;

    using Microsoft.Toolkit.Uwp.Notifications;

    /// <summary>
    /// Docs: https://docs.microsoft.com/en-us/windows/apps/design/shell/tiles-and-notifications/send-local-toast?tabs=desktop
    /// </summary>
    public class NotificationService : INotificationService
    {
        private readonly IUserSettingService userSettingService;
        private ToastNotifierCompat notifier;
        private bool isInitialised;
        private int id;

        public NotificationService(IUserSettingService userSettingService)
        {
            userSettingService.SettingChanged += this.UserSettingService_SettingChanged;
            this.userSettingService = userSettingService;
            this.Setup();
        }

        private void UserSettingService_SettingChanged(object sender, HandBrakeWPF.EventArgs.SettingChangedEventArgs e)
        {
            if (e.Key == UserSettingConstants.NotifyOnEncodeDone || e.Key == UserSettingConstants.NotifyOnQueueDone)
            {
                this.Setup();
            }
        }

        public bool SendNotification(string header, string content)
        {
            try
            {
                this.id += 1;
                ToastContentBuilder toast = new ToastContentBuilder().AddArgument("tag", this.id.ToString())
                    .AddText(header);

                if (!string.IsNullOrEmpty(content))
                {
                    toast.AddText(content);
                }

                ToastNotification notification = new ToastNotification(toast.GetXml());
                notification.ExpirationTime = DateTime.Now.AddDays(1);
                notification.Group = "HandBrake";
                notification.Tag = this.id.ToString();

                if (this.notifier != null)
                {
                    notifier.Show(notification);
                }

                return true;
            }
            catch (Exception exc)
            {
                Debug.WriteLine(exc);
                return false;
            }
        }

        public void Shutdown()
        {
            try
            {
                // Clear the history when HandBrake Closes. This information is not likely to be useful anymore.
                ToastNotificationManagerCompat.History.Clear();
            }
            catch (Exception exc)
            {
                Debug.WriteLine(exc);
            }
        }

        public void Uninstall()
        {
            try
            {
                // Useful utility to allow users to remove HandBrake from Windows Settings.
                this.notifier = null;
                ToastNotificationManagerCompat.Uninstall();
                isInitialised = false;
            }
            catch (Exception exc)
            {
                Debug.WriteLine(exc);
            }
        }

        private void Setup()
        {
            bool encode = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.NotifyOnEncodeDone);
            bool queue = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.NotifyOnQueueDone);

            if (encode || queue)
            {
                if (isInitialised)
                {
                    return;
                }

                try
                {
                    this.notifier = ToastNotificationManagerCompat.CreateToastNotifier();

                    isInitialised = true;

                    ToastNotificationManagerCompat.OnActivated += toastArgs =>
                    {
                        // Obtain the arguments from the notification
                        ToastArguments args = ToastArguments.Parse(toastArgs.Argument);

                        // Remove any notifications that are clicked
                        System.Collections.Generic.KeyValuePair<string, string> tag = args.FirstOrDefault();
                        if (!string.IsNullOrEmpty(tag.Value))
                        {
                            try
                            {
                                ToastNotificationManagerCompat.History.Remove(tag.Value);
                            }
                            catch (Exception exc)
                            {
                                Debug.WriteLine(exc);
                            }
                        }

                        // Need to dispatch to UI thread if performing UI operations
                        Application.Current.Dispatcher.Invoke(delegate
                        {
                            Window w = Application.Current.MainWindow;
                            if (w != null)
                            {
                                w.WindowState = WindowState.Normal;
                                w.BringIntoView();
                            }
                        });
                    };
                }
                catch (Exception exc)
                {
                    try
                    {
                        this.userSettingService.SetUserSetting(UserSettingConstants.NotifyOnEncodeDone, false);
                        this.userSettingService.SetUserSetting(UserSettingConstants.NotifyOnQueueDone, false);

                        this.Uninstall();
                    }
                    catch (Exception exc2)
                    {
                        Debug.WriteLine(exc2);
                    }

                    Debug.WriteLine(exc);
                }
            }
        }
    }
}
