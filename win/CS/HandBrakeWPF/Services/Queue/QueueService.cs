﻿// --------------------------------------------------------------------------------------------------------------------
// <copyright file="QueueService.cs" company="HandBrake Project (http://handbrake.fr)">
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
    using System.Collections.ObjectModel;
    using System.Diagnostics;
    using System.IO;
    using System.Linq;
    using System.Windows;

    using Caliburn.Micro;

    using HandBrake.Interop.Interop.HbLib.Wrappers.Interfaces;
    using HandBrake.Interop.Interop.Json.Queue;
    using HandBrake.Interop.Interop.Providers.Interfaces;
    using HandBrake.Interop.Model;
    using HandBrake.Interop.Utilities;

    using HandBrakeWPF.Factories;
    using HandBrakeWPF.Helpers;
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Encode.EventArgs;
    using HandBrakeWPF.Services.Encode.Factories;
    using HandBrakeWPF.Services.Encode.Model;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Queue.Model;
    using HandBrakeWPF.Utilities;

    using Newtonsoft.Json;

    using EncodeCompletedEventArgs = HandBrakeWPF.Services.Encode.EventArgs.EncodeCompletedEventArgs;
    using Execute = Caliburn.Micro.Execute;
    using GeneralApplicationException = HandBrakeWPF.Exceptions.GeneralApplicationException;
    using IEncode = HandBrakeWPF.Services.Encode.Interfaces.IEncode;
    using ILog = HandBrakeWPF.Services.Logging.Interfaces.ILog;
    using QueueCompletedEventArgs = HandBrakeWPF.EventArgs.QueueCompletedEventArgs;
    using QueueProgressEventArgs = HandBrakeWPF.EventArgs.QueueProgressEventArgs;

    public class QueueService : Interfaces.IQueueService
    {
        private static readonly object QueueLock = new object();

        private readonly IEncode encodeService;
        private readonly IUserSettingService userSettingService;
        private readonly ILog logService;
        private readonly IErrorService errorService;

        private readonly ObservableCollection<QueueTask> queue = new ObservableCollection<QueueTask>();
        private readonly string queueFile;
        private bool clearCompleted;

        public QueueService(IEncode encodeService, IUserSettingService userSettingService, ILog logService, IErrorService errorService)
        {
            this.encodeService = encodeService;
            this.userSettingService = userSettingService;
            this.logService = logService;
            this.errorService = errorService;

            // If this is the first instance, just use the main queue file, otherwise add the instance id to the filename.
            this.queueFile = string.Format("{0}{1}.json", QueueRecoveryHelper.QueueFileName, GeneralUtilities.ProcessId);
        }

        public event EventHandler<QueueProgressEventArgs> JobProcessingStarted;

        public event EventHandler QueueChanged;

        public event EventHandler<QueueCompletedEventArgs> QueueCompleted;

        public event EventHandler QueuePaused;

        public event EventHandler QueueJobStatusChanged;

        public event EventHandler<EncodeCompletedEventArgs> EncodeCompleted;

        public int Count
        {
            get
            {
                return this.queue.Count(item => item.Status == QueueItemStatus.Waiting);
            }
        }

        public int ErrorCount
        {
            get
            {
                return this.queue.Count(item => item.Status == QueueItemStatus.Error);
            }
        }

        public bool IsPaused { get; private set; }

        public bool IsProcessing { get; private set; }

        public bool IsEncoding => this.encodeService.IsEncoding;

        public QueueTask LastProcessedJob { get; set; }

        public ObservableCollection<QueueTask> Queue
        {
            get
            {
                return this.queue;
            }
        }
        
        public void Add(QueueTask job)
        {
            lock (QueueLock)
            {
                this.queue.Add(job);
                this.InvokeQueueChanged(EventArgs.Empty);
            }
        }

        public void BackupQueue(string exportPath)
        {
            Stopwatch watch = Stopwatch.StartNew();

            string appDataPath = DirectoryUtilities.GetUserStoragePath(VersionHelper.IsNightly());
            string tempPath = !string.IsNullOrEmpty(exportPath)
                                  ? exportPath
                                  : Path.Combine(appDataPath, string.Format(this.queueFile, string.Empty));

            // Make a copy of the file before we replace it. This way, if we crash we can recover.
            if (File.Exists(tempPath))
            {
                File.Copy(tempPath, tempPath + ".last");
            }

            using (StreamWriter writer = new StreamWriter(tempPath))
            {
                List<QueueTask> tasks = this.queue.Where(item => item.Status != QueueItemStatus.Completed).ToList();
                string queueJson = JsonConvert.SerializeObject(tasks, Formatting.Indented);
                writer.Write(queueJson);
            }

            if (File.Exists(tempPath + ".last"))
            {
                File.Delete(tempPath + ".last");
            }

            watch.Stop();
            Debug.WriteLine("Queue Save (ms): " + watch.ElapsedMilliseconds);
        }

        public void ExportCliJson(string exportPath)
        {
            List<QueueTask> jobs = this.queue.Where(item => item.Status != QueueItemStatus.Completed).ToList();
            List<EncodeTask> workUnits = jobs.Select(job => job.Task).ToList();
            HBConfiguration config = HBConfigurationFactory.Create(); // Default to current settings for now. These will hopefully go away in the future.

            string json = this.GetQueueJson(workUnits, config);

            using (var strm = new StreamWriter(exportPath, false))
            {
                strm.Write(json);
                strm.Close();
                strm.Dispose();
            }
        }

        public void ExportJson(string exportPath)
        {
            List<QueueTask> jobs = this.queue.Where(item => item.Status != QueueItemStatus.Completed).ToList();

            JsonSerializerSettings settings = new JsonSerializerSettings { NullValueHandling = NullValueHandling.Ignore };

            string json = JsonConvert.SerializeObject(jobs, Formatting.Indented, settings);

            using (var strm = new StreamWriter(exportPath, false))
            {
                strm.Write(json);
                strm.Close();
                strm.Dispose();
            }
        }

        public void ImportJson(string path)
        {
            using (StreamReader reader = new StreamReader(path))
            {
                string fileContent = reader.ReadToEnd();
                if (string.IsNullOrEmpty(fileContent))
                {
                    return;
                }

                List<QueueTask> reloadedQueue = JsonConvert.DeserializeObject<List<QueueTask>>(fileContent);

                if (reloadedQueue == null)
                {
                    return;
                }

                List<QueueTask> duplicates = queue.Where(task => reloadedQueue.Any(queueTask => queueTask.TaskId == task.TaskId)).ToList();
                bool replaceDuplicates = false;
                if (duplicates.Any())
                {
                    MessageBoxResult result = this.errorService.ShowMessageBox(
                        Properties.Resources.QueueService_DuplicatesQuestion,
                        Properties.Resources.QueueService_DuplicatesTitle,
                        MessageBoxButton.YesNo,
                        MessageBoxImage.Question);

                    if (result == MessageBoxResult.Yes)
                    {
                        this.Stop();

                        foreach (QueueTask task in duplicates)
                        {
                            this.queue.Remove(task);
                        }

                        replaceDuplicates = true;
                    }
                }

                foreach (QueueTask task in reloadedQueue)
                {
                    // Reset the imported jobs that were running in a previous session.
                    if (task.Status == QueueItemStatus.InProgress)
                    {
                        task.Status = QueueItemStatus.Waiting;
                        task.Statistics.Reset();
                    }

                    // Ignore jobs if the user has chosen not to replace them.
                    if (!replaceDuplicates && this.queue.Any(s => s.TaskId == task.TaskId))
                    {
                        continue;
                    }

                    // If the above conditions are not met, add it back in.
                    this.queue.Add(task);
                }

                if (reloadedQueue.Count > 0)
                {
                    this.InvokeQueueChanged(EventArgs.Empty);
                }
            }
        }
        
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

        public void RetryJob(QueueTask task)
        {
            if (task == null)
            {
                return;
            }

            task.Status = QueueItemStatus.Waiting;
            task.Statistics.Reset();
            this.BackupQueue(null);
     
            this.InvokeQueueChanged(EventArgs.Empty);
        }

        public void Clear()
        {
            List<QueueTask> deleteList = this.queue.Where(i => i.Status != QueueItemStatus.InProgress).ToList();

            foreach (QueueTask item in deleteList)
            {
                this.queue.Remove(item);
            }

            this.InvokeQueueChanged(EventArgs.Empty);
        }

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

        public QueueTask GetNextJobForProcessing()
        {
            if (this.queue.Count > 0)
            {
                return this.queue.FirstOrDefault(q => q.Status == QueueItemStatus.Waiting);
            }

            return null;
        }

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

        public void Remove(QueueTask job)
        {
            lock (QueueLock)
            {
                this.queue.Remove(job);
                this.InvokeQueueChanged(EventArgs.Empty);
            }
        }

        public void ResetJobStatusToWaiting(QueueTask job)
        {
            if (job.Status != QueueItemStatus.Error && job.Status != QueueItemStatus.Completed)
            {
                throw new GeneralApplicationException(
                    Resources.Error, Resources.Queue_UnableToResetJob, null);
            }

            job.Status = QueueItemStatus.Waiting;
        }

        public void RestoreQueue(string importPath)
        {
            string appDataPath = DirectoryUtilities.GetUserStoragePath(VersionHelper.IsNightly());
            string tempPath = !string.IsNullOrEmpty(importPath)
                                  ? importPath
                                  : (appDataPath + string.Format(this.queueFile, string.Empty));

            if (File.Exists(tempPath))
            {
                bool invokeUpdate = false;
                using (StreamReader stream = new StreamReader(!string.IsNullOrEmpty(importPath) ? importPath : tempPath))
                {
                    string queueJson = stream.ReadToEnd();
                    List<QueueTask> list;

                    try
                    {
                        list = JsonConvert.DeserializeObject<List<QueueTask>>(queueJson);
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

                if (invokeUpdate)
                {
                    this.InvokeQueueChanged(EventArgs.Empty);
                }
            }
        }

        public void Pause()
        {
            this.IsProcessing = false;
            this.IsPaused = true;
            this.InvokeQueuePaused(EventArgs.Empty);
        }

        public void PauseEncode()
        {
            if (this.encodeService.IsEncoding && !this.encodeService.IsPasued)
            {
                this.encodeService.Pause();
                this.LastProcessedJob.Statistics.SetPaused(true);
                this.LastProcessedJob.Status = QueueItemStatus.Paused;
            }

            this.Pause();
        }

        public void Start(bool isClearCompleted)
        {
            if (this.IsProcessing)
            {
                return;
            }

            this.IsPaused = false;

            this.clearCompleted = isClearCompleted;

            this.encodeService.EncodeCompleted -= this.EncodeServiceEncodeCompleted;
            this.encodeService.EncodeCompleted += this.EncodeServiceEncodeCompleted;

            this.encodeService.EncodeStatusChanged -= this.EncodeStatusChanged;
            this.encodeService.EncodeStatusChanged += this.EncodeStatusChanged;

            if (this.encodeService.IsPasued)
            {
                this.encodeService.Resume();
                this.IsProcessing = true;
                this.InvokeJobProcessingStarted(new QueueProgressEventArgs(this.LastProcessedJob));
                this.LastProcessedJob.Statistics.SetPaused(false);
            }

            if (!this.encodeService.IsEncoding)
            {
                this.ProcessNextJob();
            }
            else
            {
                this.IsProcessing = true;
            }
        }

        public void Stop()
        {
            if (this.encodeService.IsEncoding)
            {
                this.encodeService.Stop();
            }

            this.IsProcessing = false;
            this.IsPaused = false;
            this.InvokeQueuePaused(EventArgs.Empty);
        }

        public List<QueueProgressStatus> GetQueueProgressStatus()
        {
            return new List<QueueProgressStatus>() { this.LastProcessedJob?.JobProgress };
        }
        
        private void EncodeServiceEncodeCompleted(object sender, EncodeCompletedEventArgs e)
        {
            this.LastProcessedJob.Status = QueueItemStatus.Completed;
            this.LastProcessedJob.Statistics.EndTime = DateTime.Now;
            this.LastProcessedJob.Statistics.CompletedActivityLogPath = e.ActivityLogPath;
            this.LastProcessedJob.Statistics.FinalFileSize = e.FinalFilesizeInBytes;
            
            this.LastProcessedJob.JobProgress.ClearStatusDisplay();

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
                this.encodeService.EncodeStatusChanged -= this.EncodeStatusChanged;
                this.encodeService.EncodeCompleted -= this.EncodeServiceEncodeCompleted;
                this.BackupQueue(string.Empty);
                this.OnQueueCompleted(new QueueCompletedEventArgs(true));
            }

            this.OnEncodeCompleted(e);
        }

        private void EncodeStatusChanged(object sender, EncodeProgressEventArgs e)
        {
            this.LastProcessedJob?.JobProgress.Update(e);
            this.OnQueueJobStatusChanged();
        }

        private void InvokeJobProcessingStarted(QueueProgressEventArgs e)
        {
            this.JobProcessingStarted?.Invoke(this, e);
        }

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

        private void InvokeQueuePaused(EventArgs e)
        {
            this.IsProcessing = false;

            this.LastProcessedJob?.JobProgress.SetPaused();

            EventHandler handler = this.QueuePaused;
            if (handler != null)
            {
                handler(this, e);
            }
        }

        private void ProcessNextJob()
        {
            QueueTask job = this.GetNextJobForProcessing();
            if (job != null)
            {
                if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.PauseOnLowDiskspace) && !DriveUtilities.HasMinimumDiskSpace(job.Task.Destination, this.userSettingService.GetUserSetting<long>(UserSettingConstants.PauseQueueOnLowDiskspaceLevel)))
                {
                    this.logService.LogMessage(Resources.PauseOnLowDiskspace);
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

                if (!Directory.Exists(Path.GetDirectoryName(job.Task.Destination)))
                {
                    this.EncodeServiceEncodeCompleted(null, new EncodeCompletedEventArgs(false, null, "Destination Directory Missing", null, null, null, 0));
                    this.BackupQueue(string.Empty);
                    return;
                }

                this.encodeService.Start(job.Task, job.Configuration, job.SelectedPresetKey);
                this.BackupQueue(string.Empty);
            }
            else
            {
                // No more jobs to process, so unsubscribe the event
                this.encodeService.EncodeStatusChanged -= this.EncodeStatusChanged;
                this.encodeService.EncodeCompleted -= this.EncodeServiceEncodeCompleted;
                
                this.BackupQueue(string.Empty);

                // Fire the event to tell connected services.
                this.OnQueueCompleted(new QueueCompletedEventArgs(false));
            }
        }

        private void OnQueueCompleted(QueueCompletedEventArgs e)
        {
            this.IsProcessing = false;
            this.QueueCompleted?.Invoke(this, e);
        }

        private void OnQueueJobStatusChanged()
        {
            // TODO add support for delayed notificaitons here to avoid overloading the UI when we run multiple encodes. 
            this.QueueJobStatusChanged?.Invoke(this, EventArgs.Empty);
        }

        private void OnEncodeCompleted(EncodeCompletedEventArgs e)
        {
            this.EncodeCompleted?.Invoke(this, e);
        }

        /// <summary>
        /// For a given set of tasks, return the Queue JSON that can be used for the CLI.
        /// </summary>
        /// <param name="tasks">
        /// The tasks.
        /// </param>
        /// <param name="configuration">
        /// The configuration.
        /// </param>
        /// <returns>
        /// The <see cref="string"/>.
        /// </returns>
        private string GetQueueJson(List<EncodeTask> tasks, HBConfiguration configuration)
        {
            JsonSerializerSettings settings = new JsonSerializerSettings
                                              {
                                                  NullValueHandling = NullValueHandling.Ignore,
                                              };

            IHbFunctionsProvider provider = IoC.Get<IHbFunctionsProvider>(); // TODO remove IoC call.
            IHbFunctions hbFunctions = provider.GetHbFunctionsWrapper();

            List<Task> queueJobs = new List<Task>();
            foreach (var item in tasks)
            {
                Task task = new Task { Job = EncodeTaskFactory.Create(item, configuration, hbFunctions) };
                queueJobs.Add(task);
            }

            return JsonConvert.SerializeObject(queueJobs, Formatting.Indented, settings);
        }
    }
}