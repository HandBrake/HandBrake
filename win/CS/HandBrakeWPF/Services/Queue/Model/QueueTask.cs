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
    using System.Runtime.CompilerServices;

    using Caliburn.Micro;

    using HandBrake.Interop.Model;

    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.Utilities;

    using Newtonsoft.Json;

    using EncodeTask = HandBrakeWPF.Services.Encode.Model.EncodeTask;

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
        }

        public QueueTask(EncodeTask task, HBConfiguration configuration, string scannedSourcePath, Preset currentPreset, bool isPresetModified)
        {
            this.Task = task;
            this.Configuration = configuration;
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

            this.Statistics = new QueueStats();
        }

        [JsonIgnore]
        public string Id { get; }

        [JsonProperty]
        public string ScannedSourcePath { get; set; }

        [JsonProperty]
        public QueueItemStatus Status
        {
            get
            {
                return this.status;
            }

            set
            {
                this.status = value;
                this.NotifyOfPropertyChange(() => this.Status);
                this.NotifyOfPropertyChange(() => this.ShowEncodeProgress);
            }
        }

        [JsonProperty]
        public EncodeTask Task { get; set; }

        [JsonProperty]
        public HBConfiguration Configuration { get; set; }

        [JsonProperty]
        public QueueStats Statistics { get; set; }

        [JsonIgnore]
        public string SelectedPresetKey
        {
            get
            {
                return this.presetKey;
            }
        }

        [JsonIgnore]
        public bool ShowEncodeProgress => this.Status == QueueItemStatus.InProgress && SystemInfo.IsWindows10();

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
            return string.Format("Encode Task.  Title: {0}, Source: {1}, Destination: {2}", this.Task.Title, this.Task.Source, this.Task.Destination);
        }

        protected bool Equals(QueueTask other)
        {
            return this.Id == other.Id;
        }
    }
}