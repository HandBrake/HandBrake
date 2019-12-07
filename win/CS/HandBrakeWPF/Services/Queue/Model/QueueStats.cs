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
    using System.Runtime.CompilerServices;

    using Caliburn.Micro;

    using HandBrakeWPF.Properties;

    using Newtonsoft.Json;

    [JsonObject(MemberSerialization.OptOut)]
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
            get
            {
                return this.endTime;
            }

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
                    return string.Empty;
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
                    return string.Empty;
                }

                return PausedDuration.ToString("hh\\:mm\\:ss");
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
                    return string.Empty;
                }

                return this.Duration.ToString("hh\\:mm\\:ss");
            }
        }

        public long? FinalFileSize
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
                this.NotifyOfPropertyChange(() => this.FinalFileSize);
                this.NotifyOfPropertyChange(() => this.FinalFileSizeInMegaBytes);
                this.NotifyOfPropertyChange(() => this.FileSizeDisplay);
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

        public string FileSizeDisplay
        {
            get
            {
                if (FinalFileSizeInMegaBytes == 0)
                {
                    return string.Empty;
                }

                return string.Format("{0:##.###} MB", FinalFileSizeInMegaBytes);
            }
        }

        public string CompletedActivityLogPath { get; set; }

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

        public void Reset()
        {
            this.isPaused = false;
            this.pausedTimespan = TimeSpan.Zero;
            this.pausedStartPoint = DateTime.MinValue;
            this.StartTime = DateTime.MinValue;
            this.EndTime = DateTime.MinValue;
            this.FinalFileSize = 0;
            this.CompletedActivityLogPath = null;

            this.NotifyOfPropertyChange(() => this.PausedDuration);
            this.NotifyOfPropertyChange(() => this.PausedDisplay);
        }
    }
}
