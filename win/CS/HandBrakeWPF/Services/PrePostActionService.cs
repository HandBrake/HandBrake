// --------------------------------------------------------------------------------------------------------------------
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
    using System.Windows.Forms;
    using System.Windows.Media;

    using Caliburn.Micro;

    using HandBrake.Interop.Utilities;

    using HandBrakeWPF.EventArgs;
    using HandBrakeWPF.Instance;
    using HandBrakeWPF.Model.Options;
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Logging;
    using HandBrakeWPF.Services.Logging.Model;
    using HandBrakeWPF.Services.Queue.Interfaces;
    using HandBrakeWPF.Services.Scan.Interfaces;
    using HandBrakeWPF.Utilities;
    using HandBrakeWPF.ViewModels.Interfaces;

    using EncodeCompletedEventArgs = HandBrakeWPF.Services.Encode.EventArgs.EncodeCompletedEventArgs;
    using ILog = HandBrakeWPF.Services.Logging.Interfaces.ILog;

    /// <summary>
    /// The when done service.
    /// </summary>
    public class PrePostActionService : IPrePostActionService
    {
        private readonly ILog log = LogService.GetLogger();
        private readonly IQueueService queueProcessor;
        private readonly IUserSettingService userSettingService;
        private readonly IWindowManager windowManager;
        private readonly IScan scanService;

        /// <summary>
        /// Initializes a new instance of the <see cref="PrePostActionService"/> class.
        /// </summary>
        /// <param name="queueProcessor">
        /// The queue processor.
        /// </param>
        /// <param name="userSettingService">
        /// The user Setting Service.
        /// </param>
        /// <param name="windowManager">
        /// The window Manager.
        /// </param>
        public PrePostActionService(IQueueService queueProcessor, IUserSettingService userSettingService, IWindowManager windowManager, IScan scanService)
        {
            this.queueProcessor = queueProcessor;
            this.userSettingService = userSettingService;
            this.windowManager = windowManager;
            this.scanService = scanService;

            this.queueProcessor.QueueCompleted += QueueProcessorQueueCompleted;
            this.queueProcessor.EncodeService.EncodeCompleted += EncodeService_EncodeCompleted;
            this.queueProcessor.EncodeService.EncodeStarted += EncodeService_EncodeStarted;
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
                this.SendToApplication(e.FileName);
            }

            if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.PlaySoundWhenDone))
            {
                this.PlayWhenDoneSound();
            }

            // Allow the system to sleep again.
            Execute.OnUIThread(() =>
            {
                if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.PreventSleep))
                {
                    Win32.AllowSleep();
                }
            });
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

            if (this.userSettingService.GetUserSetting<int>(UserSettingConstants.WhenCompleteAction) == (int)WhenDone.DoNothing)
            {
                return;
            }

            // Give the user the ability to cancel the shutdown. Default 60 second timer.
            bool isCancelled = false;
            if (!this.userSettingService.GetUserSetting<bool>(UserSettingConstants.WhenDonePerformActionImmediately))
            {
                ICountdownAlertViewModel titleSpecificView = IoC.Get<ICountdownAlertViewModel>();
                Execute.OnUIThread(
                    () =>
                    {
                        titleSpecificView.SetAction((WhenDone)this.userSettingService.GetUserSetting<int>(UserSettingConstants.WhenCompleteAction));
                        this.windowManager.ShowDialog(titleSpecificView);
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
                        Execute.OnUIThread(() => System.Windows.Application.Current.Shutdown());
                        break;
                    case WhenDone.LogOff:
                        this.scanService.Dispose();
                        Win32.ExitWindowsEx(0, 0);
                        break;
                    case WhenDone.Sleep:
                        Application.SetSuspendState(PowerState.Suspend, true, true);
                        break;
                    case WhenDone.Hibernate:
                        Application.SetSuspendState(PowerState.Hibernate, true, true);
                        break;
                    case WhenDone.LockSystem:
                        Win32.LockWorkStation();
                        break;
                    case WhenDone.QuickHandBrake:
                        Execute.OnUIThread(() => System.Windows.Application.Current.Shutdown());
                        break;
                }
            }
        }

        /// <summary>
        /// Send a file to a 3rd party application after encoding has completed.
        /// </summary>
        /// <param name="file">
        /// The file path
        /// </param>
        private void SendToApplication(string file)
        {
            if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.SendFile) &&
                !string.IsNullOrEmpty(this.userSettingService.GetUserSetting<string>(UserSettingConstants.SendFileTo)))
            {
                string args = string.Format(
                    "{0} \"{1}\"", 
                    this.userSettingService.GetUserSetting<string>(UserSettingConstants.SendFileToArgs), 
                    file);
                var destination =
                    new ProcessStartInfo(
                        this.userSettingService.GetUserSetting<string>(UserSettingConstants.SendFileTo), args);

                this.ServiceLogMessage(string.Format("Sending output file to: {0}, with arguments: {1} ", destination, args));

                Process.Start(destination);
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
            this.log.LogMessage(string.Format("# {1}{0}", Environment.NewLine, message), LogMessageType.ScanOrEncode, LogLevel.Info);
        }
    }
}
