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
    using System.Collections.ObjectModel;
    using System.ComponentModel.Composition;
    using System.Windows;

    using Caliburn.Micro;

    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Services.Interfaces;

    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    /// The Preview View Model
    /// </summary>
    [Export(typeof(IQueueViewModel))]
    public class QueueViewModel : ViewModelBase, IQueueViewModel
    {
        #region Private Fields
        /// <summary>
        /// Queue Processor Backing field
        /// </summary>
        private readonly IQueueProcessor queueProcessor;

        /// <summary>
        /// The Error Service Backing field
        /// </summary>
        private readonly IErrorService errorService;

        /// <summary>
        /// Jobs pending backing field
        /// </summary>
        private string jobsPending;

        /// <summary>
        /// Job Status Backing field.
        /// </summary>
        private string jobStatus;

        /// <summary>
        /// IsEncoding Backing field
        /// </summary>
        private bool isEncoding;

        #endregion

        /// <summary>
        /// Initializes a new instance of the <see cref="QueueViewModel"/> class.
        /// </summary>
        /// <param name="windowManager">
        /// The window manager.
        /// </param>
        /// <param name="queueProcessor"> 
        /// The Queue Processor Service 
        /// </param>
        /// <param name="errorService"> 
        /// The Error Service 
        /// </param>
        public QueueViewModel(IWindowManager windowManager, IQueueProcessor queueProcessor, IErrorService errorService)
        {
            this.queueProcessor = queueProcessor;
            this.errorService = errorService;
            this.Title = "Queue";
            this.JobsPending = "No encodes pending";
            this.JobStatus = "There are no jobs currently encoding";
        }

        /// <summary>
        /// Gets QueueJobs.
        /// </summary>
        public ObservableCollection<QueueTask> QueueJobs
        {
            get { return this.queueProcessor.QueueManager.Queue; }
        }

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
                this.NotifyOfPropertyChange("IsEncoding");
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
                this.NotifyOfPropertyChange("JobStatus");
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
                this.NotifyOfPropertyChange("JobsPending");
            }
        }

        /// <summary>
        /// Start Encode
        /// </summary>
        public void StartEncode()
        {
            if (this.queueProcessor.QueueManager.Count == 0)
            {
                this.errorService.ShowMessageBox("There are no pending jobs.", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            this.queueProcessor.Start();
        }

        /// <summary>
        /// Pause Encode
        /// </summary>
        public void PauseEncode()
        {
            this.queueProcessor.Pause();
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
                MessageBoxResult result = this.errorService.ShowMessageBox(
                    "This encode is currently in progress. If you delete it, the encode will be stoped. Are you sure you wish to proceed?",
                    "Warning", MessageBoxButton.YesNo, MessageBoxImage.Question);

                if (result == MessageBoxResult.Yes)
                {
                    this.queueProcessor.QueueManager.Remove(task);
                }
            }
            else
            {
                this.queueProcessor.QueueManager.Remove(task);
            }

            this.JobsPending = string.Format("{0} jobs pending", this.queueProcessor.QueueManager.Count);
        }

        /// <summary>
        /// Handle the On Window Load
        /// </summary>
        public override void OnLoad()
        {
            this.queueProcessor.JobProcessingStarted += queueProcessor_JobProcessingStarted;
            this.queueProcessor.QueueCompleted += queueProcessor_QueueCompleted;
            this.queueProcessor.QueuePaused += queueProcessor_QueuePaused;
            this.queueProcessor.QueueManager.QueueChanged += QueueManager_QueueChanged;

            // Setup the window to the correct state.
            this.IsEncoding = queueProcessor.EncodeService.IsEncoding;
            this.JobsPending = string.Format("{0} jobs pending", this.queueProcessor.QueueManager.Count);

            base.OnLoad();
        }

        /// <summary>
        /// Close this window.
        /// </summary>
        public void Close()
        {
            this.TryClose();
        }

        /// <summary>
        /// Override the OnActive to run the Screen Loading code in the view model base.
        /// </summary>
        protected override void OnActivate()
        {
            this.Load();
            base.OnActivate();
        }

        /// <summary>
        /// Handle the Queue Paused Event
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The EventArgs.
        /// </param>
        private void queueProcessor_QueuePaused(object sender, System.EventArgs e)
        {
            this.JobStatus = "Queue Paused";
            this.JobsPending = string.Format("{0} jobs pending", this.queueProcessor.QueueManager.Count);
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
        private void queueProcessor_QueueCompleted(object sender, System.EventArgs e)
        {
            this.JobStatus = "Queue Completed";
            this.JobsPending = string.Format("{0} jobs pending", this.queueProcessor.QueueManager.Count);
        }

        /// <summary>
        /// Handle teh Job Processing Started Event
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The QueueProgressEventArgs.
        /// </param>
        private void queueProcessor_JobProcessingStarted(object sender, HandBrake.ApplicationServices.EventArgs.QueueProgressEventArgs e)
        {
            this.JobStatus = "Queue Started";
            this.JobsPending = string.Format("{0} jobs pending", this.queueProcessor.QueueManager.Count);
            this.queueProcessor.EncodeService.EncodeStatusChanged += EncodeService_EncodeStatusChanged;
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
        private void EncodeService_EncodeStatusChanged(object sender, HandBrake.ApplicationServices.EventArgs.EncodeProgressEventArgs e)
        {
            this.JobStatus = string.Format(
                "Encoding: Pass {0} of {1},  {2:00.00}%, FPS: {3:000.0},  Avg FPS: {4:000.0},  Time Remaining: {5},  Elapsed: {6:hh\\:mm\\:ss}",
                e.Task,
                e.TaskCount,
                e.PercentComplete,
                e.CurrentFrameRate,
                e.AverageFrameRate,
                e.EstimatedTimeLeft,
                e.ElapsedTime);
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
        private void QueueManager_QueueChanged(object sender, System.EventArgs e)
        {
            // TODO
        }
    }
}
