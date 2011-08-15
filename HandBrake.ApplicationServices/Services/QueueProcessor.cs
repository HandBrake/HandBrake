/*  Queue.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr/>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Services
{
    using System;
    using System.Diagnostics;
    using System.Windows.Forms;

    using HandBrake.ApplicationServices.EventArgs;
    using HandBrake.ApplicationServices.Exceptions;
    using HandBrake.ApplicationServices.Functions;
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Services.Interfaces;

    /// <summary>
    /// The HandBrake Queue
    /// </summary>
    public class QueueProcessor : IQueueProcessor
    {
        /// <summary>
        /// The User Setting Service
        /// </summary>
        private static IUserSettingService userSettingService = ServiceManager.UserSettingService;

        /// <summary>
        /// Initializes a new instance of the <see cref="QueueProcessor"/> class.
        /// </summary>
        /// <param name="queueManager">
        /// The queue manager.
        /// </param>
        /// <param name="encodeService">
        /// The encode Service.
        /// </param>
        /// <exception cref="ArgumentNullException">
        /// Services are not setup
        /// </exception>
        public QueueProcessor(IQueueManager queueManager, IEncode encodeService)
        {
            this.QueueManager = queueManager;
            this.EncodeService = encodeService;

            if (this.QueueManager == null)
            {
                throw new ArgumentNullException("queueManager");
            }

            if (this.QueueManager == null)
            {
                throw new ArgumentNullException("queueManager");
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="QueueProcessor"/> class.
        /// This call also initializes the Encode and QueueManager services
        /// </summary>
        /// <param name="instanceId">
        /// The instance id.
        /// </param>
        public QueueProcessor(int instanceId)
        {
            this.EncodeService = new Encode();
            this.QueueManager = new QueueManager(instanceId);
        }

        #region Events

        /// <summary>
        /// Queue Progess Status
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The QueueProgressEventArgs.
        /// </param>
        public delegate void QueueProgressStatus(object sender, QueueProgressEventArgs e);

        /// <summary>
        /// Fires when the Queue has started
        /// </summary>
        public event QueueProgressStatus JobProcessingStarted;

        /// <summary>
        /// Fires when a pause to the encode queue has been requested.
        /// </summary>
        public event EventHandler QueuePaused;

        /// <summary>
        /// Fires when the entire encode queue has completed.
        /// </summary>
        public event EventHandler QueueCompleted;

        /// <summary>
        /// Invoke the JobProcessingStarted event
        /// </summary>
        /// <param name="e">
        /// The QueueProgressEventArgs.
        /// </param>
        private void InvokeJobProcessingStarted(QueueProgressEventArgs e)
        {
            QueueProgressStatus handler = this.JobProcessingStarted;
            if (handler != null)
            {
                handler(this, e);
            }
        }

        /// <summary>
        /// Invoke the QueuePaused event
        /// </summary>
        /// <param name="e">
        /// The EventArgs.
        /// </param>
        private void InvokeQueuePaused(EventArgs e)
        {
            EventHandler handler = this.QueuePaused;
            if (handler != null)
            {
                handler(this, e);
            }
        }

        /// <summary>
        /// Invoke the QueueCompleted event.
        /// </summary>
        /// <param name="e">
        /// The EventArgs.
        /// </param>
        private void InvokeQueueCompleted(EventArgs e)
        {
            this.IsProcessing = false;

            EventHandler handler = this.QueueCompleted;
            if (handler != null)
            {
                handler(this, e);
            }
        }

        #endregion

        #region Properties

        /// <summary>
        /// Gets a value indicating whether IsProcessing.
        /// </summary>
        public bool IsProcessing { get; private set; }

        /// <summary>
        /// Gets the IEncodeService instance.
        /// </summary>
        public IEncode EncodeService { get; private set; }

        /// <summary>
        /// Gets the IQueueManager instance.
        /// </summary>
        public IQueueManager QueueManager { get; private set; }

        #endregion

        /// <summary>
        /// Starts encoding the first job in the queue and continues encoding until all jobs
        /// have been encoded.
        /// </summary>
        public void Start()
        {
            if (IsProcessing)
            {
                throw new Exception("Already Processing the Queue");
            }

            IsProcessing = true;
            this.EncodeService.EncodeCompleted += this.EncodeServiceEncodeCompleted;
            this.ProcessNextJob();
        }

        /// <summary>
        /// Requests a pause of the encode queue.
        /// </summary>
        public void Pause()
        {
            this.InvokeQueuePaused(EventArgs.Empty);
            this.IsProcessing = false;
        }

        /// <summary>
        /// After an encode is complete, move onto the next job.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The EncodeCompletedEventArgs.
        /// </param>
        private void EncodeServiceEncodeCompleted(object sender, EncodeCompletedEventArgs e)
        {
            this.QueueManager.LastProcessedJob.Status = QueueItemStatus.Completed;

            // Growl
            if (userSettingService.GetUserSetting<bool>(UserSettingConstants.GrowlEncode))
                GrowlCommunicator.Notify("Encode Completed",
                                         "Put down that cocktail...\nyour Handbrake encode is done.");

            if (!e.Successful)
            {
                this.QueueManager.LastProcessedJob.Status = QueueItemStatus.Error;
                this.Pause();
                throw new GeneralApplicationException(e.ErrorInformation, e.Exception.Message, e.Exception);
            }

            // Handling Log Data 
            this.EncodeService.ProcessLogs(this.QueueManager.LastProcessedJob.Destination);

            // Post-Processing
            if (e.Successful)
            {
                SendToApplication(this.QueueManager.LastProcessedJob.Destination);
            }

            // Move onto the next job.
            if (this.IsProcessing)
            {
                this.ProcessNextJob();
            } 
            else 
            {
                this.EncodeService.EncodeCompleted -= this.EncodeServiceEncodeCompleted;
                this.InvokeQueueCompleted(EventArgs.Empty);
                this.QueueManager.BackupQueue(string.Empty);
            }
        }

        /// <summary>
        /// Run through all the jobs on the queue.
        /// </summary>
        private void ProcessNextJob()
        {
            if (this.EncodeService.IsEncoding || !this.IsProcessing)
            {
                // We don't want to try start a second encode, so just return out. The event will trigger the next encode automatically.
                // Also, we don't want to start a new encode if we are paused.
                return;
            }

            QueueTask job = this.QueueManager.GetNextJobForProcessing();
            if (job != null)
            {
                this.EncodeService.Start(job, true);
                this.InvokeJobProcessingStarted(new QueueProgressEventArgs(job));
            }
            else
            {
                // No more jobs to process, so unsubscribe the event
                this.EncodeService.EncodeCompleted -= this.EncodeServiceEncodeCompleted;

                // Fire the event to tell connected services.
                this.InvokeQueueCompleted(EventArgs.Empty);

                // Run the After encode completeion work
                Finish();
            }
        }

        /// <summary>
        /// Send a file to a 3rd party application after encoding has completed.
        /// </summary>
        /// <param name="file"> The file path</param>
        private static void SendToApplication(string file)
        {
            if (userSettingService.GetUserSetting<bool>(UserSettingConstants.SendFile) && !string.IsNullOrEmpty(userSettingService.GetUserSetting<string>(UserSettingConstants.SendFileTo)))
            {
                string args = string.Format("{0} \"{1}\"", userSettingService.GetUserSetting<string>(UserSettingConstants.SendFileToArgs), file);
                ProcessStartInfo vlc = new ProcessStartInfo(userSettingService.GetUserSetting<string>(UserSettingConstants.SendFileTo), args);
                Process.Start(vlc);
            }
        }

        /// <summary>
        /// Perform an action after an encode. e.g a shutdown, standby, restart etc.
        /// </summary>
        private static void Finish()
        {
            // Growl
            if (userSettingService.GetUserSetting<bool>(UserSettingConstants.GrowlQueue))
            {
                GrowlCommunicator.Notify("Queue Completed", "Put down that cocktail...\nyour Handbrake queue is done.");
            }

            // Do something whent he encode ends.
            switch (userSettingService.GetUserSetting<string>(UserSettingConstants.WhenCompleteAction))
            {
                case "Shutdown":
                    Process.Start("Shutdown", "-s -t 60");
                    break;
                case "Log off":
                    Win32.ExitWindowsEx(0, 0);
                    break;
                case "Suspend":
                    Application.SetSuspendState(PowerState.Suspend, true, true);
                    break;
                case "Hibernate":
                    Application.SetSuspendState(PowerState.Hibernate, true, true);
                    break;
                case "Lock System":
                    Win32.LockWorkStation();
                    break;
                case "Quit HandBrake":
                    Application.Exit();
                    break;
                default:
                    break;
            }
        }
    }
}