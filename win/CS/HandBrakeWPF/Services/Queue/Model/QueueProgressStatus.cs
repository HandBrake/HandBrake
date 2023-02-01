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

    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Encode.EventArgs;
    using HandBrakeWPF.ViewModels;

    public class QueueProgressStatus : PropertyChangedBase
    {
        private readonly object lockObj = new object();

        private string jobStatus;
        private bool intermediateProgress;
        private double progressValue;
        private string jobStatusShort;

        private EncodeProgressEventArgs progressEventArgs;

        public QueueProgressStatus()
        {
            this.JobStatus = "Waiting";
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

        public string JobStatusShort
        {
            get => this.jobStatusShort;
            set
            {
                if (value == this.jobStatusShort) return;
                this.jobStatusShort = value;
                this.NotifyOfPropertyChange(() => this.JobStatusShort);
            }
        }

        public bool IntermediateProgress
        {
            get => this.intermediateProgress;
            set
            {
                if (value == this.intermediateProgress)
                {
                    return;
                }

                this.intermediateProgress = value;
                this.NotifyOfPropertyChange(() => this.IntermediateProgress);
            }
        }

        public double ProgressValue
        {
            get => this.progressValue;
            set
            {
                if (value == this.progressValue)
                {
                    return;
                }

                this.progressValue = value;
                this.NotifyOfPropertyChange(() => this.ProgressValue);
            }
        }

        public int? Task => this.progressEventArgs?.Task ?? 0;

        public int? TaskCount => this.progressEventArgs?.TaskCount ?? 0;

        public TimeSpan? EstimatedTimeLeft => this.progressEventArgs?.EstimatedTimeLeft ?? null;

        public double AverageFrameRate { get; private set; }
        
        public void Update(EncodeProgressEventArgs e)
        {
            lock (lockObj) 
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
                    this.JobStatusShort = string.Format(Resources.QueueViewModel_ShortSubScanStatus, e.PercentComplete, totalHrsLeft);
                }
                else if (e.IsMuxing)
                {
                    this.JobStatus = Resources.MainView_Muxing;
                    this.JobStatusShort = Resources.MainView_Muxing;
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
                    this.AverageFrameRate = e.AverageFrameRate;

                    this.JobStatusShort =
                        string.Format(
                            Resources.QueueViewModel_ShortEncodeStatus,
                            e.Task,
                            e.TaskCount,
                            e.PercentComplete,
                            e.CurrentFrameRate,
                            e.AverageFrameRate,
                            totalHrsLeft);
                }
            }
        }

        public void Update(EncodeCompletedEventArgs e)
        {
            lock (lockObj)
            {
                if (e.Successful)
                {
                    this.JobStatus = Resources.QueueView_JobStatus_Complete;
                }
                else
                {
                    switch (e.ErrorInformation)
                    {
                        case "1": // HB_ERROR_CANCELED
                            this.JobStatus = Resources.QueueView_JobStatus_Cancelled;
                            break;
                        case "2": // HB_ERROR_WRONG_INPUT
                            this.JobStatus = Resources.QueueView_JobStatus_InvalidInput;
                            break;
                        case "3": // HB_ERROR_INIT
                            this.JobStatus = Resources.QueueView_JobStatus_InitFailed;
                            break;
                        case "4": // HB_ERROR_UNKNOWN
                            this.JobStatus = Resources.QueueView_JobStatus_Unknown;
                            break;
                        case "5": // HB_ERROR_READ
                            this.JobStatus = Resources.QueueView_JobStatus_ReadError;
                            break;
                        case "-11": // Worker Crash
                            this.JobStatus = Resources.QueueView_JobStatus_WorkerCrash;
                            break;
                        default:
                            this.JobStatus = Resources.QueueView_JobStatus_NoErrorCode;
                            break;
                    }
                }
            }
        }
        
        public void SetPaused()
        {
            this.ClearStatusDisplay();
            this.JobStatus = Resources.QueueViewModel_QueuePaused;
        }

        public void ClearStatusDisplay()
        {
            this.ProgressValue = 0;
            this.IntermediateProgress = false;
        }
    }
}
