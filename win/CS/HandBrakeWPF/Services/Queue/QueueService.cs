// --------------------------------------------------------------------------------------------------------------------
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
    using System.IO;
    using System.Linq;
    using System.Linq.Expressions;
    using System.Text.Json;
    using System.Timers;
    using System.Windows;

    using HandBrake.App.Core.Extensions;
    using HandBrake.App.Core.Utilities;
    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.Json.Queue;
    using HandBrake.Interop.Utilities;

    using HandBrakeWPF.Helpers;
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Encode;
    using HandBrakeWPF.Services.Encode.Factories;
    using HandBrakeWPF.Services.Encode.Interfaces;
    using HandBrakeWPF.Services.Encode.Model;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Logging.Interfaces;
    using HandBrakeWPF.Services.Queue.Interfaces;
    using HandBrakeWPF.Services.Queue.JobEventArgs;
    using HandBrakeWPF.Services.Queue.Model;
    using HandBrakeWPF.Utilities;

    using EncodeCompletedEventArgs = Encode.EventArgs.EncodeCompletedEventArgs;
    using GeneralApplicationException = HandBrake.App.Core.Exceptions.GeneralApplicationException;
    using ILog = Logging.Interfaces.ILog;
    using QueueCompletedEventArgs = EventArgs.QueueCompletedEventArgs;
    using QueueProgressEventArgs = EventArgs.QueueProgressEventArgs;

    public class QueueService : IQueueService
    {
        private static readonly object QueueLock = new object();

        private readonly List<ActiveJob> activeJobs = new List<ActiveJob>();
        private readonly object activeJobLock = new object();
        private readonly IUserSettingService userSettingService;
        private readonly ILog logService;
        private readonly IErrorService errorService;
        private readonly ILogInstanceManager logInstanceManager;
        private readonly DelayedActionProcessor delayedQueueBackupProcessor = new DelayedActionProcessor();

        private readonly IPortService portService;

        private readonly ObservableCollection<QueueTask> queue = new ObservableCollection<QueueTask>();
        private readonly string queueFile;
        private readonly object queueFileLock = new object();

        private readonly QueueResourceService hardwareResourceManager;

        private int allowedInstances;
        private int jobIdCounter = 0;
        private bool processIsolationEnabled;

        private EncodeTaskFactory encodeTaskFactory;

        private Timer queueTaskPoller;

        public QueueService(IUserSettingService userSettingService, ILog logService, IErrorService errorService, ILogInstanceManager logInstanceManager, IPortService portService)
        {
            this.userSettingService = userSettingService;
            this.hardwareResourceManager = new QueueResourceService(userSettingService);
            this.logService = logService;
            this.errorService = errorService;
            this.logInstanceManager = logInstanceManager;
            this.portService = portService;

            // If this is the first instance, just use the main queue file, otherwise add the instance id to the filename.
            this.queueFile = string.Format("{0}{1}.json", QueueRecoveryHelper.QueueFileName, GeneralUtilities.ProcessId);

            this.allowedInstances = this.userSettingService.GetUserSetting<int>(UserSettingConstants.SimultaneousEncodes);
            this.processIsolationEnabled = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.ProcessIsolationEnabled);

            this.encodeTaskFactory = new EncodeTaskFactory(this.userSettingService, true);

            this.hardwareResourceManager.Init();
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
                lock (QueueLock)
                {
                    return this.queue.Count(
                        item => item.Status == QueueItemStatus.Waiting && item.TaskType != QueueTaskType.Breakpoint);
                }
            }
        }

        public int ActiveJobCount
        {
            get
            {
                lock (activeJobLock)
                {
                    return this.activeJobs.Count;
                }
            }
        }
        
        public int ErrorCount
        {
            get
            {
                lock (QueueLock)
                {
                    return this.queue.Count(item => item.Status == QueueItemStatus.Error);
                }
            }
        }

        public int CompletedCount => this.queue.Count(item => item.Status == QueueItemStatus.Completed);

        public bool IsPaused { get; private set; }

        public bool IsProcessing { get; private set; }

        public bool IsEncoding => this.activeJobs.Any(service => service.IsEncoding);

        public ObservableCollection<QueueTask> Queue => this.queue;
        
        public void Add(QueueTask job)
        {
            lock (QueueLock)
            {
                this.queue.Add(job);
            }

            this.InvokeQueueChanged(EventArgs.Empty);
        }

        public void Add(List<QueueTask> tasks)
        {
            lock (QueueLock)
            {
                foreach (var job in tasks)
                {
                    this.queue.Add(job);
                }
            }

            this.InvokeQueueChanged(EventArgs.Empty);
        }

        public void BackupQueue(string exportPath)
        {
            lock (this.queueFileLock)
            {
                try
                {
                    string appDataPath = DirectoryUtilities.GetUserStoragePath(HandBrakeVersionHelper.IsNightly());
                    string tempPath = !string.IsNullOrEmpty(exportPath)
                        ? exportPath
                        : Path.Combine(appDataPath, string.Format(this.queueFile, string.Empty));

                    // Make a copy of the file before we replace it. This way, if we crash we can recover.
                    if (File.Exists(tempPath))
                    {
                        File.Copy(tempPath, tempPath + ".last", true);
                    }

                    using (StreamWriter writer = new StreamWriter(tempPath))
                    {
                        List<QueueTask> tasks;
                        lock (QueueLock)
                        {
                            tasks = this.queue.Where(item => item.Status != QueueItemStatus.Completed).ToList();
                        }

                        string queueJson = JsonSerializer.Serialize(tasks, JsonSettings.Options);
                        writer.Write(queueJson);
                    }

                    if (File.Exists(tempPath + ".last"))
                    {
                        File.Delete(tempPath + ".last");
                    }
                }
                catch (Exception exc)
                {
                    this.logService.LogMessage(string.Format("{1} # {0}{1} {2}{1}", "Queue Backup Failed!", Environment.NewLine, exc));
                }
            }
        }

        public void ExportCliJson(string exportPath)
        {
            List<QueueTask> jobs;
            lock (QueueLock)
            {
                jobs = this.queue.Where(item => item.Status != QueueItemStatus.Completed).ToList();
            }

            List<EncodeTask> workUnits = jobs.Select(job => job.Task).ToList();

            string json = this.GetQueueJson(workUnits);

            using (var strm = new StreamWriter(exportPath, false))
            {
                strm.Write(json);
                strm.Close();
                strm.Dispose();
            }
        }

        public void ExportJson(string exportPath)
        {
            List<QueueTask> jobs;
            lock (QueueLock)
            {
                jobs = this.queue.Where(item => item.Status != QueueItemStatus.Completed).ToList();
            }

            string json = JsonSerializer.Serialize(jobs, JsonSettings.Options);

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

                List<QueueTask> reloadedQueue = JsonSerializer.Deserialize<List<QueueTask>>(fileContent);

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
                        this.Stop(true);

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
                    if (task.Status == QueueItemStatus.InProgress || task.Status == QueueItemStatus.Paused)
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
            lock (QueueLock)
            {
                foreach (QueueTask job in this.queue)
                {
                    if (string.Equals(job.Task?.Destination, destination.Replace("\\\\", "\\"), StringComparison.OrdinalIgnoreCase) && (job.Status == QueueItemStatus.Waiting || job.Status == QueueItemStatus.InProgress))
                    {
                        return true;
                    }
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
            lock (QueueLock)
            {
                List<QueueTask> deleteList = this.queue.Where(i => i.Status != QueueItemStatus.InProgress).ToList();

                foreach (QueueTask item in deleteList)
                {
                    this.queue.Remove(item);
                }
            }

            this.InvokeQueueChanged(EventArgs.Empty);
        }

        public void ClearCompleted()
        {
            ThreadHelper.OnUIThread(
                () =>
                {
                    lock (QueueLock)
                    {
                        List<QueueTask> deleteList = this.queue.Where(task => task.Status == QueueItemStatus.Completed).ToList();
                        foreach (QueueTask item in deleteList)
                        {
                            this.queue.Remove(item);
                        }
                    }

                    this.InvokeQueueChanged(EventArgs.Empty);
                });
        }

        public List<string> GetLogFilePaths()
        {
            List<string> logPaths = new List<string>();
            lock (QueueLock)
            {
                foreach (QueueTask task in this.Queue)
                {
                    if (!string.IsNullOrEmpty(task.Statistics?.CompletedActivityLogPath))
                    {
                        logPaths.Add(task.Statistics?.CompletedActivityLogPath);
                    }
                }
            }

            return logPaths;
        }
        
        public QueueTask GetNextJobForProcessing()
        {
            lock (QueueLock)
            {
                if (this.queue.Count > 0)
                {
                    QueueTask task = this.queue.FirstOrDefault(q => q.Status == QueueItemStatus.Waiting);
                    if (task != null && task.TaskType == QueueTaskType.EncodeTask)
                    {
                        task.TaskToken = this.hardwareResourceManager.GetToken(task.Task);
                    }

                    return task;
                }
            }

            return null;
        }

        public void MoveToBottom(IList<QueueTask> moveItems)
        {
            lock (QueueLock)
            {
                this.queue.MoveToBottom(moveItems);
            }

            this.InvokeQueueChanged(EventArgs.Empty);
        }

        public void MoveToTop(IList<QueueTask> moveItems)
        {
            lock (QueueLock)
            {
                this.queue.MoveToTop(moveItems);
            }
            
            this.InvokeQueueChanged(EventArgs.Empty);
        }

        public void Remove(QueueTask job)
        {
            bool stoppedJob = false;
            lock (QueueLock)
            {
                ActiveJob activeJob = null;

                lock (activeJobLock)
                {
                    foreach (ActiveJob ajob in this.activeJobs)
                    {
                        if (Equals(ajob.Job, job))
                        {
                            activeJob = ajob;
                            activeJob.JobFinished += (a, e) =>
                            {
                                this.activeJobs.Remove(activeJob);

                                if (this.activeJobs.Count == 0 && this.IsPaused)
                                {
                                    this.IsPaused = false;
                                }
                            };
                            ajob.Stop();
                            stoppedJob = true;
                            break;
                        }
                    }
                }

                this.queue.Remove(job);
            }

            if (!stoppedJob)
            {
                this.InvokeQueueChanged(EventArgs.Empty);
            }
        }

        private void ActiveJob_JobFinished1(object sender, ActiveJobCompletedEventArgs e)
        {
            throw new NotImplementedException();
        }

        public void RestoreQueue(string importPath)
        {
            string appDataPath = DirectoryUtilities.GetUserStoragePath(HandBrakeVersionHelper.IsNightly());
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
                        list = JsonSerializer.Deserialize<List<QueueTask>>(queueJson);
                    }
                    catch (Exception exc)
                    {
                        throw new GeneralApplicationException(Resources.Queue_UnableToRestoreFile, Resources.Queue_UnableToRestoreFileExtended, exc);
                    }

                    if (list != null)
                    {
                        foreach (QueueTask item in list)
                        {
                            if (item.Status != QueueItemStatus.Completed && item.TaskType != QueueTaskType.Breakpoint)
                            {
                                // Reset InProgress/Error to Waiting so it can be processed
                                if (item.Status == QueueItemStatus.InProgress || item.Status == QueueItemStatus.Paused)
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

        public void Pause(bool pauseJobs)
        {
            if (pauseJobs)
            {
                lock (activeJobLock)
                {
                    foreach (ActiveJob job in this.activeJobs)
                    {
                        if (job.IsEncoding && !job.IsPaused)
                        {
                            job.Pause();
                        }
                    }
                }
            }

            this.IsProcessing = false;
            this.IsPaused = true;

            this.StopJobPolling();

            this.InvokeQueuePaused(EventArgs.Empty);
        }

        public void Start()
        {
            if (this.IsProcessing)
            {
                return;
            }

            this.IsPaused = false;

            ClearCancelledAndErrorJobs();

            this.allowedInstances = this.userSettingService.GetUserSetting<int>(UserSettingConstants.SimultaneousEncodes);
            this.processIsolationEnabled = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.ProcessIsolationEnabled);

            this.IsProcessing = true;

            // Unpause all active jobs.
            lock (activeJobLock)
            {
                foreach (ActiveJob job in this.activeJobs)
                {
                    job.Start();
                    this.InvokeJobProcessingStarted(new QueueProgressEventArgs(job.Job));
                }
            }

            this.ProcessNextJob();
        }

        public void Stop(bool stopExistingJobs)
        {
            if (!this.IsProcessing)
            {
                // Just in Case, tidyup.
                this.StopJobPolling();
                this.RemoveBreakPoints();

                this.IsPaused = false;

                return;
            }

            if (stopExistingJobs)
            {
                lock (activeJobLock)
                {
                    foreach (ActiveJob job in this.activeJobs)
                    {
                        if (job.IsEncoding || job.IsPaused)
                        {
                            job.Stop();
                        }
                    }
                }

                this.IsProcessing = false;
                this.IsPaused = false;

                this.StopJobPolling();
                this.RemoveBreakPoints();

                lock (activeJobLock)
                {
                    if (this.activeJobs.Count == 0)
                    {
                        this.InvokeQueueChanged(EventArgs.Empty);
                    }
                }

                this.InvokeQueueCompleted(new QueueCompletedEventArgs(true));
            }
            else
            {
                if (!this.QueueContainsStop())
                {
                    this.AddBreakPoint();
                }
            }
        }

        public List<QueueProgressStatus> GetQueueProgressStatus()
        {
            List<QueueProgressStatus> statuses = new List<QueueProgressStatus>();
            lock (activeJobLock)
            {
                foreach (ActiveJob job in this.activeJobs)
                {
                    statuses.Add(job.Job.JobProgress);
                }
            }
            return statuses;
        }

        public List<string> GetActiveJobDestinationDirectories()
        {
            List<string> directories = new List<string>();
            lock (activeJobLock)
            {
                foreach (ActiveJob job in this.activeJobs)
                {
                    directories.Add(job.Job.Task.Destination);
                }
            }

            return directories;
        }

        public void AddBreakPoint()
        {
            lock (QueueLock)
            {
                int foundIndex = -1;

                QueueTask firstWaitingJob = this.queue.FirstOrDefault(t => t.Status == QueueItemStatus.Waiting);
                if (firstWaitingJob != null)
                {
                    foundIndex = this.queue.IndexOf(firstWaitingJob);
                }

                if (foundIndex != -1)
                {
                    this.queue.Insert(foundIndex, new QueueTask(QueueTaskType.Breakpoint));
                }
            }
        }

        private void InvokeJobProcessingStarted(QueueProgressEventArgs e)
        {
            this.JobProcessingStarted?.Invoke(this, e);
        }

        private void InvokeQueueChanged(EventArgs e)
        {
            try
            {
                delayedQueueBackupProcessor.PerformTask(() => this.BackupQueue(string.Empty), 200);
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

        private void ProcessNextJob()
        {
            this.StopJobPolling();
            this.CheckAndHandleWork(); // Kick the first job off right away.

            // Reset the timer.
            this.queueTaskPoller = new Timer();
            this.queueTaskPoller.Interval = this.allowedInstances > 1 ? 2000 : 1000;
            this.queueTaskPoller.Elapsed += (o, e) => { CheckAndHandleWork(); };
            this.queueTaskPoller.Start();
        }

        private void StopJobPolling()
        {
            if (this.queueTaskPoller != null)
            {
                this.queueTaskPoller.Stop();
                this.queueTaskPoller = null;
            }
        }
        
        private void CheckAndHandleWork()
        {
            if (!this.processIsolationEnabled)
            {
                this.allowedInstances = 1;
            }

            lock (activeJobLock)
            {
                if (this.activeJobs.Count >= this.allowedInstances)
                {
                    return;
                }
            }

            if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.ClearCompletedFromQueue))
            {
                this.ClearCompleted();
            }

            QueueTask job = this.GetNextJobForProcessing();
            if (job != null)
            {
                if (job.TaskType == QueueTaskType.Breakpoint)
                {
                    this.HandleBreakPoint(job);
                    return;
                }
                
                // Hardware encoders can typically only have 1 or two instances running at any given time. As such, we must have a  HardwareResourceToken to continue.
                if (job.TaskToken == Guid.Empty)
                {
                    return; // Hardware is busy, we'll try again later when another job completes.
                }

                if (CheckDiskSpace(job))
                {
                    return; // Don't start the next job.
                }

                this.jobIdCounter = this.jobIdCounter + 1;
                IEncode libEncode = new LibEncode(this.userSettingService, this.logInstanceManager, this.jobIdCounter, this.portService);
                ActiveJob activeJob = new ActiveJob(job, libEncode);
                activeJob.JobFinished += this.ActiveJob_JobFinished;
                activeJob.JobStatusUpdated += this.ActiveJob_JobStatusUpdated;
                lock (activeJobLock)
                {
                    this.activeJobs.Add(activeJob);
                }
                
                activeJob.Start();

                this.IsProcessing = true;

                this.InvokeQueueChanged(EventArgs.Empty);
                this.InvokeJobProcessingStarted(new QueueProgressEventArgs(job));
                this.BackupQueue(string.Empty);
            }
            else
            {
                this.BackupQueue(string.Empty);

                bool queueComplete = false;
                lock (activeJobLock)
                {
                    if (!this.activeJobs.Any(a => a.IsEncoding))
                    {
                        this.StopJobPolling();

                        queueComplete = true;
                    }
                }

                if (queueComplete)
                {
                    // Fire the event to tell connected services.
                    this.InvokeQueueCompleted(new QueueCompletedEventArgs(false));
                }
            }
        }

        private void ActiveJob_JobStatusUpdated(object sender, Encode.EventArgs.EncodeProgressEventArgs e)
        {
            this.OnQueueJobStatusChanged();
        }

        private void ActiveJob_JobFinished(object sender, ActiveJobCompletedEventArgs e)
        {
            this.hardwareResourceManager.ReleaseToken(e.Job.Job.Task.VideoEncoder, e.Job.Job.TaskToken);

            lock (activeJobLock)
            {
                this.activeJobs.Remove(e.Job);
            }

            this.OnEncodeCompleted(e.EncodeEventArgs);

            this.InvokeQueueChanged(EventArgs.Empty);
        }

        private void InvokeQueueCompleted(QueueCompletedEventArgs e)
        {
            ThreadHelper.OnUIThread(
                () =>
                {
                    this.hardwareResourceManager.ClearTokens();
                    this.IsProcessing = false;
                    this.QueueCompleted?.Invoke(this, e);
                });
        }

        private void OnQueueJobStatusChanged()
        {
            // TODO add support for delayed notifications here to avoid overloading the UI when we run multiple encodes. 
            this.QueueJobStatusChanged?.Invoke(this, EventArgs.Empty);
        }

        private void OnEncodeCompleted(EncodeCompletedEventArgs e)
        {
            this.EncodeCompleted?.Invoke(this, e);
        }

        private void InvokeQueuePaused(EventArgs e)
        {
            this.IsProcessing = false;
            
            EventHandler handler = this.QueuePaused;
            handler?.Invoke(this, e);
        }

        private string GetQueueJson(List<EncodeTask> tasks)
        {
            List<Task> queueJobs = new List<Task>();
            foreach (var item in tasks)
            {
                Task task = new Task { Job = this.encodeTaskFactory.Create(item) };
                queueJobs.Add(task);
            }

            return JsonSerializer.Serialize(queueJobs, JsonSettings.Options);
        }

        private bool CheckDiskSpace(QueueTask job)
        {
            if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.PauseOnLowDiskspace) && !DriveUtilities.HasMinimumDiskSpace(job.Task.Destination, this.userSettingService.GetUserSetting<long>(UserSettingConstants.PauseQueueOnLowDiskspaceLevel)))
            {
                this.logService.LogMessage(Resources.PauseOnLowDiskspace);
                job.Status = QueueItemStatus.Waiting;
                this.Pause(true);
                this.BackupQueue(string.Empty);
                return true; // Don't start the next job.
            }

            return false;
        }

        private void HandleBreakPoint(QueueTask task)
        {
            lock (QueueLock)
            {
                lock (activeJobLock)
                {
                    if (this.activeJobs.Count != 0)
                    {
                        return; // Wait for jobs to finish!
                    }
                }

                this.StopJobPolling();

                // Remove the Breakpoint
                ThreadHelper.OnUIThread(() => this.queue.Remove(task));
            }

            this.IsProcessing = false;
            this.IsPaused = false;

            // Setting the flag will allow or prevent the when done actions to be processed. 
            this.InvokeQueueCompleted(new QueueCompletedEventArgs(false));
        }

        private void RemoveBreakPoints()
        {
            List<QueueTask> tasks = this.queue.Where(t => t.TaskType == QueueTaskType.Breakpoint).ToList();
            foreach (var task in tasks)
            {
                this.queue.Remove(task);
            }
        }

        private bool QueueContainsStop()
        {
            return this.queue.Any(t => t.TaskType == QueueTaskType.Breakpoint);
        }

        private void ClearCancelledAndErrorJobs()
        {
            // Called when queue starts to clear previous work off the queue that was left in an error state.
            // This will remove confusion over the "Queue completed with errors" message at the end of the queue batch if the user doesn't clean it up themselves
            lock (QueueLock)
            {
                foreach (QueueTask t in this.queue.ToList())
                {
                    if (t.Status == QueueItemStatus.Cancelled || t.Status == QueueItemStatus.Error)
                    {
                        this.queue.Remove(t);
                    }
                }
            }
        }
    }
}