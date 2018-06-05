// --------------------------------------------------------------------------------------------------------------------
// <copyright file="QueueProcessor.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The HandBrake Queue
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Queue
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.IO;
    using System.Linq;
    using System.Xml.Serialization;

    using HandBrake.Interop.Model;
    using HandBrake.Interop.Utilities;

    using HandBrakeWPF.Factories;
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Encode.Factories;
    using HandBrakeWPF.Services.Encode.Model;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Queue.Model;
    using HandBrakeWPF.Utilities;

    using EncodeCompletedEventArgs = HandBrakeWPF.Services.Encode.EventArgs.EncodeCompletedEventArgs;
    using Execute = Caliburn.Micro.Execute;
    using GeneralApplicationException = HandBrakeWPF.Exceptions.GeneralApplicationException;
    using IEncode = HandBrakeWPF.Services.Encode.Interfaces.IEncode;
    using LogLevel = HandBrakeWPF.Services.Logging.Model.LogLevel;
    using LogMessageType = HandBrakeWPF.Services.Logging.Model.LogMessageType;
    using LogService = HandBrakeWPF.Services.Logging.LogService;
    using QueueCompletedEventArgs = HandBrakeWPF.EventArgs.QueueCompletedEventArgs;
    using QueueProgressEventArgs = HandBrakeWPF.EventArgs.QueueProgressEventArgs;

    /// <summary>
    /// The HandBrake Queue
    /// </summary>
    public class QueueProcessor : Interfaces.IQueueProcessor
    {
        #region Constants and Fields

        /// <summary>
        /// A Lock object to maintain thread safety
        /// </summary>
        private static readonly object QueueLock = new object();
        private readonly IUserSettingService userSettingService;
        private readonly BindingList<QueueTask> queue = new BindingList<QueueTask>();
        private readonly string queueFile;
        private bool clearCompleted;

        #endregion

        #region Constructors and Destructors

        /// <summary>
        /// Initializes a new instance of the <see cref="QueueProcessor"/> class.
        /// </summary>
        /// <param name="encodeService">
        /// The encode Service.
        /// </param>
        /// <param name="userSettingService">
        /// The user settings service.
        /// </param>
        /// <param name="errorService">
        /// The Error Service.
        /// </param>
        /// <exception cref="ArgumentNullException">
        /// Services are not setup
        /// </exception>
        public QueueProcessor(IEncode encodeService, IUserSettingService userSettingService)
        {
            this.userSettingService = userSettingService;
            this.EncodeService = encodeService;

            // If this is the first instance, just use the main queue file, otherwise add the instance id to the filename.
            this.queueFile = string.Format("hb_queue_recovery{0}.xml", GeneralUtilities.ProcessId);
        }

        #endregion

        #region Delegates

        /// <summary>
        /// Queue Progress Status
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The QueueProgressEventArgs.
        /// </param>
        public delegate void QueueProgressStatus(object sender, QueueProgressEventArgs e);

        /// <summary>
        /// The queue completed.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        public delegate void QueueCompletedEventDelegate(object sender, QueueCompletedEventArgs e);

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
        public event QueueCompletedEventDelegate QueueCompleted;

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
        /// The number of errors detected.
        /// </summary>
        public int ErrorCount
        {
            get
            {
                return this.queue.Count(item => item.Status == QueueItemStatus.Error);
            }
        }

        /// <summary>
        /// Gets the IEncodeService instance.
        /// </summary>
        public IEncode EncodeService { get; private set; }

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
            string appDataPath = DirectoryUtilities.GetUserStoragePath(VersionHelper.IsNightly());
            string tempPath = !string.IsNullOrEmpty(exportPath)
                                  ? exportPath
                                  : Path.Combine(appDataPath, string.Format(this.queueFile, string.Empty));

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
        /// Export the Queue the standardised JSON format.
        /// </summary>
        /// <param name="exportPath">
        /// The export Path.
        /// </param>
        public void ExportJson(string exportPath)
        {
            List<QueueTask> jobs = this.queue.Where(item => item.Status != QueueItemStatus.Completed).ToList();
            List<EncodeTask> workUnits = jobs.Select(job => job.Task).ToList();
            HBConfiguration config = HBConfigurationFactory.Create(); // Default to current settings for now. These will hopefully go away in the future.

            string json = QueueFactory.GetQueueJson(workUnits, config);

            using (var strm = new StreamWriter(exportPath, false))
            {
                strm.Write(json);
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
            foreach (QueueTask job in this.queue)
            {
                if (string.Equals(
                    job.Task.Destination,
                    destination.Replace("\\\\", "\\"),
                    StringComparison.OrdinalIgnoreCase) && (job.Status == QueueItemStatus.Waiting || job.Status == QueueItemStatus.InProgress))
                {
                    return true;
                }
            }

            return false;
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
                return this.queue.FirstOrDefault(q => q.Status == QueueItemStatus.Waiting);
            }

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
                    Resources.Error, Resources.Queue_UnableToResetJob, null);
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
            string appDataPath = DirectoryUtilities.GetUserStoragePath(VersionHelper.IsNightly());
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
                            throw new GeneralApplicationException(Resources.Queue_UnableToRestoreFile, Resources.Queue_UnableToRestoreFileExtended, exc);
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
                                        item.Status = QueueItemStatus.Error;
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
            this.IsProcessing = false;
            this.InvokeQueuePaused(EventArgs.Empty);
        }

        public void PauseEncode()
        {
            if (this.EncodeService.IsEncoding && !this.EncodeService.IsPasued)
            {
                this.EncodeService.Pause();
                this.LastProcessedJob.Statistics.SetPaused(true);
            }

            this.Pause();
        }

        /// <summary>
        /// Starts encoding the first job in the queue and continues encoding until all jobs
        /// have been encoded.
        /// </summary>
        /// <param name="isClearCompleted">
        /// The is Clear Completed.
        /// </param>
        public void Start(bool isClearCompleted)
        {
            if (this.IsProcessing)
            {
                return;
            }

            this.clearCompleted = isClearCompleted;

            this.EncodeService.EncodeCompleted -= this.EncodeServiceEncodeCompleted;
            this.EncodeService.EncodeCompleted += this.EncodeServiceEncodeCompleted;

            if (this.EncodeService.IsPasued)
            {
                this.EncodeService.Resume();
                this.IsProcessing = true;
                this.InvokeJobProcessingStarted(new QueueProgressEventArgs(this.LastProcessedJob));
                this.LastProcessedJob.Statistics.SetPaused(false);
            }

            if (!this.EncodeService.IsEncoding)
            {
                this.ProcessNextJob();
            }
        }

        public void Stop()
        {
            if (this.EncodeService.IsEncoding)
            {
                this.EncodeService.Stop();
            }
            this.IsProcessing = false;
            this.InvokeQueuePaused(EventArgs.Empty);
        }

        #endregion

        #region Methods

        /// <summary>
        /// The on queue completed.
        /// </summary>
        /// <param name="e">
        /// The e.
        /// </param>
        protected virtual void OnQueueCompleted(QueueCompletedEventArgs e)
        {
            QueueCompletedEventDelegate handler = this.QueueCompleted;
            if (handler != null)
            {
                handler(this, e);
            }

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
            this.LastProcessedJob.Status = QueueItemStatus.Completed;
            this.LastProcessedJob.Statistics.EndTime = DateTime.Now;
            this.LastProcessedJob.Statistics.CompletedActivityLogPath = e.ActivityLogPath;
            this.LastProcessedJob.Statistics.FinalFileSize = e.FinalFilesizeInBytes;

            // Clear the completed item of the queue if the setting is set.
            if (this.clearCompleted)
            {
                this.ClearCompleted();
            }

            if (!e.Successful)
            {
                this.LastProcessedJob.Status = QueueItemStatus.Error;
            }

            // Move onto the next job.
            if (this.IsProcessing)
            {
                this.ProcessNextJob();
            }
            else
            {
                this.EncodeService.EncodeCompleted -= this.EncodeServiceEncodeCompleted;
                this.BackupQueue(string.Empty);
                this.OnQueueCompleted(new QueueCompletedEventArgs(true));
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
        /// Invoke the QueuePaused event
        /// </summary>
        /// <param name="e">
        /// The EventArgs.
        /// </param>
        private void InvokeQueuePaused(EventArgs e)
        {
            this.IsProcessing = false;

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
                if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.PauseOnLowDiskspace) && !DriveUtilities.HasMinimumDiskSpace(job.Task.Destination, this.userSettingService.GetUserSetting<long>(UserSettingConstants.PauseOnLowDiskspaceLevel)))
                {
                    LogService.GetLogger().LogMessage(Resources.PauseOnLowDiskspace, LogMessageType.ScanOrEncode, LogLevel.Info);
                    job.Status = QueueItemStatus.Waiting;
                    this.Pause();
                    this.BackupQueue(string.Empty);
                    return; // Don't start the next job.
                }

                job.Status = QueueItemStatus.InProgress;
                job.Statistics.StartTime = DateTime.Now;
                this.LastProcessedJob = job;
                this.IsProcessing = true;
                this.InvokeQueueChanged(EventArgs.Empty);
                this.InvokeJobProcessingStarted(new QueueProgressEventArgs(job));
                this.EncodeService.Start(job.Task, job.Configuration);
                this.BackupQueue(string.Empty);
            }
            else
            {
                // No more jobs to process, so unsubscribe the event
                this.EncodeService.EncodeCompleted -= this.EncodeServiceEncodeCompleted;

                this.BackupQueue(string.Empty);

                // Fire the event to tell connected services.
                this.OnQueueCompleted(new QueueCompletedEventArgs(false));
            }
        }

        #endregion
    }
}