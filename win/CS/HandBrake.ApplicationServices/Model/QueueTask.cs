// --------------------------------------------------------------------------------------------------------------------
// <copyright file="QueueTask.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The QueueTask.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Model
{
    using Caliburn.Micro;

    using HandBrake.ApplicationServices.Parsing;

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
        /// Gets or sets ScannedSource.
        /// </summary>
        public Source ScannedSource { get; set; } 

        /// <summary>
        /// Gets or sets a value indicating whether if this is a user or GUI generated query
        /// </summary>
        public bool CustomQuery { get; set; }

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
        /// Gets or sets the Encode Task.
        /// </summary>
        public EncodeTask Task { get; set; }

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
            return Equals(this.ScannedSource, other.ScannedSource) && this.CustomQuery.Equals(other.CustomQuery) && Equals(this.Task, other.Task) && this.status == other.status;
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

            return Equals((QueueTask)obj);
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
                int hashCode = (this.ScannedSource != null ? this.ScannedSource.GetHashCode() : 0);
                hashCode = (hashCode * 397) ^ this.CustomQuery.GetHashCode();
                hashCode = (hashCode * 397) ^ (this.Task != null ? this.Task.GetHashCode() : 0);
                hashCode = (hashCode * 397) ^ (int)this.status;
                return hashCode;
            }
        }
    }
}