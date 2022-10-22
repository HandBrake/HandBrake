﻿// --------------------------------------------------------------------------------------------------------------------
// <copyright file="PrePostActionService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the WhenDoneService type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services
{
    using System;
    using System.Diagnostics;
    using System.IO;
    using System.Windows.Media;

    using HandBrakeWPF.EventArgs;
    using HandBrakeWPF.Helpers;
    using HandBrakeWPF.Model.Options;
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Queue.Interfaces;
    using HandBrakeWPF.Services.Scan.Interfaces;
    using HandBrakeWPF.Utilities;
    using HandBrakeWPF.ViewModels.Interfaces;
    using HandBrakeWPF.Views;

    using EncodeCompletedEventArgs = Encode.EventArgs.EncodeCompletedEventArgs;
    using ILog = Logging.Interfaces.ILog;

    /// <summary>
    /// The when done service.
    /// </summary>
    public class PrePostActionService : IPrePostActionService
    {
        private readonly ILog log;
        private readonly INotificationService notificationService;
        private readonly IUserSettingService userSettingService;
        private readonly IWindowManager windowManager;
        private readonly IScan scanService;

        public PrePostActionService(IQueueService queueProcessor, IUserSettingService userSettingService, IWindowManager windowManager, IScan scanService, ILog logService, INotificationService notificationService)
        {
            this.log = logService;
            this.notificationService = notificationService;
            this.userSettingService = userSettingService;
            this.windowManager = windowManager;
            this.scanService = scanService;

            queueProcessor.QueueCompleted += this.QueueProcessorQueueCompleted;
            queueProcessor.QueuePaused += this.QueueProcessor_QueuePaused;
            queueProcessor.EncodeCompleted += this.EncodeService_EncodeCompleted;
            queueProcessor.JobProcessingStarted += this.EncodeService_EncodeStarted;
        }

        private void QueueProcessor_QueuePaused(object sender, EventArgs e)
        {
            // Allow the system to sleep again.
            ThreadHelper.OnUIThread(() =>
            {
                if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.PreventSleep))
                {
                    Win32.AllowSleep();
                }
            });
        }

        /// <summary>
        /// The encode service_ encode started.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void EncodeService_EncodeStarted(object sender, EventArgs e)
        {
            if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.PreventSleep))
            {
                Win32.PreventSleep();
            }
        }

        /// <summary>
        /// The encode service_ encode completed.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The EncodeCompletedEventArgs.
        /// </param>
        private void EncodeService_EncodeCompleted(object sender, EncodeCompletedEventArgs e)
        {
            // Send the file to the users requested application
            if (e.Successful)
            {
                this.SendToApplication(e.SourceFileName, e.FileName, e.ErrorCode);
            }

            if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.PlaySoundWhenDone))
            {
                this.PlayWhenDoneSound();
            }

            if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.NotifyOnEncodeDone))
            {
                string filename = Path.GetFileName(e.FileName);
                this.notificationService.SendNotification(Resources.Notifications_EncodeDone, filename);
            }
        }

        /// <summary>
        /// The queue processor queue completed event handler.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void QueueProcessorQueueCompleted(object sender, QueueCompletedEventArgs e)
        {
            if (e.WasManuallyStopped)
            {
                return;
            }

            if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.PlaySoundWhenQueueDone))
            {
                this.PlayWhenDoneSound();
            }

            if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.NotifyOnQueueDone))
            {
                this.notificationService.SendNotification(Resources.Notifications_QueueDone, null);
            }

            // Allow the system to sleep again.
            ThreadHelper.OnUIThread(() =>
            {
                if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.PreventSleep))
                {
                    Win32.AllowSleep();
                }
            });

            // ---------------------------------------------------------

            if (this.userSettingService.GetUserSetting<int>(UserSettingConstants.WhenCompleteAction) == (int)WhenDone.DoNothing)
            {
                return;
            }

            // Give the user the ability to cancel the shutdown. Default 60 second timer.
            bool isCancelled = false;
            if (!this.userSettingService.GetUserSetting<bool>(UserSettingConstants.WhenDonePerformActionImmediately))
            {
                ICountdownAlertViewModel titleSpecificView = IoCHelper.Get<ICountdownAlertViewModel>();
                ThreadHelper.OnUIThread(
                    () =>
                    {
                        titleSpecificView.SetAction((WhenDone)this.userSettingService.GetUserSetting<int>(UserSettingConstants.WhenCompleteAction));
                        this.windowManager.ShowDialog<CountdownAlertView>(titleSpecificView);
                        isCancelled = titleSpecificView.IsCancelled;
                    });
            }

            if (!isCancelled)
            {
                this.ServiceLogMessage(string.Format("Performing 'When Done' Action: {0}", this.userSettingService.GetUserSetting<int>(UserSettingConstants.WhenCompleteAction)));

                // Do something when the encode ends.
                switch ((WhenDone)this.userSettingService.GetUserSetting<int>(UserSettingConstants.WhenCompleteAction))
                {
                    case WhenDone.Shutdown:
                        ProcessStartInfo shutdown = new ProcessStartInfo("Shutdown", "-s -t 60");
                        shutdown.UseShellExecute = false;
                        Process.Start(shutdown);
                        ThreadHelper.OnUIThread(() => System.Windows.Application.Current.Shutdown());
                        break;
                    case WhenDone.LogOff:
                        this.scanService.Dispose();
                        Win32.ExitWindowsEx(0, 0);
                        break;
                    case WhenDone.Sleep:
                        Win32.SetSuspendState(false, false, false);
                        break;
                    case WhenDone.Hibernate:
                        Win32.SetSuspendState(true, false, false);
                        break;
                    case WhenDone.LockSystem:
                        Win32.LockWorkStation();
                        break;
                    case WhenDone.QuickHandBrake:
                        ThreadHelper.OnUIThread(() => System.Windows.Application.Current.Shutdown());
                        break;
                }
            }
        }

        private void SendToApplication(string source, string destination, int exitCode)
        {
            if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.SendFile) &&
                !string.IsNullOrEmpty(this.userSettingService.GetUserSetting<string>(UserSettingConstants.SendFileTo)))
            {
                string arguments = this.userSettingService.GetUserSetting<string>(UserSettingConstants.SendFileToArgs);

                arguments = arguments.Replace("{source}", string.Format("\"{0}\"", source));
                arguments = arguments.Replace("{destination}", string.Format("\"{0}\"", destination));
                arguments = arguments.Replace("{exit_code}", string.Format("{0}", exitCode));

                var process = new ProcessStartInfo(this.userSettingService.GetUserSetting<string>(UserSettingConstants.SendFileTo), arguments);
                process.EnvironmentVariables.Add("HB_SOURCE", source);
                process.EnvironmentVariables.Add("HB_DESTINATION", destination);
                process.EnvironmentVariables.Add("HB_EXIT_CODE", exitCode.ToString());

                this.ServiceLogMessage(string.Format("Sending output file to: {0}, with arguments: {1} ", process.FileName, arguments));

                try
                {
                    Process.Start(process);
                }
                catch (Exception ex)
                {
                    this.ServiceLogMessage(string.Format("Send file to failed to execute: {0}", ex));
                }
            }
        }

        private void PlayWhenDoneSound()
        {
            string filePath = this.userSettingService.GetUserSetting<string>(UserSettingConstants.WhenDoneAudioFile);
            if (!string.IsNullOrEmpty(filePath) && File.Exists(filePath))
            {
                this.ServiceLogMessage("Playing Sound: " + filePath);
                var uri = new Uri(filePath, UriKind.RelativeOrAbsolute);
                var player = new MediaPlayer();
                player.MediaFailed += (object sender, ExceptionEventArgs e) => { this.ServiceLogMessage(e?.ToString()); };
                player.Open(uri);
                player.Play();
            }
            else
            {
                this.ServiceLogMessage("Unable to play sound. Reason: File not found!");
            }
        }

        private void ServiceLogMessage(string message)
        {
            this.log.LogMessage(string.Format("# {1}{0}", Environment.NewLine, message));
        }
    }
}
