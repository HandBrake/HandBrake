// --------------------------------------------------------------------------------------------------------------------
// <copyright file="QueueStats.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A file to record stats about a queue task.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Services.Queue.Model
{
    using System;

    using Caliburn.Micro;

    public class QueueStats : PropertyChangedBase
    {
        private DateTime startTime;
        private DateTime endTime;
        private long? finalFileSize;
        private DateTime pausedStartPoint;
        private TimeSpan pausedTimespan;

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
                if (value.Equals(this.startTime)) return;
                this.startTime = value;
                this.NotifyOfPropertyChange(() => this.StartTime);
                this.NotifyOfPropertyChange(() => this.Duration);
            }
        }

        public DateTime EndTime
        {
            get
            {
                return this.endTime;
            }
            set
            {
                if (value.Equals(this.endTime)) return;
                this.endTime = value;
                this.NotifyOfPropertyChange(() => this.EndTime);
                this.NotifyOfPropertyChange(() => this.Duration);
            }
        }

        public TimeSpan PausedDuration
        {
            get
            {
                return this.pausedTimespan;
            }
        }

        public TimeSpan Duration
        {
            get
            {
                return this.EndTime - this.StartTime - this.PausedDuration;
            }
        }

        /// <summary>
        /// Final filesize in Bytes
        /// </summary>
        public long? FinalFileSize
        {
            get
            {
                return this.finalFileSize;
            }
            set
            {
                if (value == this.finalFileSize) return;
                this.finalFileSize = value;
                this.NotifyOfPropertyChange(() => this.FinalFileSize);
                this.NotifyOfPropertyChange(() => this.FinalFileSizeInMegaBytes);
            }
        }

        public long? FinalFileSizeInMegaBytes
        {
            get
            {
                if (this.finalFileSize.HasValue)
                {
                    return this.finalFileSize / 1024 / 1024;
                }

                return 0;
            }
        }

        public string CompletedActivityLogPath { get; set; }

        public void SetPaused(bool isPaused)
        {
            if (isPaused)
            {
                this.pausedStartPoint = DateTime.Now;
            }
            else
            {
                TimeSpan pausedDuration = DateTime.Now - this.pausedStartPoint;
                this.pausedTimespan = this.PausedDuration.Add(pausedDuration);
            }
        }
    }
}
