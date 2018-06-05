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
    using Caliburn.Micro;

    using HandBrake.Interop.Model;

    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.Utilities;

    using EncodeTask = HandBrakeWPF.Services.Encode.Model.EncodeTask;

    /// <summary>
    /// The QueueTask.
    /// </summary>
    public class QueueTask : PropertyChangedBase
    {
        private static int id;
        private QueueItemStatus status;
        private string presetKey;

        #region Properties

        /// <summary>
        /// Initializes a new instance of the <see cref="QueueTask"/> class.
        /// </summary>
        public QueueTask()
        {
            this.Status = QueueItemStatus.Waiting;
            id = id + 1;
            this.Id = string.Format("{0}.{1}", GeneralUtilities.ProcessId, id);
            this.Statistics = new QueueStats();
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="QueueTask"/> class.
        /// </summary>
        /// <param name="task">
        /// The task.
        /// </param>
        /// <param name="configuration">
        /// The configuration.
        /// </param>
        /// <param name="scannedSourcePath">
        /// The scanned Source Path.
        /// </param>
        /// <param name="currentPreset">
        /// The currently active preset.
        /// </param>
        public QueueTask(EncodeTask task, HBConfiguration configuration, string scannedSourcePath, Preset currentPreset)
        {
            this.Task = task;
            this.Configuration = configuration;
            this.Status = QueueItemStatus.Waiting;
            this.ScannedSourcePath = scannedSourcePath;
            if (currentPreset != null)
            {
                this.presetKey = currentPreset.Name;
            }

            id = id + 1;
            this.Id = string.Format("{0}.{1}", GeneralUtilities.ProcessId, id);

            this.Statistics = new QueueStats();
        }

        public string Id { get; }

        /// <summary>
        /// Gets or sets ScannedSource.
        /// </summary>
        public string ScannedSourcePath { get; set; }

        /// <summary>
        /// Gets or sets Status.
        /// </summary>
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
            }
        }

        /// <summary>
        /// Gets or sets the task.
        /// </summary>
        public EncodeTask Task { get; set; }

        /// <summary>
        /// Gets or sets the configuration.
        /// </summary>
        public HBConfiguration Configuration { get; set; }

        public QueueStats Statistics { get; set; }

        public string SelectedPresetKey
        {
            get
            {
                return this.presetKey;
            }
        }

        #endregion

        protected bool Equals(QueueTask other)
        {
            return this.Id == other.Id;
        }

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
    }
}