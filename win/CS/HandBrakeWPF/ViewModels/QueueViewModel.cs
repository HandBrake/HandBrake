﻿// --------------------------------------------------------------------------------------------------------------------
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
    using System.Threading;
    using System.Threading.Tasks;
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

    public class QueueViewModel : ViewModelBase, IQueueViewModel
    {
        private readonly IErrorService errorService;
        private readonly IUserSettingService userSettingService;
        private readonly IQueueService queueProcessor;
        private string jobsPending;
        private WhenDone whenDoneAction;
        private QueueTask selectedTask;
        private bool isQueueRunning;

        public QueueViewModel(IUserSettingService userSettingService, IQueueService queueProcessor, IErrorService errorService)
        {
            this.userSettingService = userSettingService;
            this.queueProcessor = queueProcessor;
            this.errorService = errorService;
            this.Title = Resources.QueueViewModel_Queue;
            this.JobsPending = Resources.QueueViewModel_NoEncodesPending;
            this.SelectedItems = new BindingList<QueueTask>();
            this.SelectedItems.ListChanged += this.SelectedItems_ListChanged;
            this.DisplayName = "Queue";
            this.IsQueueRunning = false;
            this.SelectedTabIndex = 0;

            this.WhenDoneAction = (WhenDone)this.userSettingService.GetUserSetting<int>(UserSettingConstants.WhenCompleteAction);
        }

        public bool IsQueueRunning
        {
            get => this.isQueueRunning;

            set
            {
                if (value == this.isQueueRunning)
                {
                    return;
                }

                this.isQueueRunning = value;
                this.NotifyOfPropertyChange(() => this.IsQueueRunning);
            }
        }

        public string JobsPending
        {
            get => this.jobsPending;

            set
            {
                this.jobsPending = value;
                this.NotifyOfPropertyChange(() => this.JobsPending);
            }
        }

        public WhenDone WhenDoneAction
        {
            get => this.whenDoneAction;

            set
            {
                this.whenDoneAction = value;
                this.NotifyOfPropertyChange(() => this.WhenDoneAction);
            }
        }

        public ObservableCollection<QueueTask> QueueTasks => this.queueProcessor.Queue;

        public BindingList<QueueTask> SelectedItems { get; }

        public QueueTask SelectedTask
        {
            get => this.selectedTask;

            set
            {
                if (Equals(value, this.selectedTask))
                {
                    return;
                }

                this.selectedTask = value;
                this.NotifyOfPropertyChange(() => this.SelectedTask);
                this.HandleLogData();

                this.NotifyOfPropertyChange(() => this.CanRetryJob);
                this.NotifyOfPropertyChange(() => this.CanEditJob);
                this.NotifyOfPropertyChange(() => this.CanRemoveJob);
                this.NotifyOfPropertyChange(() => this.CanPerformActionOnSource);
                this.NotifyOfPropertyChange(() => this.CanPlayFile);
                this.NotifyOfPropertyChange(() => this.StatsVisible);
                this.NotifyOfPropertyChange(() => this.JobInfoVisible);
            }
        }

        public bool JobInfoVisible => SelectedItems.Count == 1;

        public int SelectedTabIndex { get; set; }

        public string ActivityLog { get; private set; }

        public bool CanRetryJob => this.SelectedTask != null && this.SelectedTask.Status != QueueItemStatus.Waiting && this.SelectedTask.Status != QueueItemStatus.InProgress;

        public bool CanEditJob => this.SelectedTask != null;

        public bool CanRemoveJob => this.SelectedTask != null;

        public bool CanPerformActionOnSource => this.SelectedTask != null;

        public bool CanPlayFile =>
            this.SelectedTask != null && this.SelectedTask.Task.Destination != null && 
            this.SelectedTask.Status == QueueItemStatus.Completed && File.Exists(this.SelectedTask.Task.Destination);

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

        public void WhenDone(int action)
        {
            this.WhenDone(action, true);
        }

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

        public void Clear()
        {
            MessageBoxResult result = this.errorService.ShowMessageBox(
                Resources.QueueViewModel_ClearQueueConfirmation, Resources.Confirm, MessageBoxButton.YesNo, MessageBoxImage.Question);
            if (result == MessageBoxResult.Yes)
            {
                this.queueProcessor.Clear();
            }
        }

        public void ClearCompleted()
        {
            this.queueProcessor.ClearCompleted();
        }

        public void Close()
        {
            this.TryCloseAsync();
        }

        public override void OnLoad()
        {
            // Setup the window to the correct state.
            this.IsQueueRunning = this.queueProcessor.IsProcessing;
            this.JobsPending = string.Format(Resources.QueueViewModel_JobsPending, this.queueProcessor.Count);

            base.OnLoad();
        }

        public bool CanPauseQueue()
        {
            return this.IsQueueRunning;
        }

        public void PauseQueue()
        {
            this.queueProcessor.Pause(true);

            this.JobsPending = string.Format(Resources.QueueViewModel_JobsPending, this.queueProcessor.Count);
            this.IsQueueRunning = false;
        }

        public void PauseQueueToolbar()
        {
            this.PauseQueue();
        }

        public void StopQueue()
        {
            if (this.queueProcessor.IsEncoding)
            {
                MessageBoxResult result = this.errorService.ShowMessageBox(
                    "There are currently jobs running. Would you like to complete the current jobs before stopping the queue?",
                    "Confirm",
                    MessageBoxButton.YesNoCancel,
                    MessageBoxImage.Question);

                if (result == MessageBoxResult.Yes)
                {
                    this.queueProcessor.Stop(false);
                }
                else if (result == MessageBoxResult.Cancel)
                {
                    return;
                }
                else
                {
                    this.queueProcessor.Stop(true);
                }
            }
            else
            {
                this.queueProcessor.Stop(true);
            }

            this.JobsPending = string.Format(Resources.QueueViewModel_JobsPending, this.queueProcessor.Count);
            this.IsQueueRunning = false;
        }

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

            List<QueueTask> tasksToRemove = this.SelectedItems.OrderBy(s => s.Status).ToList();
            foreach (QueueTask job in tasksToRemove)
            {
                this.RemoveJob(job);
            }
        }

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

        public void RetryJob(QueueTask task)
        {
            this.queueProcessor.RetryJob(task);
            this.JobsPending = string.Format(Resources.QueueViewModel_JobsPending, this.queueProcessor.Count);
            this.NotifyOfPropertyChange(() => this.CanRetryJob);
        }

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
                MessageBoxResult result = this.errorService.ShowMessageBox(Resources.Main_LowDiskspace, Resources.Warning, MessageBoxButton.YesNo, MessageBoxImage.Warning);
                if (result == MessageBoxResult.No)
                {
                    return;
                }
            }

            this.JobsPending = string.Format(Resources.QueueViewModel_JobsPending, this.queueProcessor.Count);
            this.IsQueueRunning = true;

            this.queueProcessor.Start();
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

        public void EditJob(QueueTask task)
        {
            MessageBoxResult result = this.errorService.ShowMessageBox(
                Resources.QueueViewModel_EditConfirmation,
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

        public void Activate()
        {
           this.OnActivateAsync(CancellationToken.None);
        }

        public void Deactivate()
        {
           this.OnDeactivateAsync(false, CancellationToken.None);
        }

        protected override Task OnActivateAsync(CancellationToken cancellationToken)
        {
            this.Load();

            this.queueProcessor.QueueCompleted += this.QueueProcessor_QueueCompleted;
            this.queueProcessor.QueueChanged += this.QueueManager_QueueChanged;
            this.queueProcessor.JobProcessingStarted += this.QueueProcessorJobProcessingStarted;
            this.queueProcessor.QueuePaused += this.QueueProcessor_QueuePaused;

            this.IsQueueRunning = this.queueProcessor.IsProcessing;
            this.JobsPending = string.Format(Resources.QueueViewModel_JobsPending, this.queueProcessor.Count);
            
            return base.OnActivateAsync(cancellationToken);
        }

        protected override Task OnDeactivateAsync(bool close, CancellationToken cancellationToken)
        {
            this.queueProcessor.QueueCompleted -= this.QueueProcessor_QueueCompleted;
            this.queueProcessor.QueueChanged -= this.QueueManager_QueueChanged;
            this.queueProcessor.JobProcessingStarted -= this.QueueProcessorJobProcessingStarted;
            this.queueProcessor.QueuePaused -= this.QueueProcessor_QueuePaused;

            return base.OnDeactivateAsync(close, cancellationToken);
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
                        Process.Start("explorer.exe", directory);
                    }
                }
            }
            catch (Exception exc)
            {
                this.errorService.ShowError(Resources.MainViewModel_UnableToLaunchDestDir, Resources.MainViewModel_UnableToLaunchDestDirSolution, exc);
            }
        }

        public void OpenLogDirectory()
        {
            string logDir = DirectoryUtilities.GetLogDirectory();
            string windir = Environment.GetEnvironmentVariable("WINDIR");
            Process prc = new Process { StartInfo = { FileName = windir + @"\explorer.exe", Arguments = logDir } };
            prc.Start();
        }

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

        private void SelectedItems_ListChanged(object sender, ListChangedEventArgs e)
        {
            this.NotifyOfPropertyChange(() => this.JobInfoVisible);

            if (!this.JobInfoVisible)
            {
                this.SelectedTabIndex = 0;
                this.NotifyOfPropertyChange(() => this.SelectedTabIndex);
            }
        }
        
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
            this.NotifyOfPropertyChange(() => this.JobInfoVisible);
            this.HandleLogData();
        }

        private void QueueProcessor_QueueCompleted(object sender, EventArgs e)
        {
            this.JobsPending = string.Format(Resources.QueueViewModel_JobsPending, this.queueProcessor.Count);
            this.IsQueueRunning = false;
            this.NotifyOfPropertyChange(() => this.SelectedTask);
            this.NotifyOfPropertyChange(() => this.StatsVisible);
            this.NotifyOfPropertyChange(() => this.CanRetryJob);
            this.NotifyOfPropertyChange(() => this.JobInfoVisible);
        }

        private void QueueProcessorJobProcessingStarted(object sender, QueueProgressEventArgs e)
        {
            this.JobsPending = string.Format(Resources.QueueViewModel_JobsPending, this.queueProcessor.Count);
            this.IsQueueRunning = true;
        }

        private void QueueProcessor_QueuePaused(object sender, EventArgs e)
        {
            this.JobsPending = string.Format(Resources.QueueViewModel_JobsPending, this.queueProcessor.Count);
            this.IsQueueRunning = false;
        }
    }
}