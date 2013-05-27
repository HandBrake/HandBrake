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
    using System.ComponentModel;
    using System.Windows;

    using Caliburn.Micro;

    using HandBrake.ApplicationServices.EventArgs;
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Services.Interfaces;

    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.ViewModels.Interfaces;

    using Ookii.Dialogs.Wpf;

    /// <summary>
    /// The Preview View Model
    /// </summary>
    public class QueueViewModel : ViewModelBase, IQueueViewModel
    {
        #region Constants and Fields

        /// <summary>
        /// The Error Service Backing field
        /// </summary>
        private readonly IErrorService errorService;

        /// <summary>
        /// The User Setting Service Backing Field.
        /// </summary>
        private readonly IUserSettingService userSettingService;

        /// <summary>
        /// Queue Processor Backing field
        /// </summary>
        private readonly IQueueProcessor queueProcessor;

        /// <summary>
        /// IsEncoding Backing field
        /// </summary>
        private bool isEncoding;

        /// <summary>
        /// Job Status Backing field.
        /// </summary>
        private string jobStatus;

        /// <summary>
        /// Jobs pending backing field
        /// </summary>
        private string jobsPending;

        /// <summary>
        /// Backing field for the when done action description
        /// </summary>
        private string whenDoneAction;

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
        public QueueViewModel(IUserSettingService userSettingService, IQueueProcessor queueProcessor, IErrorService errorService)
        {
            this.userSettingService = userSettingService;
            this.queueProcessor = queueProcessor;
            this.errorService = errorService;
            this.Title = "Queue";
            this.JobsPending = "No encodes pending";
            this.JobStatus = "There are no jobs currently encoding";
        }

        #endregion

        #region Properties

        /// <summary>
        /// Gets or sets a value indicating whether IsEncoding.
        /// </summary>
        public bool IsEncoding
        {
            get
            {
                return this.isEncoding;
            }

            set
            {
                this.isEncoding = value;
                this.NotifyOfPropertyChange(() => IsEncoding);
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
            }
        }

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
        public string WhenDoneAction
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
        public BindingList<QueueTask> QueueTasks
        {
            get
            {
                return this.queueProcessor.Queue;
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
        public void WhenDone(string action)
        {
            this.WhenDoneAction = action;
            this.userSettingService.SetUserSetting(UserSettingConstants.WhenCompleteAction, action);
        }

        /// <summary>
        /// Clear the Queue
        /// </summary>
        public void Clear()
        {
            MessageBoxResult result = this.errorService.ShowMessageBox(
                "Are you sure you wish to clear the queue?", "Confirm", MessageBoxButton.YesNo, MessageBoxImage.Warning);
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
            this.IsEncoding = this.queueProcessor.EncodeService.IsEncoding;
            this.JobsPending = string.Format("{0} jobs pending", this.queueProcessor.Count);

            base.OnLoad();
        }

        /// <summary>
        /// Pause Encode
        /// </summary>
        public void PauseEncode()
        {
            this.queueProcessor.Pause();

            this.JobStatus = "Queue Paused";
            this.JobsPending = string.Format("{0} jobs pending", this.queueProcessor.Count);
            this.IsEncoding = false;

            MessageBox.Show("The Queue has been pasued. The currently running job will run to completion and no further jobs will start.", "Queue",
                MessageBoxButton.OK, MessageBoxImage.Information);
        }

        /// <summary>
        /// Remove a Job from the queue
        /// </summary>
        /// <param name="task">
        /// The Job to remove from the queue
        /// </param>
        public void RemoveJob(QueueTask task)
        {
            if (task.Status == QueueItemStatus.InProgress)
            {
                MessageBoxResult result =
                    this.errorService.ShowMessageBox(
                        "This encode is currently in progress. If you delete it, the encode will be stopped. Are you sure you wish to proceed?",
                        "Warning",
                        MessageBoxButton.YesNo,
                        MessageBoxImage.Question);

                if (result == MessageBoxResult.Yes)
                {
                    this.queueProcessor.EncodeService.Stop();
                    this.queueProcessor.Remove(task);
                }
            }
            else
            {
                this.queueProcessor.Remove(task);
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
            task.Status = QueueItemStatus.Waiting;
            this.queueProcessor.BackupQueue(null);
            this.JobsPending = string.Format("{0} jobs pending", this.queueProcessor.Count);
        }

        /// <summary>
        /// Start Encode
        /// </summary>
        public void StartEncode()
        {
            if (this.queueProcessor.Count == 0)
            {
                this.errorService.ShowMessageBox(
                    "There are no pending jobs.", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            this.JobStatus = "Queue Started";
            this.JobsPending = string.Format("{0} jobs pending", this.queueProcessor.Count);
            this.IsEncoding = true;

            this.queueProcessor.Start();
        }

        /// <summary>
        /// Export the Queue to a file.
        /// </summary>
        public void Export()
        {
            VistaSaveFileDialog dialog = new VistaSaveFileDialog
                {
                    Filter = "HandBrake Queue Files (*.hbq)|*.hbq",
                    OverwritePrompt = true,
                    DefaultExt = ".hbq",
                    AddExtension = true
                };
            dialog.ShowDialog();

            this.queueProcessor.BackupQueue(dialog.FileName);
        }

        /// <summary>
        /// Import a saved queue
        /// </summary>
        public void Import()
        {
            VistaOpenFileDialog dialog = new VistaOpenFileDialog { Filter = "HandBrake Queue Files (*.hbq)|*.hbq", CheckFileExists = true };
            dialog.ShowDialog();

            this.queueProcessor.RestoreQueue(dialog.FileName);
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
                "Are you sure you wish to edit this job? It will be removed from the queue and sent to the main window.",
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
            mvm.EditQueueJob(new EncodeTask(task.Task));
        }

        #endregion

        #region Methods

        /// <summary>
        /// Override the OnActive to run the Screen Loading code in the view model base.
        /// </summary>
        protected override void OnActivate()
        {
            this.Load();

            this.WhenDoneAction = this.userSettingService.GetUserSetting<string>(UserSettingConstants.WhenCompleteAction);

            this.queueProcessor.QueueCompleted += this.queueProcessor_QueueCompleted;
            this.queueProcessor.QueueChanged += this.QueueManager_QueueChanged;
            this.queueProcessor.EncodeService.EncodeStatusChanged += this.EncodeService_EncodeStatusChanged;
            this.queueProcessor.EncodeService.EncodeCompleted += EncodeService_EncodeCompleted;
            this.queueProcessor.JobProcessingStarted += this.QueueProcessorJobProcessingStarted;

            this.JobsPending = string.Format("{0} jobs pending", this.queueProcessor.Count);
            this.JobStatus = "Queue Ready";

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
            this.queueProcessor.EncodeService.EncodeCompleted -= EncodeService_EncodeCompleted;
            this.queueProcessor.JobProcessingStarted -= this.QueueProcessorJobProcessingStarted;

            base.OnDeactivate(close);
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
        private void EncodeService_EncodeStatusChanged(
            object sender, EncodeProgressEventArgs e)
        {
            Caliburn.Micro.Execute.OnUIThread(() =>
            {


                this.JobStatus =
                    string.Format(
                        "Encoding: Pass {0} of {1},  {2:00.00}%, FPS: {3:000.0},  Avg FPS: {4:000.0},  Time Remaining: {5},  Elapsed: {6:hh\\:mm\\:ss}",
                        e.Task,
                        e.TaskCount,
                        e.PercentComplete,
                        e.CurrentFrameRate,
                        e.AverageFrameRate,
                        e.EstimatedTimeLeft,
                        e.ElapsedTime);
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
            this.JobsPending = string.Format("{0} jobs pending", this.queueProcessor.Count);

            if (!queueProcessor.IsProcessing)
            {
                this.JobStatus = "Queue Not Running";
            }
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
            this.JobStatus = "Queue Completed";
            this.JobsPending = string.Format("{0} jobs pending", this.queueProcessor.Count);
            this.IsEncoding = false;
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
                this.JobStatus = "Last Queued Job Finished";
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
            this.JobStatus = "Queue Started";
            this.JobsPending = string.Format("{0} jobs pending", this.queueProcessor.Count);
            this.IsEncoding = true;
        }

        #endregion
    }
}