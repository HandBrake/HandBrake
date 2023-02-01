// --------------------------------------------------------------------------------------------------------------------
// <copyright file="QueueTask.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The QueueTask.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Queue.Model
{
    using System;
    using System.Text.Json.Serialization;

    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.Services.Scan.Model;
    using HandBrakeWPF.Utilities;
    using HandBrakeWPF.ViewModels;

    using EncodeTask = Encode.Model.EncodeTask;

    public class QueueTask : PropertyChangedBase
    {
        private static int id;
        private QueueItemStatus status;
        private string presetKey;

        public QueueTask()
        {
            this.Status = QueueItemStatus.Waiting;
            id = id + 1;
            this.Id = string.Format("{0}.{1}", GeneralUtilities.ProcessId, id);
            this.Statistics = new QueueStats();
            this.JobProgress = new QueueProgressStatus();
        }

        public QueueTask(QueueTaskType type)
        {
            this.TaskType = type;
            id = id + 1;
            this.Id = string.Format("{0}.{1}", GeneralUtilities.ProcessId, id);
            this.NotifyOfPropertyChange(() => this.IsBreakpointTask);
        }

        public QueueTask(EncodeTask task, string scannedSourcePath, Preset currentPreset, bool isPresetModified, Title selectedTitle)
        {
            this.SourceTitleInfo = selectedTitle;
            this.Task = task;
            this.Status = QueueItemStatus.Waiting;
            this.ScannedSourcePath = scannedSourcePath;
            if (currentPreset != null)
            {
                this.presetKey = currentPreset.Name;
                if (isPresetModified)
                {
                    this.presetKey = this.presetKey + " (Modified)";
                }
            }
            id = id + 1;
            this.Id = string.Format("{0}.{1}", GeneralUtilities.ProcessId, id);
            this.SelectedPresetKey = this.presetKey;

            this.Statistics = new QueueStats();
            this.Statistics.UpdateStats(this, null);
            this.TaskId = Guid.NewGuid().ToString();
            this.JobProgress = new QueueProgressStatus();
            this.TaskType = QueueTaskType.EncodeTask;
        }
        
        [JsonIgnore]
        public string Id { get; }

        public string TaskId { get; set; }

        public QueueTaskType TaskType { get; set; }

        /* Breakpoint Task */

        public bool IsBreakpointTask => TaskType == QueueTaskType.Breakpoint;

        /* Encode Task*/

        public string ScannedSourcePath { get; set; }

        [JsonIgnore]
        public Guid? TaskToken { get; set; }

        public Title SourceTitleInfo { get; }

        public QueueItemStatus Status
        {
            get => this.status;

            set
            {
                this.status = value;
                this.NotifyOfPropertyChange(() => this.Status);
                this.NotifyOfPropertyChange(() => this.ShowEncodeProgress);
                this.NotifyOfPropertyChange(() => this.IsJobStatusVisible);
                this.NotifyOfPropertyChange(() => this.ShowJobCompleteInfo);
            }
        }

        public EncodeTask Task { get; set; }

        public QueueStats Statistics { get; set; }

        public string SelectedPresetKey { get; set; }

        [JsonIgnore]
        public QueueProgressStatus JobProgress { get; set; }

        [JsonIgnore]
        public bool IsJobStatusVisible => this.Status == QueueItemStatus.InProgress;
        
        [JsonIgnore]
        public bool ShowEncodeProgress => this.Status == QueueItemStatus.InProgress;

        [JsonIgnore]
        public bool ShowJobCompleteInfo => this.Status == QueueItemStatus.Completed;

        /* Overrides */

        public override bool Equals(object obj)
        {
            if (ReferenceEquals(null, obj)) return false;
            if (ReferenceEquals(this, obj)) return true;
            if (obj.GetType() != this.GetType()) return false;
            return Equals((QueueTask)obj);
        }

        public override int GetHashCode()
        {
            return this.Id.GetHashCode();
        }

        public override string ToString()
        {
            return string.Format("Encode Task.  Title: {0}, Source: {1}, Destination: {2}", this.Task?.Title, this.Task?.Source, this.Task?.Destination);
        }

        protected bool Equals(QueueTask other)
        {
            return this.Id == other.Id;
        }
    }
}