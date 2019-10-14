// --------------------------------------------------------------------------------------------------------------------
// <copyright file="QueueViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Preview View Model
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.IO;
    using System.Linq;
    using System.Runtime.CompilerServices;
    using System.Windows;

    using Caliburn.Micro;

    using HandBrakeWPF.EventArgs;
    using HandBrakeWPF.Extensions;
    using HandBrakeWPF.Model.Options;
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Queue.Interfaces;
    using HandBrakeWPF.Services.Queue.Model;
    using HandBrakeWPF.Utilities;
    using HandBrakeWPF.ViewModels.Interfaces;

    using Microsoft.Win32;

    using EncodeCompletedEventArgs = Services.Encode.EventArgs.EncodeCompletedEventArgs;
    using EncodeProgressEventArgs = Services.Encode.EventArgs.EncodeProgressEventArgs;

    /// <summary>
    /// The Preview View Model
    /// </summary>
    public class QueueViewModel : ViewModelBase, IQueueViewModel
    {
        #region Constants and Fields

        private readonly IErrorService errorService;
        private readonly IUserSettingService userSettingService;
        private readonly IQueueService queueProcessor;
        private string jobStatus;
        private string jobsPending;
        private WhenDone whenDoneAction;
        private QueueTask selectedTask;
        private bool isQueueRunning;
        private double progressValue;
        private bool intermediateProgress;
        private bool showEncodeProgress;

        #endregion

        #region Constructors and Destructors

        /// <summary>
        /// Initializes a new instance of the <see cref="QueueViewModel"/> class.
        /// </summary>
        /// <param name="userSettingService">
        /// The user Setting Service.
        /// </param>
        /// <param name="queueProcessor">
        /// The Queue Processor Service 
        /// </param>
        /// <param name="errorService">
        /// The Error Service 
        /// </param>
        public QueueViewModel(IUserSettingService userSettingService, IQueueService queueProcessor, IErrorService errorService)
        {
            this.userSettingService = userSettingService;
            this.queueProcessor = queueProcessor;
            this.errorService = errorService;
            this.Title = Resources.QueueViewModel_Queue;
            this.JobsPending = Resources.QueueViewModel_NoEncodesPending;
            this.JobStatus = string.Empty;
            this.SelectedItems = new BindingList<QueueTask>();
            this.DisplayName = "Queue";
            this.IsQueueRunning = false;

            this.WhenDoneAction = (WhenDone)this.userSettingService.GetUserSetting<int>(UserSettingConstants.WhenCompleteAction);
        }

        #endregion

        #region Properties

        /// <summary>
        /// Gets or sets a value indicating whether the Queue is paused or not..
        /// </summary>
        public bool IsQueueRunning
        {
            get
            {
                return this.isQueueRunning;
            }

            set
            {
                if (value == this.isQueueRunning) return;
                this.isQueueRunning = value;
                this.NotifyOfPropertyChange(() => this.IsQueueRunning);
            }
        }

        /// <summary>
        /// Gets or sets JobStatus.
        /// </summary>
        public string JobStatus
        {
            get
            {
                return this.jobStatus;
            }

            set
            {
                this.jobStatus = value;
                this.NotifyOfPropertyChange(() => this.JobStatus);
                this.NotifyOfPropertyChange(() => this.IsJobStatusVisible);
            }
        }

        public bool IsJobStatusVisible => !string.IsNullOrEmpty(this.JobStatus)  && this.SelectedTask?.Status == QueueItemStatus.InProgress;

        /// <summary>
        /// Gets or sets JobsPending.
        /// </summary>
        public string JobsPending
        {
            get
            {
                return this.jobsPending;
            }

            set
            {
                this.jobsPending = value;
                this.NotifyOfPropertyChange(() => this.JobsPending);
            }
        }

        /// <summary>
        /// Gets or sets WhenDoneAction.
        /// </summary>
        public WhenDone WhenDoneAction
        {
            get
            {
                return this.whenDoneAction;
            }

            set
            {
                this.whenDoneAction = value;
                this.NotifyOfPropertyChange(() => this.WhenDoneAction);
            }
        }

        /// <summary>
        /// Gets the queue tasks.
        /// </summary>
        public ObservableCollection<QueueTask> QueueTasks
        {
            get
            {
                return this.queueProcessor.Queue;
            }
        }

        /// <summary>
        /// Gets or sets the selected items.
        /// </summary>
        public BindingList<QueueTask> SelectedItems { get; set; }

        public QueueTask SelectedTask
        {
            get
            {
                return this.selectedTask;
            }

            set
            {
                if (Equals(value, this.selectedTask)) return;
                this.selectedTask = value;
                this.NotifyOfPropertyChange(() => this.SelectedTask);
                this.HandleLogData();

                this.NotifyOfPropertyChange(() => this.CanRetryJob);
                this.NotifyOfPropertyChange(() => this.CanEditJob);
                this.NotifyOfPropertyChange(() => this.CanRemoveJob);
                this.NotifyOfPropertyChange(() => this.CanPerformActionOnSource);
                this.NotifyOfPropertyChange(() => this.CanPlayFile);
                this.NotifyOfPropertyChange(() => this.StatsVisible);
            }
        }

        public string ActivityLog { get; private set; }

        public bool CanRetryJob => this.SelectedTask != null && this.SelectedTask.Status != QueueItemStatus.Waiting && this.SelectedTask.Status != QueueItemStatus.InProgress;

        public bool CanEditJob => this.SelectedTask != null;

        public bool CanRemoveJob => this.SelectedTask != null;

        public bool CanPerformActionOnSource => this.SelectedTask != null;

        public bool CanPlayFile =>
            this.SelectedTask != null && this.SelectedTask.Task.Destination != null && 
            this.SelectedTask.Status == QueueItemStatus.Completed && File.Exists(this.SelectedTask.Task.Destination);

        public double ProgressValue
        {
            get => this.progressValue;
            set
            {
                if (value == this.progressValue) return;
                this.progressValue = value;
                this.NotifyOfPropertyChange(() => this.ProgressValue);
            }
        }

        public bool IntermediateProgress
        {
            get => this.intermediateProgress;
            set
            {
                if (value == this.intermediateProgress) return;
                this.intermediateProgress = value;
                this.NotifyOfPropertyChange(() => this.IntermediateProgress);
            }
        }

        public bool ShowEncodeProgress
        {
            get => this.showEncodeProgress;
            set
            {
                if (value == this.showEncodeProgress) return;
                this.showEncodeProgress = value;
                this.NotifyOfPropertyChange(() => this.ShowEncodeProgress);
            }
        }

        public bool StatsVisible
        {
            get
            {
                if (this.SelectedTask != null &&
                    (this.selectedTask.Status == QueueItemStatus.Completed || this.selectedTask.Status == QueueItemStatus.Error || this.selectedTask.Status == QueueItemStatus.InProgress))
                {
                    return true;
                }

                return false;
            }
        }

        #endregion

        #region Public Methods

        /// <summary>
        /// Update the When Done Setting
        /// </summary>
        /// <param name="action">
        /// The action.
        /// </param>
        public void WhenDone(int action)
        {
            this.WhenDone(action, true);
        }

        /// <summary>
        /// Update the When Done Setting
        /// </summary>
        /// <param name="action">
        /// The action.
        /// </param>
        /// <param name="saveChange">
        /// Save the change to the setting. Use false when updating UI.
        /// </param>
        public void WhenDone(int action, bool saveChange)
        {
            this.WhenDoneAction = (WhenDone)action;

            if (saveChange)
            {
                this.userSettingService.SetUserSetting(UserSettingConstants.WhenCompleteAction, action);
            }

            IOptionsViewModel ovm = IoC.Get<IOptionsViewModel>();
            ovm.UpdateSettings();
        }

        /// <summary>
        /// Clear the Queue
        /// </summary>
        public void Clear()
        {
            MessageBoxResult result = this.errorService.ShowMessageBox(
                Resources.QueueViewModel_ClearQueueConfrimation, Resources.Confirm, MessageBoxButton.YesNo, MessageBoxImage.Warning);
            if (result == MessageBoxResult.Yes)
            {
                this.queueProcessor.Clear();
            }
        }

        /// <summary>
        /// Clear Completed Items
        /// </summary>
        public void ClearCompleted()
        {
            this.queueProcessor.ClearCompleted();
        }

        /// <summary>
        /// Close this window.
        /// </summary>
        public void Close()
        {
            this.TryClose();
        }

        /// <summary>
        /// Handle the On Window Load
        /// </summary>
        public override void OnLoad()
        {
            // Setup the window to the correct state.
            this.IsQueueRunning = this.queueProcessor.EncodeService.IsEncoding;
            this.JobsPending = string.Format(Resources.QueueViewModel_JobsPending, this.queueProcessor.Count);

            base.OnLoad();
        }

        /// <summary>
        /// Can Pause the Queue.
        /// Used by Caliburn Micro to enable/disable the context menu item.
        /// </summary>
        /// <returns>
        /// True when we can pause the queue.
        /// </returns>
        public bool CanPauseQueue()
        {
            return this.IsQueueRunning;
        }

        /// <summary>
        /// Pause the Queue
        /// </summary>
        public void PauseQueue()
        {
            this.queueProcessor.Pause();

            this.JobsPending = string.Format(Resources.QueueViewModel_JobsPending, this.queueProcessor.Count);
            this.IsQueueRunning = false;

            this.errorService.ShowMessageBox(Resources.QueueViewModel_QueuePauseNotice, Resources.QueueViewModel_Queue,
                MessageBoxButton.OK, MessageBoxImage.Information);
        }

        /// <summary>
        /// Pause the Queue
        /// </summary>
        /// <remarks>
        /// Prevents evaluation of CanPauseQueue
        /// </remarks>
        public void PauseQueueToolbar()
        {
            this.PauseQueue();
        }

        /// <summary>
        /// The remove selected jobs.
        /// </summary>
        public void RemoveSelectedJobs()
        {
            if (this.SelectedItems.Count == 0)
            {
                return;
            }

            MessageBoxResult result =
                  this.errorService.ShowMessageBox(
                      Resources.QueueViewModel_DelSelectedJobConfirmation,
                      Resources.Question,
                      MessageBoxButton.YesNo,
                      MessageBoxImage.Question);

            if (result == MessageBoxResult.No)
            {
                return;
            }

            List<QueueTask> tasksToRemove = this.SelectedItems.ToList();
            foreach (QueueTask job in tasksToRemove)
            {
                this.RemoveJob(job);
            }
        }

        /// <summary>
        /// Remove a Job from the queue
        /// </summary>
        /// <param name="queueTask">
        /// The Job to remove from the queue
        /// </param>
        public void RemoveJob(object queueTask)
        {
            QueueTask task = queueTask as QueueTask;
            if (task == null)
            {
                return;
            }

            bool removed = false;
            int index = this.QueueTasks.IndexOf(task);

            if (task.Status == QueueItemStatus.InProgress)
            {
                MessageBoxResult result =
                    this.errorService.ShowMessageBox(
                        Resources.QueueViewModel_JobCurrentlyRunningWarning,
                        Resources.Warning,
                        MessageBoxButton.YesNo,
                        MessageBoxImage.Question);

                if (result == MessageBoxResult.Yes)
                {
                    this.queueProcessor.EncodeService.Stop();
                    this.queueProcessor.Remove(task);
                    removed = true;
                }
            }
            else
            {
                this.queueProcessor.Remove(task);
                removed = true;
            }

            if (this.QueueTasks.Any() && removed)
            {              
                this.SelectedTask = index > 1 ? this.QueueTasks[index - 1] : this.QueueTasks.FirstOrDefault();
            }
        }

        /// <summary>
        /// Reset the job state to waiting.
        /// </summary>
        /// <param name="task">
        /// The task.
        /// </param>
        public void RetryJob(QueueTask task)
        {
            if (task == null)
            {
                return;
            }

            task.Status = QueueItemStatus.Waiting;
            task.Statistics.Reset();
            this.queueProcessor.BackupQueue(null);
            this.JobsPending = string.Format(Resources.QueueViewModel_JobsPending, this.queueProcessor.Count);
            this.NotifyOfPropertyChange(() => this.CanRetryJob);
        }

        /// <summary>
        /// Start Encode
        /// </summary>
        public void StartQueue()
        {
            if (!this.QueueTasks.Any(a => a.Status == QueueItemStatus.Waiting || a.Status == QueueItemStatus.InProgress))
            {
                this.errorService.ShowMessageBox(
                    Resources.QueueViewModel_NoPendingJobs, Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            var firstOrDefault = this.QueueTasks.FirstOrDefault(s => s.Status == QueueItemStatus.Waiting);
            if (firstOrDefault != null && !DriveUtilities.HasMinimumDiskSpace(firstOrDefault.Task.Destination,
                    this.userSettingService.GetUserSetting<long>(UserSettingConstants.PauseQueueOnLowDiskspaceLevel)))
            {
                this.errorService.ShowMessageBox(Resources.Main_LowDiskspace, Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            this.JobsPending = string.Format(Resources.QueueViewModel_JobsPending, this.queueProcessor.Count);
            this.IsQueueRunning = true;

            this.queueProcessor.Start(userSettingService.GetUserSetting<bool>(UserSettingConstants.ClearCompletedFromQueue));
        }

        public void ExportCli()
        {
            SaveFileDialog dialog = new SaveFileDialog
            {
                Filter = "Json (*.json)|*.json",
                OverwritePrompt = true,
                DefaultExt = ".json",
                AddExtension = true
            };

            if (dialog.ShowDialog() == true)
            {
                this.queueProcessor.ExportCliJson(dialog.FileName);
            }
        }

        public void Export()
        {
            SaveFileDialog dialog = new SaveFileDialog
                                    {
                                        Filter = "Json (*.json)|*.json",
                                        OverwritePrompt = true,
                                        DefaultExt = ".json",
                                        AddExtension = true
                                    };

            if (dialog.ShowDialog() == true)
            {
                this.queueProcessor.ExportJson(dialog.FileName);
            }
        }

        public void Import()
        {
            OpenFileDialog dialog = new OpenFileDialog { Filter = "Json (*.json)|*.json", CheckFileExists = true };
            if (dialog.ShowDialog() == true)
            {
                this.queueProcessor.ImportJson(dialog.FileName);
            }
        }

        /// <summary>
        /// Edit this Job
        /// </summary>
        /// <param name="task">
        /// The task.
        /// </param>
        public void EditJob(QueueTask task)
        {
            MessageBoxResult result = this.errorService.ShowMessageBox(
                Resources.QueueViewModel_EditConfrimation,
                "Modify Job?",
                MessageBoxButton.YesNo,
                MessageBoxImage.Question);

            if (result != MessageBoxResult.Yes)
            {
                return;
            }

            // Remove the job if it is not already encoding. Let the user decide if they want to cancel or not.
            this.RemoveJob(task);

            // Pass a copy of the job back to the Main Screen
            IMainViewModel mvm = IoC.Get<IMainViewModel>();
            mvm.EditQueueJob(task);
        }

        public void OpenSourceDir()
        {
            this.OpenSourceDirectory(this.SelectedTask);
        }

        public void OpenDestDir()
        {
            this.OpenDestinationDirectory(this.SelectedTask);
        }

        public void OpenSourceDirectory(QueueTask task)
        {
            if (task != null)
            {
                this.OpenDirectory(task.ScannedSourcePath);
            }
        }

        public void OpenDestinationDirectory(QueueTask task)
        {
            if (task != null)
            {
                this.OpenDirectory(task.Task.Destination);
            }
        }

        public void ResetSelectedJobs()
        {
            foreach (var task in this.SelectedItems)
            {
                if (task.Status == QueueItemStatus.Completed || task.Status == QueueItemStatus.Error)
                {
                    this.RetryJob(task);
                }
            }       
        }

        public void ResetAllJobs()
        {
            foreach (var task in this.QueueTasks)
            {
                if (task.Status == QueueItemStatus.Completed || task.Status == QueueItemStatus.Error)
                {
                    this.RetryJob(task);
                }
            }
        }
        
        public void ResetFailed()
        {
            foreach (var task in this.QueueTasks)
            {
                if (task.Status == QueueItemStatus.Error)
                {
                    this.RetryJob(task);
                }
            }
        }

        public void PlayFile()
        {
            if (this.SelectedTask != null && this.SelectedTask.Task != null && File.Exists(this.SelectedTask.Task.Destination))
            {
                Process.Start(this.SelectedTask.Task.Destination);
            }
        }

        public void MoveUp()
        {
            Dictionary<int, QueueTask> tasks = new Dictionary<int, QueueTask>();
            foreach (var item in this.SelectedItems)
            {
                tasks.Add(this.QueueTasks.IndexOf(item), item);
            }

            foreach (var item in tasks.OrderBy(s => s.Key))
            {
                this.QueueTasks.MoveUp(item.Value);
            }
        }

        public void MoveDown()
        {
            Dictionary<int, QueueTask> tasks = new Dictionary<int, QueueTask>();
            foreach (var item in this.SelectedItems)
            {
                tasks.Add(this.QueueTasks.IndexOf(item), item);
            }

            foreach (var item in tasks.OrderByDescending(s => s.Key))
            {
                this.QueueTasks.MoveDown(item.Value);
            }
        }

        #endregion

        #region Methods

        public void Activate()
        {
           this.OnActivate();
        }

        public void Deactivate()
        {
           this.OnDeactivate(false);
        }

        /// <summary>
        /// Override the OnActive to run the Screen Loading code in the view model base.
        /// </summary>
        protected override void OnActivate()
        {
            this.Load();

            this.queueProcessor.QueueCompleted += this.queueProcessor_QueueCompleted;
            this.queueProcessor.QueueChanged += this.QueueManager_QueueChanged;
            this.queueProcessor.EncodeService.EncodeStatusChanged += this.EncodeService_EncodeStatusChanged;
            this.queueProcessor.EncodeService.EncodeCompleted += this.EncodeService_EncodeCompleted;
            this.queueProcessor.JobProcessingStarted += this.QueueProcessorJobProcessingStarted;
            this.queueProcessor.QueuePaused += this.QueueProcessor_QueuePaused;

            this.JobsPending = string.Format(Resources.QueueViewModel_JobsPending, this.queueProcessor.Count);

            base.OnActivate();
        }

        /// <summary>
        /// Override the Deactivate
        /// </summary>
        /// <param name="close">
        /// The close.
        /// </param>
        protected override void OnDeactivate(bool close)
        {
            this.queueProcessor.QueueCompleted -= this.queueProcessor_QueueCompleted;
            this.queueProcessor.QueueChanged -= this.QueueManager_QueueChanged;
            this.queueProcessor.EncodeService.EncodeStatusChanged -= this.EncodeService_EncodeStatusChanged;
            this.queueProcessor.EncodeService.EncodeCompleted -= this.EncodeService_EncodeCompleted;
            this.queueProcessor.JobProcessingStarted -= this.QueueProcessorJobProcessingStarted;
            this.queueProcessor.QueuePaused -= this.QueueProcessor_QueuePaused;

            base.OnDeactivate(close);
        }

        private void OpenDirectory(string directory)
        {
            try
            {
                if (!string.IsNullOrEmpty(directory))
                {
                    if (File.Exists(directory))
                    {
                        string argument = "/select, \"" + directory + "\"";
                        Process.Start("explorer.exe", argument);
                        return;
                    }
                    
                    if (!File.Exists(directory) && !directory.EndsWith("\\"))
                    {
                        directory = Path.GetDirectoryName(directory) + "\\";
                    }

                    directory = Path.GetDirectoryName(directory);
                    if (directory != null && Directory.Exists(directory))
                    {
                        Process.Start(directory);
                    }
                }
            }
            catch (Exception exc)
            {
                this.errorService.ShowError(Resources.MainViewModel_UnableToLaunchDestDir, Resources.MainViewModel_UnableToLaunchDestDirSolution, exc);
            }
        }

        /// <summary>
        /// Open the Log file directory
        /// </summary>
        public void OpenLogDirectory()
        {
            string logDir = DirectoryUtilities.GetLogDirectory();
            string windir = Environment.GetEnvironmentVariable("WINDIR");
            Process prc = new Process { StartInfo = { FileName = windir + @"\explorer.exe", Arguments = logDir } };
            prc.Start();
        }

        /// <summary>
        /// Copy the log file to the system clipboard
        /// </summary>
        public void CopyLog()
        {
            try
            {
                Clipboard.SetDataObject(this.ActivityLog, true);
            }
            catch (Exception exc)
            {
                this.errorService.ShowError(Resources.Clipboard_Unavailable, Resources.Clipboard_Unavailable_Solution, exc);
            }
        }

        private void HandleLogData()
        {
            if (this.SelectedTask == null || this.SelectedTask.Status == QueueItemStatus.InProgress || this.SelectedTask.Status == QueueItemStatus.Waiting)
            {
                this.ActivityLog = Resources.QueueView_LogNotAvailableYet;
            }
            else
            {
                try
                {
                    // TODO full log path
                    if (!string.IsNullOrEmpty(this.SelectedTask.Statistics.CompletedActivityLogPath)
                        && File.Exists(this.SelectedTask.Statistics.CompletedActivityLogPath))
                    {
                        using (StreamReader logReader = new StreamReader(this.SelectedTask.Statistics.CompletedActivityLogPath))
                        {
                            string logContent = logReader.ReadToEnd();
                            this.ActivityLog = logContent;
                        }

                    }
                    else
                    {
                        this.ActivityLog = string.Empty;
                    }
                }
                catch (Exception exc)
                {
                    Debug.WriteLine(exc);
                    this.ActivityLog = exc.ToString();
                }
            }

            this.NotifyOfPropertyChange(() => this.ActivityLog);
        }

        /// <summary>
        /// Handle the Encode Status Changed Event.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The EncodeProgressEventArgs.
        /// </param>
        private void EncodeService_EncodeStatusChanged(object sender, EncodeProgressEventArgs e)
        {
            Execute.OnUIThread(() =>
            {
                this.IntermediateProgress = false;

                if (e.IsSubtitleScan)
                {
                    this.JobStatus = string.Format(Resources.MainViewModel_EncodeStatusChanged_SubScan_StatusLabel,
                        e.Task,
                        e.TaskCount,
                        e.PercentComplete,
                        e.EstimatedTimeLeft,
                        e.ElapsedTime,
                        null);

                    this.ProgressValue = e.PercentComplete;
                }
                else if (e.IsMuxing)
                {
                    this.JobStatus = Resources.MainView_Muxing;
                    this.IntermediateProgress = true;
                }
                else if (e.IsSearching)
                {
                    this.JobStatus = string.Format(Resources.MainView_ProgressStatusWithTask, Resources.MainView_Searching, e.PercentComplete, e.EstimatedTimeLeft, null);
                    this.ProgressValue = e.PercentComplete;
                }
                else
                {
                    this.JobStatus =
                        string.Format(Resources.QueueViewModel_EncodeStatusChanged_StatusLabel,
                            e.Task,
                            e.TaskCount,
                            e.PercentComplete,
                            e.CurrentFrameRate,
                            e.AverageFrameRate,
                            e.EstimatedTimeLeft,
                            e.ElapsedTime,
                            null);
                    this.ProgressValue = e.PercentComplete;
                }
            });
        }

        /// <summary>
        /// Handle the Queue Changed Event.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void QueueManager_QueueChanged(object sender, EventArgs e)
        {
            this.JobsPending = string.Format(Resources.QueueViewModel_JobsPending, this.queueProcessor.Count);

            if (!queueProcessor.IsProcessing)
            {
                this.IsQueueRunning = false;
            }

            this.NotifyOfPropertyChange(() => this.CanRetryJob);
            this.NotifyOfPropertyChange(() => this.CanEditJob);
            this.NotifyOfPropertyChange(() => this.CanRemoveJob);
            this.NotifyOfPropertyChange(() => this.CanPerformActionOnSource);
            this.NotifyOfPropertyChange(() => this.CanPlayFile);
            this.NotifyOfPropertyChange(() => this.StatsVisible);
            this.HandleLogData();
        }

        /// <summary>
        /// Handle the Queue Completed Event
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The EventArgs.
        /// </param>
        private void queueProcessor_QueueCompleted(object sender, EventArgs e)
        {
            this.JobsPending = string.Format(Resources.QueueViewModel_JobsPending, this.queueProcessor.Count);
            this.IsQueueRunning = false;
            this.NotifyOfPropertyChange(() => this.SelectedTask);
            this.NotifyOfPropertyChange(() => this.StatsVisible);
            this.NotifyOfPropertyChange(() => this.CanRetryJob);

            this.JobStatus = string.Empty;
        }

        /// <summary>
        /// The encode service_ encode completed.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void EncodeService_EncodeCompleted(object sender, EncodeCompletedEventArgs e)
        {
            if (!this.queueProcessor.IsProcessing)
            {
                this.JobStatus = string.Empty;
            }
        }

        /// <summary>
        /// The queue processor job processing started.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The QueueProgressEventArgs.
        /// </param>
        private void QueueProcessorJobProcessingStarted(object sender, QueueProgressEventArgs e)
        {
            this.JobStatus = Resources.QueueViewModel_QueueStarted;
            this.JobsPending = string.Format(Resources.QueueViewModel_JobsPending, this.queueProcessor.Count);
            this.IsQueueRunning = true;
        }

        private void QueueProcessor_QueuePaused(object sender, EventArgs e)
        {
            this.JobStatus = Resources.QueueViewModel_QueuePaused;
            this.JobsPending = string.Format(Resources.QueueViewModel_JobsPending, this.queueProcessor.Count);
            this.IsQueueRunning = false;
        }

        #endregion
    }
}