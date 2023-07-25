// --------------------------------------------------------------------------------------------------------------------
// <copyright file="QueueStats.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A file to record stats about a queue task.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Queue.Model
{
    using System;
    using System.IO;

    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Encode.EventArgs;
    using HandBrakeWPF.Services.Encode.Model.Models;
    using HandBrakeWPF.ViewModels;

    public class QueueStats : PropertyChangedBase
    {
        private DateTime startTime;
        private DateTime endTime;
        private long? finalFileSize;
        private DateTime pausedStartPoint;
        private TimeSpan pausedTimespan;

        private bool isPaused;

        public QueueStats()
        {
            this.pausedTimespan = new TimeSpan();
        }

        public DateTime StartTime
        {
            get
            {
                return this.startTime;
            }

            set
            {
                if (value.Equals(this.startTime))
                {
                    return;
                }

                this.startTime = value;
                this.NotifyOfPropertyChange(() => this.StartTime);
                this.NotifyOfPropertyChange(() => this.Duration);
                this.NotifyOfPropertyChange(() => this.StartTimeDisplay);
                this.NotifyOfPropertyChange(() => this.DurationDisplay);
            }
        }

        public string StartTimeDisplay
        {
            get
            {
                if (this.startTime == DateTime.MinValue)
                {
                    return Resources.QueueView_NotAvailable;
                }

                return this.startTime.ToString();
            }
        }

        public DateTime EndTime
        {
            get => this.endTime;

            set
            {
                if (value.Equals(this.endTime))
                {
                    return;
                }

                this.endTime = value;
                this.NotifyOfPropertyChange(() => this.EndTime);
                this.NotifyOfPropertyChange(() => this.Duration);
                this.NotifyOfPropertyChange(() => this.EndTimeDisplay);
                this.NotifyOfPropertyChange(() => this.DurationDisplay);
            }
        }

        public string EndTimeDisplay
        {
            get
            {
                if (this.endTime == DateTime.MinValue)
                {
                    return Resources.QueueView_NotAvailable;
                }

                return this.endTime.ToString();
            }
        }

        public TimeSpan PausedDuration
        {
            get
            {
                return this.pausedTimespan;
            }
        }

        public string PausedDisplay
        {
            get
            {
                if (this.isPaused)
                {
                    return Resources.QueueView_CurrentlyPaused;
                }

                if (this.PausedDuration == TimeSpan.Zero)
                {
                    return Resources.QueueView_NotAvailable;
                }

                return this.PausedDuration.Days >= 1 ? string.Format(@"{0:d\:hh\:mm\:ss}", this.PausedDuration) : string.Format(@"{0:hh\:mm\:ss}", this.PausedDuration);
            }
        }

        public TimeSpan Duration
        {
            get
            {
                if (this.endTime == DateTime.MinValue)
                {
                    return TimeSpan.Zero;
                }

                return this.EndTime - this.StartTime - this.PausedDuration;
            }
        }

        public string DurationDisplay
        {
            get
            {
                if (this.Duration == TimeSpan.Zero)
                {
                    return Resources.QueueView_NotAvailable;
                }

                return this.Duration.Days >= 1 ? string.Format(@"{0:d\:hh\:mm\:ss}", this.Duration) : string.Format(@"{0:hh\:mm\:ss}", this.Duration);
            }
        }

        public long? SourceFileSizeInBytes
        {
            get;
            set;
        }

        public long? FinalFileSizeBytes
        {
            get
            {
                return this.finalFileSize;
            }

            set
            {
                if (value == this.finalFileSize)
                {
                    return;
                }

                this.finalFileSize = value;
                this.NotifyOfPropertyChange(() => this.FinalFileSizeBytes);
                this.NotifyOfPropertyChange(() => this.FinalFileSizeInMegaBytes);
                this.NotifyOfPropertyChange(() => this.FileSizeDisplay);
            }
        }

        public decimal? FinalFileSizeInMegaBytes
        {
            get
            {
                if (this.finalFileSize.HasValue)
                {
                    return (decimal)this.finalFileSize / 1024 / 1024;
                }

                return 0;
            }
        }

        public string FileSizeDisplay
        {
            get
            {
                if (!FinalFileSizeInMegaBytes.HasValue || FinalFileSizeInMegaBytes == 0)
                {
                    return Resources.QueueView_NotAvailable;
                }

                // Work out the size difference
                string percentage = string.Empty;
                if (SourceFileSizeInBytes != null && SourceFileSizeInBytes != 0 && FinalFileSizeBytes.HasValue)
                {
                    decimal difference = (decimal) 100 / SourceFileSizeInBytes.Value * FinalFileSizeBytes.Value;
                    percentage = string.Format(" ({0} %{1})", Math.Round(difference, 3), Resources.QueueViewModel_DifferenceText);
                }

                return string.Format("{0:######.###} MB{1}", FinalFileSizeInMegaBytes, percentage);
            }
        }

        public string CompletedActivityLogPath { get; set; }

        public double EncodingSpeed { get; set; }

        public string EncodingSpeedDisplay
        {
            get
            {
                if (EncodingSpeed != 0)
                {
                    return string.Format("{0} fps", Math.Round(EncodingSpeed, 2));
                }
                else
                {
                    return Resources.QueueView_NotAvailable;
                }

                return string.Empty;
            }
        }

        public string ContentLength { get; set; }

        public string SourceLength { get; set; }

        public string SummaryCompleteStats { get; set; }
        
        public void SetPaused(bool isPaused)
        {
            this.isPaused = isPaused;
            if (isPaused)
            {
                this.pausedStartPoint = DateTime.Now;
            }
            else
            {
                TimeSpan pausedDuration = DateTime.Now - this.pausedStartPoint;
                this.pausedTimespan = this.PausedDuration.Add(pausedDuration);
            }

            this.NotifyOfPropertyChange(() => this.PausedDisplay);
        }

        public void UpdateStats(QueueTask job, DateTime? startTime)
        {
            if (File.Exists(job.Task.Source))
            {
                FileInfo file = new FileInfo(job.Task.Source);
                this.SourceFileSizeInBytes = file.Length;
            }

            this.ContentLength = this.DurationCalculation(job);
            this.SourceLength = GetSourceDuration(job);

            if (startTime != null)
            {
                this.StartTime = DateTime.Now;
            }
            
            this.NotifyOfPropertyChange(() => this.SourceFileSizeInBytes);
            this.NotifyOfPropertyChange(() => this.ContentLength);
            this.NotifyOfPropertyChange(() => this.StartTime);
            this.NotifyOfPropertyChange(() => this.SourceLength);
        }

        public void UpdateStats(EncodeCompletedEventArgs e, QueueTask job)
        {
            EndTime = DateTime.Now;
            CompletedActivityLogPath = e.ActivityLogPath;
            FinalFileSizeBytes = e.FinalFilesizeInBytes;

            if (File.Exists(job.Task.Source))
            {
                FileInfo file = new FileInfo(job.Task.Source);
                SourceFileSizeInBytes = file.Length;
            }

            EncodingSpeed = job.JobProgress.AverageFrameRate;
            ContentLength = this.DurationCalculation(job);
            SourceLength = GetSourceDuration(job);

            this.SummaryCompleteStats = string.Format(Resources.QueueStats_ShortOutputStats, FileSizeDisplay, Math.Round(EncodingSpeed, 0));

            this.NotifyOfPropertyChange(() => this.EndTime);
            this.NotifyOfPropertyChange(() => this.CompletedActivityLogPath);
            this.NotifyOfPropertyChange(() => this.FileSizeDisplay);
            this.NotifyOfPropertyChange(() => this.EncodingSpeedDisplay);
            this.NotifyOfPropertyChange(() => this.ContentLength);
            this.NotifyOfPropertyChange(() => this.SourceLength);
            this.NotifyOfPropertyChange(() => this.SummaryCompleteStats);
        }

        public void Reset()
        {
            this.isPaused = false;
            this.pausedTimespan = TimeSpan.Zero;
            this.pausedStartPoint = DateTime.MinValue;
            this.StartTime = DateTime.MinValue;
            this.EndTime = DateTime.MinValue;
            this.FinalFileSizeBytes = 0;
            this.CompletedActivityLogPath = null;
            this.EncodingSpeed = 0;
            this.SourceFileSizeInBytes = 0;
            this.ContentLength = string.Empty;
            this.SourceLength = string.Empty;

            this.NotifyOfPropertyChange(() => this.PausedDuration);
            this.NotifyOfPropertyChange(() => this.PausedDisplay);
            this.NotifyOfPropertyChange(() => this.EncodingSpeedDisplay);
            this.NotifyOfPropertyChange(() => this.ContentLength);
            this.NotifyOfPropertyChange(() => this.SourceLength);
        }

        private string GetSourceDuration(QueueTask job)
        {
            if (job.SourceTitleInfo == null)
            {
                return Resources.QueueViewModel_NotAvailable;
            }

            TimeSpan duration = job.SourceTitleInfo.Duration;

            return string.Format("{0:00}:{1:00}:{2:00}", duration.Hours, duration.Minutes, duration.Seconds);
        }

        private string DurationCalculation(QueueTask job)
        {
            if (job.SourceTitleInfo == null)
            {
                return Resources.QueueViewModel_NotAvailable;
            }

            double startEndDuration = job.Task.EndPoint - job.Task.StartPoint;
            TimeSpan output;

            switch (job.Task.PointToPointMode)
            {
                case PointToPointMode.Chapters:
                    output = job.SourceTitleInfo.CalculateDuration(job.Task.StartPoint, job.Task.EndPoint);
                    return string.Format("{0:00}:{1:00}:{2:00}", output.Hours, output.Minutes, output.Seconds);
                case PointToPointMode.Seconds:
                    output = TimeSpan.FromSeconds(startEndDuration);
                    return string.Format("{0:00}:{1:00}:{2:00}", output.Hours, output.Minutes, output.Seconds);
                case PointToPointMode.Frames:
                    startEndDuration = startEndDuration / job.SourceTitleInfo.Fps;
                    output = TimeSpan.FromSeconds(Math.Round(startEndDuration, 2));
                    return string.Format("{0:00}:{1:00}:{2:00}", output.Hours, output.Minutes, output.Seconds);
            }

            return Resources.QueueViewModel_NotAvailable;
        }
    }
}
