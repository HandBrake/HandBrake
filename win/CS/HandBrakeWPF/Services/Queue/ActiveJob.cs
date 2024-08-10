// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ActiveJob.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the ActiveJob type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Queue
{
    using System;

    using HandBrakeWPF.Services.Encode.EventArgs;
    using HandBrakeWPF.Services.Encode.Interfaces;
    using HandBrakeWPF.Services.Queue.JobEventArgs;
    using HandBrakeWPF.Services.Queue.Model;

    public class ActiveJob : IDisposable
    {
        private readonly QueueTask job;
        private readonly IEncode encodeService;

        public ActiveJob(QueueTask task, IEncode encodeService)
        {
            this.job = task;
            this.encodeService = encodeService;
        }

        public event EventHandler<ActiveJobCompletedEventArgs> JobFinished;

        public event EventHandler<EncodeProgressEventArgs> JobStatusUpdated;

        public QueueTask Job => this.job;

        public bool IsPaused { get; private set; }

        public bool IsEncoding { get; set; }
        
        public void Start()
        {
            this.IsPaused = false;
            this.IsEncoding = true;

            if (this.encodeService.IsPaused)
            {
                this.encodeService.Resume();
                this.job.Statistics.SetPaused(false);
                this.job.Status = QueueItemStatus.InProgress;
            }
            else if (!this.encodeService.IsEncoding)
            {
                this.job.Status = QueueItemStatus.InProgress;
                this.job.Statistics.UpdateStats(job, DateTime.Now);
            
                this.encodeService.EncodeCompleted += this.EncodeServiceEncodeCompleted;
                this.encodeService.EncodeStatusChanged += this.EncodeStatusChanged;
                
                this.encodeService.Start(this.job.Task, this.job.SelectedPresetKey);
            }
        }

        public void Pause()
        {
            if (this.encodeService.IsEncoding && !this.encodeService.IsPaused)
            {
                this.IsPaused = true;
                this.encodeService.Pause();
                this.job.Statistics.SetPaused(true);
                this.job.Status = QueueItemStatus.Paused;
                this.IsEncoding = false;
            }
        }

        public void Stop()
        {
            this.job.IsShuttingDown = true;
            if (this.encodeService.IsEncoding)
            {
                this.encodeService.Stop();
            }

            this.IsEncoding = false;
            this.IsPaused = false;
            this.encodeService.EncodeStatusChanged -= this.EncodeStatusChanged;
        }

        public void Dispose()
        {
            this.encodeService.EncodeCompleted -= this.EncodeServiceEncodeCompleted;
            this.encodeService.EncodeStatusChanged -= this.EncodeStatusChanged;
        }

        private void EncodeStatusChanged(object sender, EncodeProgressEventArgs e)
        {
            this.job?.JobProgress.Update(e);
            this.OnJobStatusUpdated(e);
        }

        private void EncodeServiceEncodeCompleted(object sender, EncodeCompletedEventArgs e)
        {
            this.job.IsShuttingDown = false;
            this.IsEncoding = false;
            this.IsPaused = false;

            if (e.Successful)
            {
                this.job.Status = QueueItemStatus.Completed;
            }
            else if (e.ErrorCode == 1)
            {
                this.job.Status = QueueItemStatus.Cancelled;
            }
            else
            {
                this.job.Status = QueueItemStatus.Error;
            }

            this.job?.JobProgress.Update(e);
   
            this.job.Statistics.UpdateStats(e, this.job);
            
            this.job.JobProgress.ClearStatusDisplay();
            
            this.encodeService.EncodeStatusChanged -= this.EncodeStatusChanged;
            this.encodeService.EncodeCompleted -= this.EncodeServiceEncodeCompleted;

            this.OnJobFinished(e);
        }

        private void OnJobFinished(EncodeCompletedEventArgs e)
        {
            this.JobFinished?.Invoke(this, new ActiveJobCompletedEventArgs(this, e));
        }

        private void OnJobStatusUpdated(EncodeProgressEventArgs e)
        {
            this.JobStatusUpdated?.Invoke(this, e);
        }
    }
}
