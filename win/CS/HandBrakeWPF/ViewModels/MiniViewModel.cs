// --------------------------------------------------------------------------------------------------------------------
// <copyright file="MiniViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The mini view model.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels
{
    using System;

    using HandBrakeWPF.EventArgs;
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Queue.Interfaces;
    using HandBrakeWPF.ViewModels.Interfaces;

    using EncodeProgressEventArgs = HandBrakeWPF.Services.Encode.EventArgs.EncodeProgressEventArgs;
    using IEncode = HandBrakeWPF.Services.Encode.Interfaces.IEncode;

    /// <summary>
    /// The mini view model.
    /// </summary>
    public class MiniViewModel : ViewModelBase, IMiniViewModel
    {
        private readonly IEncode encodeService;
        private readonly IQueueService queueProcessor;
        private string queueStatus;
        private string progress;
        private string task;
        private string windowTitle;

        /// <summary>
        /// Initializes a new instance of the <see cref="MiniViewModel"/> class.
        /// </summary>
        /// <param name="encodeService">
        /// The encode Service.
        /// </param>
        /// <param name="queueProcessor">
        /// The queue Processor.
        /// </param>
        public MiniViewModel(IEncode encodeService, IQueueService queueProcessor)
        {
            this.encodeService = encodeService;
            this.queueProcessor = queueProcessor;

            this.Task = "Ready";
            this.Progress = string.Empty;
            this.QueueStatus = string.Format("{0} jobs pending", this.queueProcessor.Count);
        }

        /// <summary>
        /// Gets or sets the task.
        /// </summary>
        public string Task
        {
            get
            {
                return this.task;
            }
            set
            {
                if (value == this.task)
                {
                    return;
                }
                this.task = value;
                this.NotifyOfPropertyChange(() => this.Task);
            }
        }

        /// <summary>
        /// Gets or sets the progress.
        /// </summary>
        public string Progress
        {
            get
            {
                return this.progress;
            }
            set
            {
                if (value == this.progress)
                {
                    return;
                }
                this.progress = value;
                this.NotifyOfPropertyChange(() => this.Progress);
            }
        }

        /// <summary>
        /// Gets or sets the queue status.
        /// </summary>
        public string QueueStatus
        {
            get
            {
                return this.queueStatus;
            }
            set
            {
                if (value == this.queueStatus)
                {
                    return;
                }
                this.queueStatus = value;
                this.NotifyOfPropertyChange(() => this.QueueStatus);
            }
        }

        /// <summary>
        /// Gets or sets the window title.
        /// </summary>
        public string WindowTitle
        {
            get
            {
                return this.windowTitle;
            }
            set
            {
                if (value == this.windowTitle)
                {
                    return;
                }
                this.windowTitle = value;
                this.NotifyOfPropertyChange(() => this.WindowTitle);
            }
        }

        /// <summary>
        /// The activate.
        /// </summary>
        public void Activate()
        {
            this.encodeService.EncodeStatusChanged += EncodeService_EncodeStatusChanged;
            this.queueProcessor.QueueChanged += QueueProcessor_QueueChanged;
            this.queueProcessor.QueueCompleted += QueueProcessor_QueueCompleted;
            this.WindowTitle = "Mini Status Display";
        }

        /// <summary>
        /// The tear down.
        /// </summary>
        public void Close()
        {
            this.encodeService.EncodeStatusChanged -= EncodeService_EncodeStatusChanged;
            this.queueProcessor.QueueChanged -= QueueProcessor_QueueChanged;
            this.queueProcessor.QueueCompleted -= QueueProcessor_QueueCompleted;
            this.TryClose();
        }

        /// <summary>
        /// The queue processor_ queue completed.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void QueueProcessor_QueueCompleted(object sender, QueueCompletedEventArgs e)
        {
            this.Task = "Not Encoding.";
            this.Progress = string.Empty;
            this.QueueStatus = string.Format("{0} jobs pending", this.queueProcessor.Count);
        }

        /// <summary>
        /// The queue processor_ queue changed.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void QueueProcessor_QueueChanged(object sender, EventArgs e)
        {
            this.QueueStatus = string.Format("{0} jobs pending", this.queueProcessor.Count);
        }

        /// <summary>
        /// The encode service_ encode status changed.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void EncodeService_EncodeStatusChanged(object sender, EncodeProgressEventArgs e)
        {
            this.Task = this.queueProcessor.LastProcessedJob.ScannedSourcePath;

            this.Progress =
                string.Format(Resources.MiniViewModel_EncodeStatusChanged_StatusLabel,
                    e.Task,
                    e.TaskCount,
                    e.PercentComplete,
                    e.CurrentFrameRate,
                    e.AverageFrameRate,
                    e.EstimatedTimeLeft,
                    e.ElapsedTime);
        }
    }
}
