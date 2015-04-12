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
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Services.Encode.Model;
    using HandBrake.ApplicationServices.Utilities;

    /// <summary>
    /// The QueueTask.
    /// </summary>
    public class QueueTask : PropertyChangedBase
    {
        #region Constants and Fields

        /// <summary>
        /// The status.
        /// </summary>
        private QueueItemStatus status;

        #endregion

        #region Properties

        /// <summary>
        /// Initializes a new instance of the <see cref="QueueTask"/> class.
        /// </summary>
        public QueueTask()
        {
            this.Status = QueueItemStatus.Waiting;
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
        public QueueTask(EncodeTask task, HBConfiguration configuration, string scannedSourcePath)
        {
            this.Task = task;
            this.Configuration = configuration;
            this.Status = QueueItemStatus.Waiting;
            this.ScannedSourcePath = scannedSourcePath;
        }

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

        #endregion

        /// <summary>
        /// The equals.
        /// </summary>
        /// <param name="other">
        /// The other.
        /// </param>
        /// <returns>
        /// The <see cref="bool"/>.
        /// </returns>
        protected bool Equals(QueueTask other)
        {
            return Equals(this.ScannedSourcePath, other.ScannedSourcePath) && Equals(this.Task, other.Task) && this.status == other.status;
        }

        /// <summary>
        /// The equals.
        /// </summary>
        /// <param name="obj">
        /// The obj.
        /// </param>
        /// <returns>
        /// The <see cref="bool"/>.
        /// </returns>
        public override bool Equals(object obj)
        {
            if (ReferenceEquals(null, obj))
            {
                return false;
            }

            if (ReferenceEquals(this, obj))
            {
                return true;
            }

            if (obj.GetType() != this.GetType())
            {
                return false;
            }

            return this.Equals((QueueTask)obj);
        }

        /// <summary>
        /// The get hash code.
        /// </summary>
        /// <returns>
        /// The <see cref="int"/>.
        /// </returns>
        public override int GetHashCode()
        {
            unchecked
            {
                int hashCode = (this.ScannedSourcePath != null ? this.ScannedSourcePath.GetHashCode() : 0);
                hashCode = (hashCode * 397) ^ (this.Task != null ? this.Task.GetHashCode() : 0);
                hashCode = (hashCode * 397) ^ (int)this.status;
                return hashCode;
            }
        }
    }
}