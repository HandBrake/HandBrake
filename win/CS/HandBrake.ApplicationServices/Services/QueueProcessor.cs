// --------------------------------------------------------------------------------------------------------------------
// <copyright file="QueueProcessor.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The HandBrake Queue
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Services
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.IO;
    using System.Linq;
    using System.Windows.Forms;
    using System.Xml.Serialization;

    using Caliburn.Micro;

    using HandBrake.ApplicationServices.EventArgs;
    using HandBrake.ApplicationServices.Exceptions;
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Services.Interfaces;
    using HandBrake.ApplicationServices.Utilities;

    /// <summary>
    /// The HandBrake Queue
    /// </summary>
    public class QueueProcessor : IQueueProcessor
    {
        #region Constants and Fields

        /// <summary>
        /// A Lock object to maintain thread safety
        /// </summary>
        private static readonly object QueueLock = new object();

        /// <summary>
        /// The Queue of Job objects
        /// </summary>
        private readonly BindingList<QueueTask> queue = new BindingList<QueueTask>();

        /// <summary>
        /// The User Setting Service
        /// </summary>
        private readonly IUserSettingService userSettingService;

        /// <summary>
        /// HandBrakes Queue file with a place holder for an extra string.
        /// </summary>
        private string queueFile;

        #endregion

        #region Constructors and Destructors

        /// <summary>
        /// Initializes a new instance of the <see cref="QueueProcessor"/> class.
        /// </summary>
        /// <param name="encodeService">
        /// The encode Service.
        /// </param>
        /// <param name="userSettingService">
        /// The user Setting Service.
        /// </param>
        /// <exception cref="ArgumentNullException">
        /// Services are not setup
        /// </exception>
        public QueueProcessor(IEncodeServiceWrapper encodeService, IUserSettingService userSettingService)
        {
            this.userSettingService = userSettingService;
            this.EncodeService = encodeService;

            // If this is the first instance, just use the main queue file, otherwise add the instance id to the filename.
            this.queueFile = string.Format("hb_queue_recovery{0}.xml", GeneralUtilities.ProcessId);
        }

        #endregion

        #region Delegates

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

        #endregion

        #region Events

        /// <summary>
        /// Fires when the Queue has started
        /// </summary>
        public event QueueProgressStatus JobProcessingStarted;

        /// <summary>
        /// Fires when a job is Added, Removed or Re-Ordered.
        /// Should be used for triggering an update of the Queue Window.
        /// </summary>
        public event EventHandler QueueChanged;

        /// <summary>
        /// Fires when the entire encode queue has completed.
        /// </summary>
        public event EventHandler QueueCompleted;

        /// <summary>
        /// Fires when a pause to the encode queue has been requested.
        /// </summary>
        public event EventHandler QueuePaused;

        #endregion

        #region Properties

        /// <summary>
        /// Gets the number of jobs in the queue;
        /// </summary>
        public int Count
        {
            get
            {
                return this.queue.Count(item => item.Status == QueueItemStatus.Waiting);
            }
        }

        /// <summary>
        /// Gets the IEncodeService instance.
        /// </summary>
        public IEncodeServiceWrapper EncodeService { get; private set; }

        /// <summary>
        /// Gets a value indicating whether IsProcessing.
        /// </summary>
        public bool IsProcessing { get; private set; }

        /// <summary>
        /// Gets or sets Last Processed Job.
        /// This is set when the job is poped of the queue by GetNextJobForProcessing();
        /// </summary>
        public QueueTask LastProcessedJob { get; set; }

        /// <summary>
        /// Gets The current queue.
        /// </summary>
        public BindingList<QueueTask> Queue
        {
            get
            {
                return this.queue;
            }
        }

        #endregion

        #region Public Methods

        /// <summary>
        /// Add a job to the Queue. 
        /// This method is Thread Safe.
        /// </summary>
        /// <param name="job">
        /// The encode Job object.
        /// </param>
        public void Add(QueueTask job)
        {
            lock (QueueLock)
            {
                this.queue.Add(job);
                this.InvokeQueueChanged(EventArgs.Empty);
            }
        }

        /// <summary>
        /// Backup any changes to the queue file
        /// </summary>
        /// <param name="exportPath">
        /// If this is not null or empty, this will be used instead of the standard backup location.
        /// </param>
        public void BackupQueue(string exportPath)
        {
            string appDataPath = Path.Combine(
                Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), @"HandBrake\");
            string tempPath = !string.IsNullOrEmpty(exportPath)
                                  ? exportPath
                                  : appDataPath + string.Format(this.queueFile, string.Empty);

            using (var strm = new FileStream(tempPath, FileMode.Create, FileAccess.Write))
            {
                List<QueueTask> tasks = this.queue.Where(item => item.Status != QueueItemStatus.Completed).ToList();
                var serializer = new XmlSerializer(typeof(List<QueueTask>));
                serializer.Serialize(strm, tasks);
                strm.Close();
                strm.Dispose();
            }
        }

        /// <summary>
        /// Checks the current queue for an existing instance of the specified destination.
        /// </summary>
        /// <param name="destination">
        /// The destination of the encode.
        /// </param>
        /// <returns>
        /// Whether or not the supplied destination is already in the queue.
        /// </returns>
        public bool CheckForDestinationPathDuplicates(string destination)
        {
            return this.queue.Any(job => job.Task != null && job.Task.Destination.Contains(destination.Replace("\\\\", "\\")));
        }

        /// <summary>
        /// Clear down all Queue Items
        /// </summary>
        public void Clear()
        {
            List<QueueTask> deleteList = this.queue.ToList();
            foreach (QueueTask item in deleteList)
            {
                this.queue.Remove(item);
            }
            this.InvokeQueueChanged(EventArgs.Empty);
        }

        /// <summary>
        /// Clear down the Queue´s completed items
        /// </summary>
        public void ClearCompleted()
        {
            Execute.OnUIThread(
                () =>
                    {
                        List<QueueTask> deleteList =
                            this.queue.Where(task => task.Status == QueueItemStatus.Completed).ToList();
                        foreach (QueueTask item in deleteList)
                        {
                            this.queue.Remove(item);
                        }
                        this.InvokeQueueChanged(EventArgs.Empty);
                    });
        }

        /// <summary>
        /// Get the first job on the queue for processing.
        /// This also removes the job from the Queue and sets the LastProcessedJob
        /// </summary>
        /// <returns>
        /// An encode Job object.
        /// </returns>
        public QueueTask GetNextJobForProcessing()
        {
            if (this.queue.Count > 0)
            {
                QueueTask job = this.queue.FirstOrDefault(q => q.Status == QueueItemStatus.Waiting);
                if (job != null)
                {
                    job.Status = QueueItemStatus.InProgress;
                    this.LastProcessedJob = job;
                    this.InvokeQueueChanged(EventArgs.Empty);
                }

                this.BackupQueue(string.Empty);
                return job;
            }

            this.BackupQueue(string.Empty);
            return null;
        }

        /// <summary>
        /// Moves an item down one position in the queue.
        /// </summary>
        /// <param name="index">
        /// The zero-based location of the job in the queue.
        /// </param>
        public void MoveDown(int index)
        {
            if (index < this.queue.Count - 1)
            {
                QueueTask item = this.queue[index];

                this.queue.RemoveAt(index);
                this.queue.Insert((index + 1), item);
            }

            this.InvokeQueueChanged(EventArgs.Empty);
        }

        /// <summary>
        /// Moves an item up one position in the queue.
        /// </summary>
        /// <param name="index">
        /// The zero-based location of the job in the queue.
        /// </param>
        public void MoveUp(int index)
        {
            if (index > 0)
            {
                QueueTask item = this.queue[index];

                this.queue.RemoveAt(index);
                this.queue.Insert((index - 1), item);
            }

            this.InvokeQueueChanged(EventArgs.Empty);
        }

        /// <summary>
        /// Remove a job from the Queue.
        /// This method is Thread Safe
        /// </summary>
        /// <param name="job">
        /// The job.
        /// </param>
        public void Remove(QueueTask job)
        {
            lock (QueueLock)
            {
                this.queue.Remove(job);
                this.InvokeQueueChanged(EventArgs.Empty);
            }
        }

        /// <summary>
        /// Reset a Queued Item from Error or Completed to Waiting
        /// </summary>
        /// <param name="job">
        /// The job.
        /// </param>
        public void ResetJobStatusToWaiting(QueueTask job)
        {
            if (job.Status != QueueItemStatus.Error && job.Status != QueueItemStatus.Completed)
            {
                throw new GeneralApplicationException(
                    "Job Error", "Unable to reset job status as it is not in an Error or Completed state", null);
            }

            job.Status = QueueItemStatus.Waiting;
        }

        /// <summary>
        /// Restore a Queue from file or from the queue backup file.
        /// </summary>
        /// <param name="importPath">
        /// The import path. String.Empty or null will result in the default file being loaded.
        /// </param>
        public void RestoreQueue(string importPath)
        {
            string appDataPath = Path.Combine(
                Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), @"HandBrake\");
            string tempPath = !string.IsNullOrEmpty(importPath)
                                  ? importPath
                                  : (appDataPath + string.Format(this.queueFile, string.Empty));

            if (File.Exists(tempPath))
            {
                bool invokeUpdate = false;
                using (
                    var strm = new FileStream(
                        (!string.IsNullOrEmpty(importPath) ? importPath : tempPath), FileMode.Open, FileAccess.Read))
                {
                    if (strm.Length != 0)
                    {
                        var serializer = new XmlSerializer(typeof(List<QueueTask>));

                        List<QueueTask> list;

                        try
                        {
                            list = serializer.Deserialize(strm) as List<QueueTask>;
                        }
                        catch (Exception exc)
                        {
                            throw new GeneralApplicationException(
                                "Unable to restore queue file.", 
                                "The file may be corrupted or from an older incompatible version of HandBrake", 
                                exc);
                        }

                        if (list != null)
                        {
                            foreach (QueueTask item in list)
                            {
                                if (item.Status != QueueItemStatus.Completed)
                                {
                                    // Reset InProgress/Error to Waiting so it can be processed
                                    if (item.Status == QueueItemStatus.InProgress)
                                    {
                                        item.Status = QueueItemStatus.Waiting;
                                    }

                                    this.queue.Add(item);
                                }
                            }
                        }

                        invokeUpdate = true;
                    }
                }

                if (invokeUpdate)
                {
                    this.InvokeQueueChanged(EventArgs.Empty);
                }
            }
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
        /// Starts encoding the first job in the queue and continues encoding until all jobs
        /// have been encoded.
        /// </summary>
        public void Start()
        {
            if (this.IsProcessing)
            {
                throw new Exception("Already Processing the Queue");
            }

            this.IsProcessing = true;
            this.EncodeService.EncodeCompleted += this.EncodeServiceEncodeCompleted;
            this.ProcessNextJob();
        }

        #endregion

        #region Methods

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
            this.LastProcessedJob.Status = QueueItemStatus.Completed;

            // Clear the completed item of the queue if the setting is set.
            if (this.userSettingService.GetUserSetting<bool>(ASUserSettingConstants.ClearCompletedFromQueue))
            {
                this.ClearCompleted();
            }

            if (!e.Successful)
            {
                this.LastProcessedJob.Status = QueueItemStatus.Error;
                this.Pause();
            }

            // Handling Log Data 
            this.EncodeService.ProcessLogs(this.LastProcessedJob.Task.Destination);

            // Post-Processing
            if (e.Successful)
            {
                this.SendToApplication(this.LastProcessedJob.Task.Destination);
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
                this.BackupQueue(string.Empty);
            }
        }

        /// <summary>
        /// Perform an action after an encode. e.g a shutdown, standby, restart etc.
        /// </summary>
        private void Finish()
        {
            // Do something whent he encode ends.
            switch (this.userSettingService.GetUserSetting<string>(ASUserSettingConstants.WhenCompleteAction))
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
                    Execute.OnUIThread(Application.Exit);
                    break;
            }
        }

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
        /// Invoke the Queue Changed Event
        /// </summary>
        /// <param name="e">
        /// The e.
        /// </param>
        private void InvokeQueueChanged(EventArgs e)
        {
            try
            {
                this.BackupQueue(string.Empty);
            }
            catch (Exception)
            {
                // Do Nothing.
            }

            EventHandler handler = this.QueueChanged;
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
        /// Run through all the jobs on the queue.
        /// </summary>
        private void ProcessNextJob()
        {
            QueueTask job = this.GetNextJobForProcessing();
            if (job != null)
            {
                this.InvokeJobProcessingStarted(new QueueProgressEventArgs(job));
                this.EncodeService.Start(job, true);
            }
            else
            {
                // No more jobs to process, so unsubscribe the event
                this.EncodeService.EncodeCompleted -= this.EncodeServiceEncodeCompleted;

                // Fire the event to tell connected services.
                this.InvokeQueueCompleted(EventArgs.Empty);

                // Run the After encode completeion work
                this.Finish();
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
            if (this.userSettingService.GetUserSetting<bool>(ASUserSettingConstants.SendFile) &&
                !string.IsNullOrEmpty(this.userSettingService.GetUserSetting<string>(ASUserSettingConstants.SendFileTo)))
            {
                string args = string.Format(
                    "{0} \"{1}\"", 
                    this.userSettingService.GetUserSetting<string>(ASUserSettingConstants.SendFileToArgs), 
                    file);
                var vlc =
                    new ProcessStartInfo(
                        this.userSettingService.GetUserSetting<string>(ASUserSettingConstants.SendFileTo), args);
                Process.Start(vlc);
            }
        }

        #endregion
    }
}