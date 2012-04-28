/*  QueueTask.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Model
{
    using Caliburn.Micro;

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

        #region Constructors and Destructors

        /// <summary>
        /// Initializes a new instance of the <see cref="QueueTask"/> class.
        /// </summary>
        public QueueTask()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="QueueTask"/> class.
        /// </summary>
        /// <param name="query">
        /// The query.
        /// </param>
        public QueueTask(string query)
        {
            this.Query = query;
            if (this.Task == null)
            {
                this.Task = new EncodeTask();
            }
        }

        #endregion

        #region Properties

        /// <summary>
        /// Gets or sets a value indicating whether if this is a user or GUI generated query
        /// </summary>
        public bool CustomQuery { get; set; }

        /// <summary>
        /// Gets or sets the query string.
        /// </summary>
        public string Query { get; set; }

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
    }
}