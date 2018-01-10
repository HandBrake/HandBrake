// --------------------------------------------------------------------------------------------------------------------
// <copyright file="PrePostActionService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the WhenDoneService type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Services
{
    using System;
    using System.Diagnostics;

    using Caliburn.Micro;
    using HandBrake;
    using HandBrake.Utilities.Interfaces;
    using HandBrake.EventArgs;
    using HandBrake.Services.Interfaces;
    using HandBrake.Services.Queue.Interfaces;
    using HandBrake.ViewModels.Interfaces;

    using EncodeCompletedEventArgs = HandBrake.Services.Encode.EventArgs.EncodeCompletedEventArgs;
    using Execute = Caliburn.Micro.Execute;

    /// <summary>
    /// The when done service.
    /// </summary>
    public class PrePostActionService : IPrePostActionService
    {
        /// <summary>
        /// The queue processor.
        /// </summary>
        private readonly IQueueProcessor queueProcessor;

        /// <summary>
        /// The user setting service.
        /// </summary>
        private readonly IUserSettingService userSettingService;

        /// <summary>
        /// The System State Manager.
        /// </summary>
        private readonly ISystemStateService systemStateService;

        /// <summary>
        /// The View Manager.
        /// </summary>
        private readonly IViewManager viewManager;

        /// <summary>
        /// Initializes a new instance of the <see cref="PrePostActionService"/> class.
        /// </summary>
        /// <param name="queueProcessor">
        /// The queue processor.
        /// </param>
        /// <param name="userSettingService">
        /// The user Setting Service.
        /// </param>
        /// <param name="systemStateService">
        /// The System State Service.
        /// </param>
        /// <param name="viewManager">
        /// The View Manager.
        /// </param>
        public PrePostActionService(IQueueProcessor queueProcessor, IUserSettingService userSettingService, ISystemStateService systemStateService, IViewManager viewManager)
        {
            this.queueProcessor = queueProcessor;
            this.userSettingService = userSettingService;
            this.systemStateService = systemStateService;

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
                this.systemStateService.PreventSleep();
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
            // Send the file to the users requested applicaiton
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
                    this.systemStateService.AllowSleep();
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
            if (e.WasManuallyStopped || this.systemStateService == null)
            {
                return;
            }

            if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.PlaySoundWhenQueueDone))
            {
                this.PlayWhenDoneSound();
            }

            if (this.userSettingService.GetUserSetting<string>(UserSettingConstants.WhenCompleteAction) == "Do nothing")
            {
                return;
            }

            // Give the user the ability to cancel the shutdown. Default 60 second timer.
            ICountdownAlertViewModel titleSpecificView = IoC.Get<ICountdownAlertViewModel>();
            Execute.OnUIThread(
                () =>
                    {
                        titleSpecificView.SetAction(this.userSettingService.GetUserSetting<string>(UserSettingConstants.WhenCompleteAction));
                        viewManager.ShowDialog(titleSpecificView);
                    });

            if (!titleSpecificView.IsCancelled)
            {
                // Do something whent he encode ends.
                switch (this.userSettingService.GetUserSetting<string>(UserSettingConstants.WhenCompleteAction))
                {
                    case "Shutdown":
                        if (this.systemStateService.SupportsPowerStateChange)
                        {
                            this.systemStateService.Shutdown();
                        }

                        break;

                    case "Log off":
                        if (this.systemStateService.SupportsLogOff)
                        {
                            this.systemStateService.LogOff();
                        }

                        break;

                    case "Suspend":
                        if (this.systemStateService.SupportsPowerStateChange)
                        {
                            this.systemStateService.Suspend();
                        }

                        break;

                    case "Hibernate":
                        if (this.systemStateService.SupportsHibernate)
                        {
                            this.systemStateService.Hibernate();
                        }

                        break;

                    case "Lock System":
                        if (this.systemStateService.SupportsLock)
                        {
                            this.systemStateService.Lock();
                        }

                        break;

                    case "Quit HandBrake":
                        this.systemStateService.Quit();
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
                var vlc =
                    new ProcessStartInfo(
                        this.userSettingService.GetUserSetting<string>(UserSettingConstants.SendFileTo), args);
                Process.Start(vlc);
            }
        }

        private void PlayWhenDoneSound()
        {
            try
            {
                var soundService = IoC.Get<ISoundService>();
                string filePath = this.userSettingService.GetUserSetting<string>(UserSettingConstants.WhenDoneAudioFile);
                soundService.PlayWhenDoneSound(filePath);
            }
            catch { }
        }
    }
}