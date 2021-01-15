// --------------------------------------------------------------------------------------------------------------------
// <copyright file="QueueProgressStatus.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the QueueProgressStatus type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Queue.Model
{
    using System;

    using Caliburn.Micro;

    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Encode.EventArgs;

    public class QueueProgressStatus : PropertyChangedBase
    {
        private string jobStatus;
        private bool intermediateProgress;
        private double progressValue;

        private EncodeProgressEventArgs progressEventArgs;

        public QueueProgressStatus()
        {
        }

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

        public int? Task => this.progressEventArgs?.Task ?? 0;

        public int? TaskCount => this.progressEventArgs?.TaskCount ?? 0;

        public TimeSpan? EstimatedTimeLeft => this.progressEventArgs?.EstimatedTimeLeft ?? null;
        
        public void Update(EncodeProgressEventArgs e)
        {
            progressEventArgs = e;
            this.IntermediateProgress = false;

            string totalHrsLeft = e.EstimatedTimeLeft.Days >= 1 ? string.Format(@"{0:d\:hh\:mm\:ss}", e.EstimatedTimeLeft) : string.Format(@"{0:hh\:mm\:ss}", e.EstimatedTimeLeft);
            string elapsedTimeHrs = e.ElapsedTime.Days >= 1 ? string.Format(@"{0:d\:hh\:mm\:ss}", e.ElapsedTime) : string.Format(@"{0:hh\:mm\:ss}", e.ElapsedTime);

            if (e.IsSubtitleScan)
            {
                this.JobStatus = string.Format(Resources.MainViewModel_EncodeStatusChanged_SubScan_StatusLabel,
                    e.Task,
                    e.TaskCount,
                    e.PercentComplete,
                    totalHrsLeft,
                    elapsedTimeHrs,
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
                        totalHrsLeft,
                        elapsedTimeHrs,
                        null);
                this.ProgressValue = e.PercentComplete;
            }
        }

        public void SetPaused()
        {
            this.ClearStatusDisplay();
            this.JobStatus = Resources.QueueViewModel_QueuePaused;
        }

        public void ClearStatusDisplay()
        {
            this.JobStatus = string.Empty;
            this.ProgressValue = 0;
            this.IntermediateProgress = false;
        }
    }
}
